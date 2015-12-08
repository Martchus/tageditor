# meta data
projectname = tageditor
appname = "Tag Editor"
appauthor = Martchus
appurl = "https://github.com/$${appauthor}/$${projectname}"
QMAKE_TARGET_DESCRIPTION = "A tageditor with Qt GUI and command line interface. Supports MP4 (iTunes), ID3, Vorbis and Matroska."
VERSION = 1.3.0

# include ../../common.pri when building as part of a subdirs project; otherwise include general.pri
!include(../../common.pri) {
    !include(./general.pri) {
        error("Couldn't find the common.pri or the general.pri file!")
    }
}

# basic configuration: application
TEMPLATE = app
QT += core gui widgets script
# use webenginewidgets if available; otherwise use webkitwidgets
!forcewebkit:qtHaveModule(webenginewidgets) {
    QT += webenginewidgets
    DEFINES += TAGEDITOR_USE_WEBENGINE
} else {
    QT += webkitwidgets
}

# add project files
HEADERS += application/main.h \
    application/knownfieldmodel.h \
    application/settings.h \
    gui/filefilterproxymodel.h \
    gui/tagfieldedit.h \
    gui/tagedit.h \
    gui/mainwindow.h \
    gui/pathlineedit.h \
    gui/notificationlabel.h \
    gui/renamefilesdialog.h \
    gui/javascripthighlighter.h \
    gui/picturepreviewselection.h \
    gui/notificationmodel.h \
    gui/infowidgetbase.h \
    gui/settingsdialog.h \
    renamingutility/filesystemitem.h \
    renamingutility/filesystemitemmodel.h \
    renamingutility/filteredfilesystemitemmodel.h \
    renamingutility/renamingengine.h \
    renamingutility/scriptfunctions.h \
    misc/htmlinfo.h \
    gui/previousvaluehandling.h \
    gui/initiate.h \
    cli/mainfeatures.h \
    misc/utility.h \
    gui/entertargetdialog.h \
    gui/attachmentsmodel.h \
    gui/attachmentsedit.h \
    gui/codeedit.h

SOURCES += application/main.cpp \
    application/knownfieldmodel.cpp \
    application/settings.cpp \
    gui/filefilterproxymodel.cpp \
    gui/settingsdialog.cpp \
    gui/mainwindow.cpp \
    gui/pathlineedit.cpp \
    gui/notificationlabel.cpp \
    gui/renamefilesdialog.cpp \
    gui/javascripthighlighter.cpp \
    gui/tagfieldedit.cpp \
    gui/tagedit.cpp \
    gui/picturepreviewselection.cpp \
    gui/notificationmodel.cpp \
    gui/infowidgetbase.cpp \
    renamingutility/filesystemitem.cpp \
    renamingutility/filesystemitemmodel.cpp \
    renamingutility/filteredfilesystemitemmodel.cpp \
    renamingutility/renamingengine.cpp \
    renamingutility/scriptfunctions.cpp \
    misc/htmlinfo.cpp \
    gui/previousvaluehandling.cpp \
    gui/initiate.cpp \
    cli/mainfeatures.cpp \
    misc/utility.cpp \
    gui/entertargetdialog.cpp \
    gui/attachmentsmodel.cpp \
    gui/attachmentsedit.cpp \
    gui/codeedit.cpp

FORMS += \
    gui/id3v2optionpage.ui \
    gui/id3v1optionpage.ui \
    gui/tagprocessinggeneraloptionpage.ui \
    gui/editorgeneraloptionpage.ui \
    gui/filebrowsergeneraloptionpage.ui \
    gui/mainwindow.ui \
    gui/renamefilesdialog.ui \
    gui/editorautocorrectionoptionpage.ui \
    gui/picturepreviewselection.ui \
    gui/editorfieldsoptionpage.ui \
    gui/infooptionpage.ui \
    gui/entertargetdialog.ui \
    gui/attachmentsedit.ui \
    gui/editortempoptionpage.ui \
    gui/filelayout.ui

RESOURCES += \
    resources/icons.qrc \
    resources/scripts.qrc

TRANSLATIONS = translations/tageditor_en_US.ts \
     translations/tageditor_de_DE.ts

OTHER_FILES += \
    README.md \
    LICENSE \
    CMakeLists.txt \
    resources/config.h.in \
    resources/windows.rc.in

# release translations
include(translations.pri)

# make windows icon
win32:include(windowsicon.pri)

# add libs
CONFIG(debug, debug|release) {
    LIBS += -lc++utilitiesd -ltagparserd
    !no-gui {
        LIBS += -lqtutilitiesd
    }
} else {
    LIBS += -lc++utilities -ltagparser
    !no-gui {
        LIBS += -lqtutilities
    }
}

# installs
target.path = $$(INSTALL_ROOT)/bin
INSTALLS += target
!mingw-w64-install {
    icon.path = $$(INSTALL_ROOT)/share/icons/hicolor/scalable/apps/
    icon.files = $${PWD}/resources/icons/hicolor/scalable/apps/$${projectname}.svg
    INSTALLS += icon
    menu.path = $$(INSTALL_ROOT)/share/applications/
    menu.files = $${PWD}/resources/desktop/applications/$${projectname}.desktop
    INSTALLS += menu
}
