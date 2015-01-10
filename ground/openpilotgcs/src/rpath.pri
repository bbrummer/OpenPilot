macx {
    QMAKE_LFLAGS_SONAME = -Wl,-install_name,@executable_path/../Plugins/
} else:linux-* {
    # do the rpath by hand since it's not possible to use ORIGIN in QMAKE_RPATHDIR
    # this expands to $ORIGIN (after qmake and make), it does NOT read a qmake var
    QMAKE_RPATHDIR = \$\$ORIGIN/../$$GCS_LIBRARY_BASENAME/openpilotgcs
    QMAKE_RPATHDIR += \$\$ORIGIN/../$$GCS_LIBRARY_BASENAME/qt5
    QMAKE_RPATHDIR += \$\$ORIGIN/../$$GCS_LIBRARY_BASENAME/osg

    GCS_PLUGIN_RPATH = $$join(QMAKE_RPATHDIR, ":")

    QMAKE_LFLAGS += -Wl,-z,origin \'-Wl,-rpath,$${GCS_PLUGIN_RPATH}\'
    QMAKE_RPATHDIR =
}
