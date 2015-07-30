ffmpegpath = $$(FFMPEG_PATH)
equals(ffmpegpath, "") {
    exists(/usr/bin/ffmpeg) {
        ffmpegpath = /usr/bin/ffmpeg
    } else {
        packagesExist(/usr/bin/avconv) {
            ffmpegpath = /usr/bin/avconv
        }
    }
}
!equals(ffmpegpath, "") {
    WINDOWS_ICONS = resources/icons/hicolor/128x128/apps/$${projectname}.png
    windowsicon.name = windowsicon
    windowsicon.input = WINDOWS_ICONS
    windowsicon.output = $${OUT_PWD}/$$TS_DIR/${QMAKE_FILE_BASE}.qm
    windowsicon.commands = $${ffmpegpath} -y -i ${QMAKE_FILE_IN} -vf crop=iw-20:ih-20:10:10,scale=64:64 "$${OUT_PWD}/res/windowsicon_${QMAKE_FILE_BASE}.ico"
    windowsicon.CONFIG = no_link
    QMAKE_EXTRA_COMPILERS += windowsicon
    PRE_TARGETDEPS += compiler_windowsicon_make_all
    RC_ICONS = $${OUT_PWD}/res/windowsicon_$${projectname}.ico
} else {
    warning("Unfortunately ffmpeg could not be found and hence windows application icons can not be generated. The path of ffmpeg can be specified using the FFMPEG_PATH environment variable.")
}
