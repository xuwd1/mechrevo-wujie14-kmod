import QtQuick 2.6
import QtQuick.Controls 1.3
import QtQuick.Layouts 1.1

Item {
    property alias cfg_updateInterval: updateInterval.value
    property alias cfg_makeFontBold: makeFontBold.checked

    ColumnLayout {
        RowLayout {
            Label {
                id: updateIntervalLabel
                text: i18n(" Update interval:")
            }
            SpinBox {
                id: updateInterval
                decimals: 1
                stepSize: 0.1
                minimumValue: 0.1
                suffix: i18nc("Abbreviation for seconds", "s")
            }
        }

        CheckBox {
            id: makeFontBold
            text: i18n("Bold Text")
        }

    }
}
