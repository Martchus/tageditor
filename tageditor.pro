projectname = tageditor

# include ../../common.pri when building as part of a subdirs project; otherwise include general.pri
!include(../../common.pri) {
    !include(./general.pri) {
        error("Couldn't find the common.pri or the general.pri file!")
    }
}

TEMPLATE = app

QT += core gui script webkitwidgets
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

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
    gui/editortempoptionpage.ui

RESOURCES += resources/icons.qrc \
    resources/scripts.qrc

TRANSLATIONS = translations/tageditor_en_US.ts \
     translations/tageditor_de_DE.ts
include(translations.pri)

OTHER_FILES += \
    README.md \
    LICENSE \
    pkgbuild/default/PKGBUILD \
    pkgbuild/mingw-w64/PKGBUILD

CONFIG(debug, debug|release) {
    LIBS += -L../../ -lc++utilitiesd -ltagparserd
    !no-gui {
        LIBS += -lqtutilitiesd
    }
} else {
    LIBS += -L../../ -lc++utilities -ltagparser
    !no-gui {
        LIBS += -lqtutilities
    }
}

INCLUDEPATH += ../

win32 {
    #win32:RC_FILE += windowsicon.rc
}

# installs
target.path = $$(INSTALL_ROOT)/bin
INSTALLS += target
icon.path = $$(INSTALL_ROOT)/share/icons/hicolor/scalable/apps/
icon.files = $${PWD}/resources/icons/hicolor/scalable/apps/$${projectname}.svg
INSTALLS += icon
menu.path = $$(INSTALL_ROOT)/share/applications/
menu.files = $${PWD}/resources/desktop/applications/$${projectname}.desktop
INSTALLS += menu
translations.path = $$(INSTALL_ROOT)/share/$${projectname}/translations/
translations.files = $${OUT_PWD}/translations/*.qm
INSTALLS += translations
