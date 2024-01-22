import QtQuick 2.6
import QtQuick.Layouts 1.1
import org.kde.plasma.components 3.0 as PlasmaComponents
import org.kde.plasma.plasmoid 2.0

Item {
    id: main
    anchors.fill: parent

    //height and widht, when the widget is placed in desktop
    width: 40
    height: 40

    //height and width, when widget is placed in plasma panel
    Layout.preferredWidth: 40 * units.devicePixelRatio
    Layout.preferredHeight: 40 * units.devicePixelRatio

    Plasmoid.preferredRepresentation: Plasmoid.fullRepresentation



    function getPerfModeString(){
        var path = "/sys/firmware/acpi/platform_profile"
        var req = new XMLHttpRequest();
        req.open("GET", path, false);
        req.send(null);
        var perfmodestr = req.responseText
        return perfmodestr
    }

    function convertPerfModeString(arg_perfmode_str){
        if (arg_perfmode_str.startsWith("perf")) {
            return "P";
        } else if (arg_perfmode_str.startsWith("balance")) {
            return "B";
        } else {
            return "E";
        }
    }

    

    PlasmaComponents.Label {
        id: display

        anchors {
            fill: parent
            margins: Math.round(parent.width * 0.01)
        }

        verticalAlignment: Text.AlignVCenter
        horizontalAlignment: Text.AlignHCenter

        text: {
            return convertPerfModeString(getPerfModeString());
        }

        font.pixelSize: 800;
        minimumPointSize: theme.smallestFont.pointSize
        fontSizeMode: Text.Fit
        font.bold: plasmoid.configuration.makeFontBold
    }

    Timer {
        interval: plasmoid.configuration.updateInterval * 1000
        running: true
        repeat: true
        onTriggered: {
            display.text = convertPerfModeString(getPerfModeString());
        }
    }
}