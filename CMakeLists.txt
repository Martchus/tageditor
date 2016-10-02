cmake_minimum_required(VERSION 3.1.0 FATAL_ERROR)

# meta data
set(META_PROJECT_NAME tageditor)
set(META_PROJECT_TYPE application)
set(META_APP_NAME "Tag Editor")
set(META_APP_CATEGORIES "Utility;Audio;Video;")
set(META_APP_AUTHOR "Martchus")
set(META_APP_URL "https://github.com/${META_APP_AUTHOR}/${META_PROJECT_NAME}")
set(META_APP_DESCRIPTION "A tageditor with Qt GUI and command line interface. Supports MP4 (iTunes), ID3, Vorbis, Opus, FLAC and Matroska")
set(META_GUI_OPTIONAL true)
set(META_VERSION_MAJOR 2)
set(META_VERSION_MINOR 0)
set(META_VERSION_PATCH 0)

# add project files
set(HEADER_FILES
    cli/mainfeatures.h
    application/knownfieldmodel.h
)
set(SRC_FILES
    application/main.cpp
    cli/mainfeatures.cpp
    application/knownfieldmodel.cpp
)

set(GUI_HEADER_FILES
    application/targetlevelmodel.h
    application/settings.h
    misc/htmlinfo.h
    misc/utility.h
)
set(GUI_SRC_FILES
    application/targetlevelmodel.cpp
    application/settings.cpp
    misc/htmlinfo.cpp
    misc/utility.cpp
)

set(WIDGETS_HEADER_FILES
    gui/entertargetdialog.h
    gui/javascripthighlighter.h
    gui/attachmentsedit.h
    gui/attachmentsmodel.h
    gui/codeedit.h
    gui/mainwindow.h
    gui/minimumemptyspinbox.h
    gui/notificationlabel.h
    gui/notificationmodel.h
    gui/pathlineedit.h
    gui/picturepreviewselection.h
    gui/filefilterproxymodel.h
    gui/initiate.h
    gui/previousvaluehandling.h
    gui/renamefilesdialog.h
    gui/settingsdialog.h
    gui/tagedit.h
    gui/tagfieldedit.h
    gui/tageditorwidget.h
    gui/fileinfomodel.h
    dbquery/dbquery.h
    gui/dbquerywidget.h
    misc/networkaccessmanager.h
    renamingutility/filesystemitem.h
    renamingutility/filesystemitemmodel.h
    renamingutility/filteredfilesystemitemmodel.h
    renamingutility/renamingengine.h
    renamingutility/scriptdefs.h
    renamingutility/tageditorobject.h
)
set(WIDGETS_SRC_FILES
    gui/attachmentsedit.cpp
    gui/attachmentsmodel.cpp
    gui/codeedit.cpp
    gui/entertargetdialog.cpp
    gui/mainwindow.cpp
    gui/minimumemptyspinbox.cpp
    gui/notificationlabel.cpp
    gui/notificationmodel.cpp
    gui/pathlineedit.cpp
    gui/picturepreviewselection.cpp
    gui/filefilterproxymodel.cpp
    gui/initiate.cpp
    gui/javascripthighlighter.cpp
    gui/renamefilesdialog.cpp
    gui/settingsdialog.cpp
    gui/tagedit.cpp
    gui/tagfieldedit.cpp
    gui/tageditorwidget.cpp
    gui/fileinfomodel.cpp
    dbquery/dbquery.cpp
    gui/dbquerywidget.cpp
    misc/networkaccessmanager.cpp
    renamingutility/filesystemitem.cpp
    renamingutility/filesystemitemmodel.cpp
    renamingutility/filteredfilesystemitemmodel.cpp
    renamingutility/renamingengine.cpp
    renamingutility/tageditorobject.cpp
    resources/icons.qrc
    resources/scripts.qrc
)
set(WIDGETS_UI_FILES
    gui/id3v2optionpage.ui
    gui/id3v1optionpage.ui
    gui/tagprocessinggeneraloptionpage.ui
    gui/tagprocessingtargetsoptionpage.ui
    gui/editorgeneraloptionpage.ui
    gui/filebrowsergeneraloptionpage.ui
    gui/mainwindow.ui
    gui/renamefilesdialog.ui
    gui/editorautocorrectionoptionpage.ui
    gui/picturepreviewselection.ui
    gui/editorfieldsoptionpage.ui
    gui/editordbqueryoptionpage.ui
    gui/infooptionpage.ui
    gui/entertargetdialog.ui
    gui/attachmentsedit.ui
    gui/editortempoptionpage.ui
    gui/filelayout.ui
    gui/tageditorwidget.ui
    gui/dbquerywidget.ui
)

#set(QUICK_HEADER_FILES
#)
#set(QUICK_SRC_FILES
#    resources/icons.qrc
#)

set(TEST_HEADER_FILES
)
set(TEST_SRC_FILES
    tests/cppunit.cpp
    tests/cli.cpp
)

set(TS_FILES
    translations/${META_PROJECT_NAME}_de_DE.ts
    translations/${META_PROJECT_NAME}_en_US.ts
)

set(ICON_FILES
    resources/icons/hicolor/scalable/apps/${META_PROJECT_NAME}.svg
)

set(DOC_FILES
    README.md
)

set(REQUIRED_ICONS
    collapse-menu-hover
    edit-paste
    window-close
    document-save-as
    document-open
    view-list-tree
    edit-copy
    preferences-other
    document-edit
    edit-delete
    document-revert
    document-open-recent
    view-preview
    dialog-ok
    tag-add
    process-stop
    edit-find
    media-playback-start
    dialog-cancel
    edit-clear
    list-add
    dialog-ok-apply
    tag
    help-about
    application-exit
    edit-undo
    document-properties
    tag-delete
    system-file-manager
    image-x-generic
    document-save
    tag-properties
    system-run
    edit-rename
    go-next
    expand-menu-hover
    autocorrection
    unlock
    lock
)

# find c++utilities
find_package(c++utilities 4.1.0 REQUIRED)
use_cpp_utilities()
include(BasicConfig)

# find qtutilities
if(WIDGETS_GUI OR QUICK_GUI)
    find_package(qtutilities 5.0.0 REQUIRED)
    use_qt_utilities()
endif()

# find tagparser
find_package(tagparser 6.0.0 REQUIRED)
use_tag_parser()
list(APPEND TEST_LIBRARIES ${TAG_PARSER_SHARED_LIB})

# add Qt modules which can currently not be detected automatically
list(APPEND ADDITIONAL_QT_MODULES Concurrent Network)

# include modules to apply configuration
if(WIDGETS_GUI OR QUICK_GUI)
    include(QtGuiConfig)
    include(JsProviderConfig)
    include(WebViewProviderConfig)
    include(QtConfig)
endif()
include(WindowsResources)
include(TestTarget)
include(AppTarget)
include(ShellCompletion)
include(ConfigHeader)

# create desktop file using previously defined meta data
add_desktop_file()

# create custom desktop file for launching the renaming utility separately
add_custom_desktop_file(
    "${META_PROJECT_NAME}-renamingutility"
    "Renaming utility (${META_APP_NAME})"
    "Tool to rename files using its tag information"
    "${META_APP_CATEGORIES}"
    "${META_PROJECT_NAME} qt-gui --renaming-utility"
    "${META_PROJECT_NAME}"
)
