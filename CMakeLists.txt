cmake_minimum_required(VERSION 3.1.0 FATAL_ERROR)

# meta data
set(META_PROJECT_NAME tageditor)
set(META_PROJECT_TYPE application)
set(META_APP_NAME "Tag Editor")
set(META_APP_CATEGORIES "AudioVideo;Database;")
set(META_APP_AUTHOR "Martchus")
set(META_APP_URL "https://github.com/${META_APP_AUTHOR}/${META_PROJECT_NAME}")
set(META_APP_DESCRIPTION
    "A tageditor with Qt GUI and command line interface. Supports MP4 (iTunes), ID3, Vorbis, Opus, FLAC and Matroska")
set(META_GUI_OPTIONAL true)
set(META_JS_SRC_DIR renamingutility)
set(META_VERSION_MAJOR 3)
set(META_VERSION_MINOR 2)
set(META_VERSION_PATCH 1)
set(META_ADD_DEFAULT_CPP_UNIT_TEST_APPLICATION ON)

# add project files
set(HEADER_FILES cli/attachmentinfo.h cli/fieldmapping.h cli/helper.h cli/mainfeatures.h application/knownfieldmodel.h)
set(SRC_FILES
    application/main.cpp
    cli/attachmentinfo.cpp
    cli/fieldmapping.cpp
    cli/helper.cpp
    cli/mainfeatures.cpp
    application/knownfieldmodel.cpp)

set(GUI_HEADER_FILES
    application/targetlevelmodel.h
    application/settings.h
    gui/fileinfomodel.h
    misc/htmlinfo.h
    misc/utility.h)
set(GUI_SRC_FILES
    application/targetlevelmodel.cpp
    application/settings.cpp
    gui/fileinfomodel.cpp
    misc/htmlinfo.cpp
    misc/utility.cpp)

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
    dbquery/dbquery.h
    dbquery/musicbrainz.h
    dbquery/lyricswikia.h
    gui/dbquerywidget.h
    misc/networkaccessmanager.h
    renamingutility/filesystemitem.h
    renamingutility/filesystemitemmodel.h
    renamingutility/filteredfilesystemitemmodel.h
    renamingutility/renamingengine.h
    renamingutility/tageditorobject.h)
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
    dbquery/dbquery.cpp
    dbquery/musicbrainz.cpp
    dbquery/lyricswikia.cpp
    gui/dbquerywidget.cpp
    misc/networkaccessmanager.cpp
    renamingutility/filesystemitem.cpp
    renamingutility/filesystemitemmodel.cpp
    renamingutility/filteredfilesystemitemmodel.cpp
    renamingutility/renamingengine.cpp
    renamingutility/tageditorobject.cpp
    resources/icons.qrc
    resources/scripts.qrc)
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
    gui/dbquerywidget.ui)

set(TEST_HEADER_FILES)
set(TEST_SRC_FILES tests/cli.cpp)

set(TS_FILES translations/${META_PROJECT_NAME}_de_DE.ts translations/${META_PROJECT_NAME}_en_US.ts)

set(ICON_FILES resources/icons/hicolor/scalable/apps/${META_PROJECT_NAME}.svg)

set(DOC_FILES README.md)

set(REQUIRED_ICONS
    window-close
    document-open
    edit-clear
    edit-copy
    emblem-error
    emblem-warning
    emblem-information
    emblem-question
    object-locked
    object-unlocked
    preferences-other
    view-preview
    document-open-recent
    tag-add
    process-stop
    edit-find
    media-playback-start
    list-add
    tag
    dialog-ok-apply
    help-about
    preferences-desktop-locale
    edit-undo
    document-properties
    image-x-generic
    unlock
    tag-properties
    edit-rename
    system-run
    go-next
    expand-menu-hover
    autocorrection
    lock
    qtcreator
    collapse-menu-hover
    edit-paste
    document-save-as
    view-list-tree
    document-edit
    edit-delete
    system-search
    document-revert
    dialog-ok
    preferences-desktop-icons
    dialog-cancel
    edit-clear
    application-exit
    tag-delete
    system-file-manager
    document-save
    view-media-lyrics)

# find c++utilities
set(CONFIGURATION_PACKAGE_SUFFIX ""
    CACHE STRING "sets the suffix for find_package() calls to packages configured via c++utilities")
find_package(c++utilities${CONFIGURATION_PACKAGE_SUFFIX} 5.0.0 REQUIRED)
use_cpp_utilities()
include(BasicConfig)

# find qtutilities
if (WIDGETS_GUI OR QUICK_GUI)
    find_package(qtutilities${CONFIGURATION_PACKAGE_SUFFIX} 6.0.0 REQUIRED)
    use_qt_utilities()
endif ()

# find tagparser
find_package(tagparser${CONFIGURATION_PACKAGE_SUFFIX} 9.0.0 REQUIRED)
use_tag_parser()

# enable experimental JSON export
option(ENABLE_JSON_EXPORT "enable JSON export" OFF)
if (ENABLE_JSON_EXPORT)
    # find reflective-rapidjson
    find_package(reflective_rapidjson${CONFIGURATION_PACKAGE_SUFFIX} REQUIRED)
    use_reflective_rapidjson()

    # add additional source files
    list(APPEND HEADER_FILES cli/json.h)
    list(APPEND SRC_FILES cli/json.cpp)

    # add generator invocation
    include(ReflectionGenerator)
    add_reflection_generator_invocation(INPUT_FILES
                                        cli/json.h
                                        GENERATORS
                                        json
                                        OUTPUT_LISTS
                                        HEADER_FILES
                                        CLANG_OPTIONS_FROM_TARGETS
                                        ${META_TARGET_NAME})

    # add compile definitions
    list(APPEND META_PRIVATE_COMPILE_DEFINITIONS ${META_PROJECT_VARNAME_UPPER}_JSON_EXPORT)
endif ()

# add Qt modules which can not be detected automatically
list(APPEND ADDITIONAL_QT_MODULES Concurrent Network)

# include modules to apply configuration
if (WIDGETS_GUI OR QUICK_GUI)
    include(QtGuiConfig)
    include(QtJsProviderConfig)
    include(QtWebViewProviderConfig)
    include(QtConfig)
endif ()
include(WindowsResources)
include(TestTarget)
include(AppTarget)
include(ShellCompletion)
include(ConfigHeader)

# create desktop file using previously defined meta data
add_desktop_file()

# create custom desktop file for launching the renaming utility separately
add_custom_desktop_file(FILE_NAME
                        "${META_ID}-renamingutility"
                        DESKTOP_FILE_APP_NAME
                        "Renaming utility (${META_APP_NAME})"
                        DESKTOP_FILE_GENERIC_NAME
                        "Renaming utility (${META_APP_NAME})"
                        DESKTOP_FILE_DESCRIPTION
                        "Tool to rename files using its tag information"
                        DESKTOP_FILE_CATEGORIES
                        "${META_APP_CATEGORIES}"
                        DESKTOP_FILE_CMD
                        "${META_TARGET_NAME} qt-gui --renaming-utility"
                        DESKTOP_FILE_ICON
                        "${META_PROJECT_NAME}")
