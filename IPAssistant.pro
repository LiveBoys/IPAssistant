QT += core gui widgets network

TEMPLATE = app
TARGET = IPAssistant
VERSION = 1.0.0

CONFIG += c++17 warn_on lrelease
CONFIG -= console
CONFIG += windows

INCLUDEPATH += src

CONFIG(debug, debug|release) {
    DESTDIR = $$OUT_PWD/debug
    QM_BUILD_DIR = $$OUT_PWD/translations/debug
} else {
    DESTDIR = $$OUT_PWD/release
    QM_BUILD_DIR = $$OUT_PWD/translations/release
}

# qmake appends the active debug/release directory to this relative path.
LRELEASE_DIR = translations

win32 {
    LIBS += -liphlpapi -lws2_32 -lole32 -loleaut32
    RC_FILE = src/resources/app.rc
    # LocaleManager loads external translations beside the executable.
    QMAKE_POST_LINK += $$QMAKE_COPY_DIR \
        $$shell_path($$QM_BUILD_DIR) \
        $$shell_path($$DESTDIR/translations)
}

SOURCES += \
    src/main.cpp \
    src/core/AutoStart.cpp \
    src/core/IPAssistantCore.cpp \
    src/core/LocaleManager.cpp \
    src/core/NetworkAdapter.cpp \
    src/core/Profile.cpp \
    src/core/ProfileStore.cpp \
    src/service/ServiceInstaller.cpp \
    src/ui/AboutDialog.cpp \
    src/ui/ActiveIndicatorDelegate.cpp \
    src/ui/MainWindow.cpp \
    src/ui/ProfileEditDialog.cpp \
    src/ui/SettingsDialog.cpp \
    src/ui/ToggleSwitch.cpp

HEADERS += \
    src/core/AutoStart.h \
    src/core/IPAssistantCore.h \
    src/core/LocaleManager.h \
    src/core/NetworkAdapter.h \
    src/core/Profile.h \
    src/core/ProfileStore.h \
    src/service/ServiceInstaller.h \
    src/ui/AboutDialog.h \
    src/ui/ActiveIndicatorDelegate.h \
    src/ui/MainWindow.h \
    src/ui/ProfileEditDialog.h \
    src/ui/SettingsDialog.h \
    src/ui/StyleSheet.h \
    src/ui/ToggleSwitch.h

FORMS += \
    src/ui/AboutDialog.ui \
    src/ui/MainWindow.ui \
    src/ui/ProfileEditDialog.ui \
    src/ui/SettingsDialog.ui

TRANSLATIONS += \
    src/translations/ipassistant_ar_SA.ts \
    src/translations/ipassistant_de_DE.ts \
    src/translations/ipassistant_es_ES.ts \
    src/translations/ipassistant_fr_FR.ts \
    src/translations/ipassistant_hi_IN.ts \
    src/translations/ipassistant_id_ID.ts \
    src/translations/ipassistant_it_IT.ts \
    src/translations/ipassistant_ja_JP.ts \
    src/translations/ipassistant_ko_KR.ts \
    src/translations/ipassistant_pl_PL.ts \
    src/translations/ipassistant_pt_BR.ts \
    src/translations/ipassistant_ru_RU.ts \
    src/translations/ipassistant_th_TH.ts \
    src/translations/ipassistant_tr_TR.ts \
    src/translations/ipassistant_vi_VN.ts \
    src/translations/ipassistant_zh_CN.ts \
    src/translations/ipassistant_zh_TW.ts
