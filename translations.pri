!isEmpty(TRANSLATIONS) {
    isEmpty(lreleasepath) {
        lreleasepath = $$[QT_HOST_BINS]/lrelease
    }
    genqm.name = translations
    genqm.input = TRANSLATIONS
    genqm.output = $${OUT_PWD}/translations/${QMAKE_FILE_BASE}.qm
    genqm.commands = $$lreleasepath ${QMAKE_FILE_IN} -qm $${OUT_PWD}/translations/${QMAKE_FILE_BASE}.qm
    genqm.CONFIG = no_link
    QMAKE_EXTRA_COMPILERS += genqm
    PRE_TARGETDEPS += compiler_genqm_make_all
    translations.path = $$(INSTALL_ROOT)/share/$${projectname}/translations/
    translations.extra = install -m644 -D $${OUT_PWD}/translations/*.qm $$(INSTALL_ROOT)/share/$${projectname}/translations/
    INSTALLS += translations
}
