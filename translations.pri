!isEmpty(TRANSLATIONS) {
    isEmpty(QMAKE_LRELEASE) {
        QMAKE_LRELEASE = $$[QT_INSTALL_BINS]/lrelease
    }
    isEmpty(TS_DIR):TS_DIR = translations
    genqm.name = translations
    genqm.input = TRANSLATIONS
    genqm.output = $${OUT_PWD}/$$TS_DIR/${QMAKE_FILE_BASE}.qm
    genqm.commands = $$QMAKE_LRELEASE ${QMAKE_FILE_IN} -qm $${OUT_PWD}/$$TS_DIR/${QMAKE_FILE_BASE}.qm
    genqm.CONFIG = no_link
    QMAKE_EXTRA_COMPILERS += genqm
    PRE_TARGETDEPS += compiler_genqm_make_all
    translations.path = $$(INSTALL_ROOT)/share/$${projectname}/translations/
    translations.files = $${OUT_PWD}/translations/*.qm
    INSTALLS += translations
}
