cmake_minimum_required(VERSION 3.23)
project(wujie14-km VERSION 0.1 LANGUAGES C)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# default kernel header location
execute_process(COMMAND uname -r OUTPUT_VARIABLE UNAME_R)
set(DEFAULT_KHEADER_DIR /lib/modules/${UNAME_R}/build)

# user options
set(FULL_KERNEL_DIR "" CACHE STRING "Path to full kernel source tree")
set(HEADER_ONLY_KERNEL_DIR ${DEFAULT_KHEADER_DIR} CACHE STRING "Path to header-only kernel source tree") 
option(HEADER_BUILD "Use full kernel tree for compilation database and build with header-only kernel tree. \
                     If set ON, both FULL_KERNEL_DIR and HEADER_ONLY_KERNEL_DIR have to be provided, \
                     otherwise use HEADER_ONLY_KERNEL_DIR for both compilation database and building" OFF)
option(CLANGD "Remove compile flags that are not understood by clangd" ON)
option(DKMS "Generate dkms.conf by configuring dkms.conf.in and enable dkms support" ON)

# tell cmake to generate JSON compilation database
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)



if(HEADER_BUILD)
    if(${FULL_KERNEL_DIR} STREQUAL "")
        message(FATAL_ERROR "No full kernel source tree provided")
    endif()
    # probe for compile_commands.json under full kernel source tree
    if(EXISTS ${FULL_KERNEL_DIR}/compile_commands.json)
    else()
        message(WARNING "No compile_commands.json found under full kernel source tree !!! \
                         Language server may (definitely) NOT work well when navigating under the kernel source tree. \
                         To generate it, first build a kernel image from the full kernel source, \
                         and run 'python /path/to/source/scripts/clang-tools/gen_compile_commands.py'")
    endif()
    if(${HEADER_ONLY_KERNEL_DIR} STREQUAL "")
        message(FATAL_ERROR "No header-only kernel source tree provided")
    endif()
    # compilation database tree
    set(KERNELDIR_CD ${FULL_KERNEL_DIR})
    # build tree
    set(KERNELDIR_BUILD ${HEADER_ONLY_KERNEL_DIR})
else()
    if(${HEADER_ONLY_KERNEL_DIR} STREQUAL "")
        message(FATAL_ERROR "No header-only kernel source tree provided")
    endif()
    set(KERNELDIR_BUILD ${HEADER_ONLY_KERNEL_DIR})
    set(KERNELDIR_CD ${KERNELDIR_BUILD})
endif()

    

### start of Kbuild generation ###
# specify module source files here
# MODULE_NAME: the module name. It would correspond to the '$obj-m' in the Kbuild file.
# MODULE_SRC_LIST: source files that the module uses. They would correspond to the '${MODULE_NAME}-y' in the Kbuild file.

set(MODULE_NAME wujie14)
list(APPEND MODULE_SRC_LIST 
    wujie14-km.c 
    wujie14-perfmode.c 
    wujie14-wmi-event.c
    wujie14-kb.c)


list(APPEND ALL_MODULE_SRC ${MODULE_SRC_LIST})
# first we generate the obj-m string 
set(KBUILD_OBJ_M_STR "obj-m := ")
string(CONCAT MODULE_OBJ_NAME ${MODULE_NAME} ".o")
string(CONCAT KBUILD_OBJ_M_STR ${KBUILD_OBJ_M_STR} ${MODULE_OBJ_NAME} "\n")


# next we generate the '${MODULE_NAME}'-y string

string(CONCAT KBUILD_OBJ_Y_STR ${MODULE_NAME} "-objs := ")
foreach(LIB_SRCFILE ${MODULE_SRC_LIST})
    string(REGEX REPLACE "\.c" "" TU_NAME ${LIB_SRCFILE})
    string(CONCAT LIB_OBJ_NAME ${TU_NAME} ".o")
    string(CONCAT KBUILD_OBJ_Y_STR ${KBUILD_OBJ_Y_STR} ${LIB_OBJ_NAME} " ")
endforeach()

string(CONCAT KBUILD_OBJ_Y_STR ${KBUILD_OBJ_Y_STR} "\n")
# the generated Kbuild file will be at ${CMAKE_SOURCE_DIR}/Kbuild
set(GENERATED_KBUILD_FILE ${CMAKE_SOURCE_DIR}/Kbuild)
file(WRITE ${GENERATED_KBUILD_FILE} ${KBUILD_OBJ_M_STR} ${KBUILD_OBJ_Y_STR})

### end of Kbuild generation ###

### start of dkms generation ###
if(DKMS AND EXISTS ${CMAKE_SOURCE_DIR}/dkms.conf.in)
    set(DKMS_PACKAGE_NAME ${CMAKE_PROJECT_NAME})
    set(DKMS_PACKAGE_VERSION ${PROJECT_VERSION})
    # actually dkms is smart enough to get a correct make and clean command
    #set(DKMS_MAKECMD "$(MAKE) -C ${KERNELDIR_BUILD} M=$(CURDIR) modules")
    #set(DKMS_CLEANCMD "$(MAKE) -C ${KERNELDIR_BUILD} M=$(CURDIR) clean")
    set(DKMS_DEST_LOCATION "/extramodules")
    set(GENERATED_DKMS_CONF ${CMAKE_SOURCE_DIR}/dkms.conf)
    configure_file(dkms.conf.in ${CMAKE_SOURCE_DIR}/dkms.conf)
endif()


### start of dummy module building ###
# The generated Kbuild file enables us to build our module already, and once a successful build has completed
# it is possible to generate a JSON compilation database for our module
# However this style is NOT desired as we cannot get the compilation database BEFORE a successful module build
# Thus instead, we will build a dummy module here to gather the compile options that the Kbuild system emitted
# and use these compile options to generate a JSON compilation database for our module BEFORE building it.
# i.e., to generate compilation database at cmake comfiguring time, as a usual cmake project would do.

set(DUMMY_MODULE_DIR ${CMAKE_BINARY_DIR}/dummy_mod)
file(REMOVE_RECURSE ${DUMMY_MODULE_DIR})
file(MAKE_DIRECTORY ${DUMMY_MODULE_DIR})
file(WRITE ${DUMMY_MODULE_DIR}/dummy_mod.c "
#include <linux/module.h>  
#include <linux/kernel.h>   
#include <linux/init.h>     
MODULE_LICENSE(\"GPL\");  
MODULE_AUTHOR(\"Nyaruko\");   
MODULE_DESCRIPTION(\"Dummy\");  
MODULE_VERSION(\"3.3\");        
static int __init hello_start(void) { return 0; } 
static void __exit hello_end(void) { } 
module_init(hello_start); 
module_exit(hello_end);  
")
file(WRITE ${DUMMY_MODULE_DIR}/Kbuild "obj-m := dummy_mod.o")
execute_process(COMMAND make -C ${KERNELDIR_BUILD} M=${DUMMY_MODULE_DIR} RESULT_VARIABLE DUMMY_MOD_MAKE_RESULT)
if (DUMMY_MOD_MAKE_RESULT EQUAL 0)
else()
    message(FATAL_ERROR "Build dummy module failed. This is likely due to a wrong kernel source directory was provided.")
endif()

### end of dummy module building ###

# now we do some simple parsing on the dummy module's compile commands to get the compile option
file(STRINGS ${DUMMY_MODULE_DIR}/.dummy_mod.mod.o.cmd DUMMY_MOD_COMPILE_CMDS)
list(GET DUMMY_MOD_COMPILE_CMDS 0 DUMMY_MOD_COMPILE_COMMAND)
string(REGEX MATCHALL " -[^ ]*" DUMMY_MOD_COMPILE_OPTIONS ${DUMMY_MOD_COMPILE_COMMAND})
list(TRANSFORM DUMMY_MOD_COMPILE_OPTIONS STRIP)
# -Wp,MMD, option has no effect.
# -DKBUILD_MODNAME, -DKBUILD_BASENAME, -D__KBUILD_MODNAME should be set for each source file accordingly. 
# -include option is hard to parse with REGEX so add them manually
list(FILTER DUMMY_MOD_COMPILE_OPTIONS EXCLUDE REGEX "-Wp.*|-DKBUILD.*|-include|-D__KBUILD.*|-o|-c")
# fix -I pathes
list(TRANSFORM DUMMY_MOD_COMPILE_OPTIONS REPLACE "-I." "-I${KERNELDIR_CD}" OUTPUT_VARIABLE COMMON_COMPILE_OPTIONS)
# add -include options
list(APPEND COMMON_COMPILE_OPTIONS "SHELL:-include ${KERNELDIR_CD}/include/linux/compiler-version.h")
list(APPEND COMMON_COMPILE_OPTIONS "SHELL:-include ${KERNELDIR_CD}/include/linux/kconfig.h")
list(APPEND COMMON_COMPILE_OPTIONS "SHELL:-include ${KERNELDIR_CD}/include/linux/compiler_types.h")
# By now COMMON_COMPILE_OPTIONS contains all the common compile options for every module source file.

# exclude commpile options that are not understood by clangd
if(CLANGD)
    set(CLANGD_EXCLUDE_OPTION_REGEX "--hack.*|--ibt|--orc|--retpoline|--rethunk|--sls|--static-call|--uaccess|--link|--module|-mpreferred-stack.*|-mindirect-branch.*|-fno-allow-store.*|-fconserve-stack|-mrecord-mcount|-ftrivial-auto.*")
    list(FILTER COMMON_COMPILE_OPTIONS EXCLUDE REGEX ${CLANGD_EXCLUDE_OPTION_REGEX})
endif()

# add a dummy library target for each source file so that cmake would be happy to generate the compilation database for them
# meanwhile set the compile options that would show up in the database
foreach(MODULE_SRCFILE ${ALL_MODULE_SRC})
    add_library(${MODULE_SRCFILE}_dummy_target ${MODULE_SRCFILE})
    target_compile_options(${MODULE_SRCFILE}_dummy_target PRIVATE ${COMMON_COMPILE_OPTIONS})
    target_compile_definitions(${MODULE_SRCFILE}_dummy_target PRIVATE -DKBUILD_MODNAME=\"${MODULE_NAME}\")
    string(REGEX REPLACE "\.c" "" BASENAME ${MODULE_SRCFILE})
    target_compile_definitions(${MODULE_SRCFILE}_dummy_target PRIVATE -DKBUILD_BASENAME=\"${BASENAME}\")
    target_compile_definitions(${MODULE_SRCFILE}_dummy_target PRIVATE -D__KBUILD_MODNAME=kmod_${MODULE_NAME})
endforeach()


# finally the custom target 'module' for actually building the module
# thus, you build your module after cmake configuring with 'make module'
set(MODULE_KO ${MODULE_NAME}.ko)
set(KBUILD_CMD $(MAKE) -C ${KERNELDIR_BUILD} modules M=${CMAKE_BINARY_DIR} src=${CMAKE_SOURCE_DIR})
add_custom_command(OUTPUT ${MODULE_KO}
        COMMAND ${KBUILD_CMD}
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        DEPENDS ${ALL_MODULE_SRC} VERBATIM)
add_custom_target(module ALL DEPENDS ${MODULE_KO})
# custom target 'module-clean' for cleaning
add_custom_target(module-clean COMMAND ${KBUILD_CMD} clean VERBATIM)
# custom target 'gen-clean' that cleans the generated files
list(APPEND ALL_GEN_FILES ${GENERATED_KBUILD_FILE})
if(DKMS)
    list(APPEND ALL_GEN_FILES ${GENERATED_DKMS_CONF})
endif()
add_custom_target(gen-clean COMMAND rm ${ALL_GEN_FILES} VERBATIM)
# custom target 'allclean' that cleans everything
add_custom_target(allclean DEPENDS gen-clean module-clean)
# custom target 'module-xz' that compresses the module object
set(MODULE_KO_XZ ${MODULE_KO}.xz)
add_custom_command(OUTPUT ${MODULE_KO_XZ}
                   COMMAND xz -T1 -fk ${MODULE_KO}
                   WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
                   DEPENDS ${MODULE_KO} VERBATIM)
add_custom_target(module-xz ALL DEPENDS ${MODULE_KO_XZ})


# only dkms installing is provided
if(DKMS)   
    set(DKMS_MODULE_DIR /usr/src/${DKMS_PACKAGE_NAME}-${DKMS_PACKAGE_VERSION})
    add_custom_target(dkms-prepare 
        COMMAND mkdir -p ${DKMS_MODULE_DIR}
        COMMAND cp -f ${CMAKE_SOURCE_DIR}/wujie14-kb.c ${CMAKE_SOURCE_DIR}/wujie14-kb.h  ${DKMS_MODULE_DIR}
        COMMAND cp -f ${CMAKE_SOURCE_DIR}/wujie14-km.c ${CMAKE_SOURCE_DIR}/wujie14-km.h  ${DKMS_MODULE_DIR}
        COMMAND cp -f ${CMAKE_SOURCE_DIR}/wujie14-perfmode.c ${CMAKE_SOURCE_DIR}/wujie14-perfmode.h  ${DKMS_MODULE_DIR}
        COMMAND cp -f ${CMAKE_SOURCE_DIR}/wujie14-wmi-event.c ${CMAKE_SOURCE_DIR}/wujie14-wmi-event.h  ${DKMS_MODULE_DIR}
        COMMAND cp -f ${CMAKE_SOURCE_DIR}/dkms.conf ${DKMS_MODULE_DIR}
        COMMAND cp -f ${CMAKE_SOURCE_DIR}/Kbuild ${DKMS_MODULE_DIR}
        VERBATIM)

    add_custom_target(dkms-add 
        COMMAND dkms add -m ${DKMS_PACKAGE_NAME} -v ${DKMS_PACKAGE_VERSION}
        VERBATIM)

    add_custom_target(dkms-build
        COMMAND dkms build -m ${DKMS_PACKAGE_NAME} -v ${DKMS_PACKAGE_VERSION}
        VERBATIM)
    
    add_custom_target(dkms-install
        COMMAND dkms install -m ${DKMS_PACKAGE_NAME} -v ${DKMS_PACKAGE_VERSION}
        VERBATIM)

    add_custom_target(dkms-remove
        COMMAND dkms remove -m ${DKMS_PACKAGE_NAME} -v ${DKMS_PACKAGE_VERSION}
        VERBATIM)

    add_custom_target(dkms-clean
        COMMAND rm -rf ${DKMS_MODULE_DIR}
        VERBATIM)

endif()