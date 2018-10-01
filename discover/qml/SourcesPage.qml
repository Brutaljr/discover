import QtQuick 2.4
import QtQuick.Controls 2.1
import QtQuick.Layouts 1.1
import org.kde.discover 2.0
import org.kde.discover.app 1.0
import org.kde.kirigami 2.2 as Kirigami
import "navigation.js" as Navigation

DiscoverPage {
    id: page
    clip: true
    title: i18n("Settings")
    property string search: ""

    contextualActions: [
        KirigamiActionBridge { action: app.action("help_about_app") },
        KirigamiActionBridge { action: app.action("help_report_bug") }
    ]

    mainItem: ListView {
        id: sourcesView
        model: QSortFilterProxyModel {
            filterRegExp: new RegExp(page.search, 'i')
            dynamicSortFilter: false //We don't want to sort, as sorting can have some semantics on some backends
            sourceModel: SourcesModel
        }
        currentIndex: -1

        section.property: "sourceName"
        section.delegate: Kirigami.AbstractListItem {
            id: backendItem
            hoverEnabled: false
            supportsMouseEvents: false
            readonly property QtObject backend: SourcesModel.sourcesBackendByName(section)
            readonly property QtObject resourcesBackend: backend.resourcesBackend
            readonly property bool isDefault: ResourcesModel.currentApplicationBackend == resourcesBackend

            RowLayout {
                Layout.fillHeight: true
                Connections {
                    target: backendItem.backend
                    onPassiveMessage: window.showPassiveNotification(message)
                    onProceedRequest: {
                        var dialog = sourceProceedDialog.createObject(window, {sourcesBackend: sourcesBackend, title: title, description: description})
                        dialog.open()
                    }
                }

                Kirigami.Heading {
                    Layout.fillWidth: true
                    Layout.leftMargin: Kirigami.Units.smallSpacing
                    Layout.alignment: Qt.AlignVCenter
                    text: backendItem.isDefault ? i18n("%1 (Default)", resourcesBackend.displayName) : resourcesBackend.displayName
                    level: 3
                }
                Button {
                    Layout.alignment: Qt.AlignVCenter
                    icon.name: "preferences-other"

                    visible: resourcesBackend && resourcesBackend.hasApplications
                    Component {
                        id: dialogComponent
                        AddSourceDialog {
                            source: backendItem.backend
                            onVisibleChanged: if (!visible) {
                                destroy()
                            }
                        }
                    }

                    id: this
                    onClicked: settingsMenu.popup(this)
                    Menu {
                        id: settingsMenu
                        MenuItem {
                            enabled: !backendItem.isDefault
                            text: i18n("Make default")
                            onTriggered: ResourcesModel.currentApplicationBackend = backendItem.backend.resourcesBackend
                        }

                        MenuItem {
                            text: i18n("Add Source...")
                            visible: backendItem.backend && backendItem.backend.supportsAdding

                            onTriggered: {
                                var addSourceDialog = dialogComponent.createObject(null, {displayName: backendItem.backend.resourcesBackend.displayName })
                                addSourceDialog.open()
                            }
                        }

                        MenuSeparator {
                            visible: backendActionsInst.count>0
                        }

                        Instantiator {
                            id: backendActionsInst
                            model: ActionsModel {
                                actions: backendItem.backend ? backendItem.backend.actions : undefined
                            }
                            delegate: MenuItem {
                                action: ActionBridge {
                                    action: modelData
                                }
                            }
                            onObjectAdded: {
                                settingsMenu.insertItem(index, object)
                            }
                            onObjectRemoved: {
                                object.destroy()
                            }
                        }
                    }
                }
            }
        }

        Component {
            id: sourceProceedDialog
            Kirigami.OverlaySheet {
                id: sheet
                showCloseButton: false
                property QtObject  sourcesBackend
                property alias title: heading.text
                property alias description: desc.text
                property bool acted: false
                ColumnLayout {
                    Kirigami.Heading {
                        id: heading
                    }
                    Label {
                        id: desc
                        Layout.fillWidth: true
                        textFormat: Text.StyledText
                        wrapMode: Text.WordWrap
                    }
                    RowLayout {
                        Layout.alignment: Qt.AlignRight
                        Button {
                            text: i18n("Proceed")
                            icon.name: "dialog-ok"
                            onClicked: {
                                sourcesBackend.proceed()
                                sheet.acted = true
                                sheet.close()
                            }
                        }
                        Button {
                            Layout.alignment: Qt.AlignRight
                            text: i18n("Cancel")
                            icon.name: "dialog-cancel"
                            onClicked: {
                                sourcesBackend.cancel()
                                sheet.acted = true
                                sheet.close()
                            }
                        }
                    }
                }
                onSheetOpenChanged: if(!sheetOpen) {
                    sheet.destroy(1000)
                    if (!sheet.acted)
                        sourcesBackend.cancel()
                }
            }
        }

        delegate: Kirigami.SwipeListItem {
            Layout.fillWidth: true
            enabled: display.length>0
            highlighted: ListView.isCurrentItem
            onClicked: Navigation.openApplicationListSource(sourceId)

            Keys.onReturnPressed: clicked()
            actions: [
                Kirigami.Action {
                    iconName: "go-up"
                    enabled: sourcesBackend.firstSourceId !== sourceId
                    visible: sourcesBackend.canMoveSources
                    onTriggered: {
                            var ret = sourcesBackend.moveSource(sourceId, -1)
                            if (!ret)
                                window.showPassiveNotification(i18n("Failed to increase '%1' preference", display))
                    }
                },
                Kirigami.Action {
                    iconName: "go-down"
                    enabled: sourcesBackend.lastSourceId !== sourceId
                    visible: sourcesBackend.canMoveSources
                    onTriggered: {
                        var ret = sourcesBackend.moveSource(sourceId, +1)
                        if (!ret)
                                window.showPassiveNotification(i18n("Failed to decrease '%1' preference", display))
                    }
                },
                Kirigami.Action {
                    iconName: "edit-delete"
                    tooltip: i18n("Delete the origin")
                    visible: sourcesBackend.supportsAdding
                    onTriggered: {
                        var backend = sourcesBackend
                        if (!backend.removeSource(sourceId)) {
                            window.showPassiveNotification(i18n("Failed to remove the source '%1'", display))
                        }
                    }
                }
            ]

            RowLayout {
                CheckBox {
                    id: enabledBox

                    readonly property variant idx: sourcesView.model.index(index, 0)
                    readonly property variant modelChecked: sourcesView.model.data(idx, Qt.CheckStateRole)
                    checked: modelChecked != Qt.Unchecked
                    enabled: modelChecked !== undefined
                    onClicked: {
                        sourcesView.model.setData(idx, checkedState, Qt.CheckStateRole)
                    }
                }
                Label {
                    text: display + (toolTip ? " - <i>" + toolTip + "</i>" : "")
                    elide: Text.ElideRight
                    Layout.fillWidth: true
                }
            }
        }

        footer: ColumnLayout {
            id: foot
            anchors {
                right: parent.right
                left: parent.left
                margins: Kirigami.Units.smallSpacing
            }
            Kirigami.Heading {
                Layout.fillWidth: true
                text: i18n("Missing Backends")
                visible: back.count>0
            }
            spacing: 0
            Repeater {
                id: back
                model: ResourcesProxyModel {
                    extending: "org.kde.discover.desktop"
                    filterMinimumState: false
                }
                delegate: Kirigami.BasicListItem {
                    supportsMouseEvents: false
                    label: name
                    icon: model.icon
                    InstallApplicationButton {
                        application: model.application
                    }
                }
            }
        }
    }
}
