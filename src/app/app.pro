TEMPLATE = app
TARGET = pegasus-fe

SOURCES += main.cpp
RESOURCES += \
    ../qmlutils/qmlutils.qrc \
    ../frontend/frontend.qrc \
    ../themes/themes.qrc \
    $${TOP_SRCDIR}/assets/assets.qrc

DEFINES *= $${COMMON_DEFINES}

greaterThan(QT_MINOR_VERSION, 10): CONFIG += qtquickcompiler


# Linking

include($${TOP_SRCDIR}/src/link_to_backend.pri)


# Translations

LOCALE_TS_FILES = $$files($${TOP_SRCDIR}/lang/pegasus_*.ts)
LOCALE_QRC_IN = $${TOP_SRCDIR}/lang/translations.qrc.in

qtPrepareTool(LRELEASE, lrelease)
locales.name = Compile translations
locales.input = LOCALE_TS_FILES
locales.output  = lang/${QMAKE_FILE_BASE}.qm
locales.commands = $$LRELEASE -removeidentical ${QMAKE_FILE_IN} -qm ${QMAKE_FILE_OUT}
locales.clean = ${QMAKE_FILE_OUT}
locales.CONFIG += no_link target_predeps

locales_qrc.name = Generate translations QRC
locales_qrc.input = LOCALE_QRC_IN
locales_qrc.output  = lang/translations.qrc
locales_qrc.commands = $$QMAKE_COPY ${QMAKE_FILE_IN} ${QMAKE_FILE_OUT}
locales_qrc.clean = ${QMAKE_FILE_OUT}
locales_qrc.CONFIG += no_link target_predeps

for(tsfile, LOCALE_TS_FILES) {
    qmfile = lang/$$basename(tsfile)
    qmfile ~= s/.ts$/.qm

    locales_qrc.depends += $$qmfile
}

QMAKE_EXTRA_COMPILERS += locales locales_qrc
RESOURCES += $${locales_qrc.output}


# Deployment

include($${TOP_SRCDIR}/src/deployment_vars.pri)

unix:!macx {
    target.path = $${INSTALL_BINDIR}

    !isEmpty(INSTALL_ICONDIR) {
        icon.files += platform/linux/pegasus-fe.png
        icon.path = $${INSTALL_ICONDIR}
        OTHER_FILES += $${icon.files}
        INSTALLS += icon
    }
    !isEmpty(INSTALL_DESKTOPDIR) {
        desktop_file.input = platform/linux/pegasus-fe.desktop.in
        desktop_file.output = $${OUT_PWD}/pegasus-fe.desktop
        OTHER_FILES += $${desktop_file.input}

        QMAKE_SUBSTITUTES += desktop_file
        desktop.files += $$desktop_file.output
        desktop.path = $${INSTALL_DESKTOPDIR}
        INSTALLS += desktop
    }
}
win32 {
    QMAKE_TARGET_PRODUCT = "pegasus-frontend"
    QMAKE_TARGET_COMPANY = "pegasus-frontend.org"
    QMAKE_TARGET_DESCRIPTION = "Pegasus emulator frontend"
    QMAKE_TARGET_COPYRIGHT = "Copyright (c) 2017-2018 Matyas Mustoha"
    RC_ICONS = platform/windows/app_icon.ico
    OTHER_FILES += $${RC_ICONS}

    target.path = $${INSTALL_BINDIR}
}
macx {
    ICON = platform/macos/pegasus-fe.icns
    QMAKE_APPLICATION_BUNDLE_NAME = Pegasus
    QMAKE_TARGET_BUNDLE_PREFIX = org.pegasus-frontend
    QMAKE_INFO_PLIST = platform/macos/Info.plist.in

    target.path = $${INSTALL_BINDIR}
}
android {
    QT += androidextras
    OTHER_FILES += \
        platform/android/AndroidManifest.xml \
        platform/android/res/drawable-hdpi/icon.png \
        platform/android/res/drawable-ldpi/icon.png \
        platform/android/res/drawable-mdpi/icon.png \
        platform/android/src/org/pegasus_frontend/android/MainActivity.java \
        platform/android/res/values/libs.xml \

    ANDROID_CFGDIR_IN = $$PWD/platform/android
    ANDROID_CFGDIR_OUT = $$OUT_PWD/android
    equals(ANDROID_CFGDIR_IN, $${ANDROID_CFGDIR_OUT}) {
        ANDROID_CFGDIR_OUT = $$OUT_PWD/android.out
    }
    ANDROID_MANIFEST_OUT = $${ANDROID_CFGDIR_OUT}/AndroidManifest.xml

    # TODO: make this cross-platform
    QMAKE_POST_LINK += \
        $${QMAKE_COPY_DIR} $$shell_path($$ANDROID_CFGDIR_IN) $$shell_path($$ANDROID_CFGDIR_OUT) && \
        sed -i s/@GIT_REVISION@/$${GIT_REVISION}/ $$shell_path($$ANDROID_MANIFEST_OUT) && \
        sed -i s/@GIT_COMMIT_CNT@/$${GIT_COMMIT_CNT}/ $$shell_path($$ANDROID_MANIFEST_OUT)

    ANDROID_PACKAGE_SOURCE_DIR = $${ANDROID_CFGDIR_OUT}
    ANDROID_EXTRA_LIBS += \
        /opt/openssl-1.0.2p_android/libcrypto.so \
        /opt/openssl-1.0.2p_android/libssl.so
}

!isEmpty(target.path): INSTALLS += target
