cmake_minimum_required(VERSION 3.16)
project(IPAssistant VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Gui Widgets Network LinguistTools)
include(CTest)

set(APP_TS_FILES
    src/translations/ipassistant_zh_CN.ts
    src/translations/ipassistant_zh_TW.ts
    src/translations/ipassistant_ja_JP.ts
    src/translations/ipassistant_ko_KR.ts
    src/translations/ipassistant_de_DE.ts
    src/translations/ipassistant_fr_FR.ts
    src/translations/ipassistant_es_ES.ts
    src/translations/ipassistant_pt_BR.ts
    src/translations/ipassistant_ru_RU.ts
    src/translations/ipassistant_it_IT.ts
    src/translations/ipassistant_tr_TR.ts
    src/translations/ipassistant_pl_PL.ts
    src/translations/ipassistant_id_ID.ts
    src/translations/ipassistant_vi_VN.ts
    src/translations/ipassistant_th_TH.ts
    src/translations/ipassistant_ar_SA.ts
    src/translations/ipassistant_hi_IN.ts
)

add_executable(IPAssistant WIN32
    src/main.cpp
    src/core/Profile.h
    src/core/Profile.cpp
    src/core/ProfileStore.h
    src/core/ProfileStore.cpp
    src/core/NetworkAdapter.h
    src/core/NetworkAdapter.cpp
    src/core/IPAssistantCore.h
    src/core/IPAssistantCore.cpp
    src/core/AutoStart.h
    src/core/AutoStart.cpp
    src/core/LocaleManager.h
    src/core/LocaleManager.cpp
    src/service/ServiceInstaller.h
    src/service/ServiceInstaller.cpp
    src/ui/MainWindow.h
    src/ui/MainWindow.cpp
    src/ui/SettingsDialog.h
    src/ui/SettingsDialog.cpp
    src/ui/AboutDialog.h
    src/ui/AboutDialog.cpp
    src/ui/ProfileEditDialog.h
    src/ui/ProfileEditDialog.cpp
    src/ui/ToggleSwitch.h
    src/ui/ToggleSwitch.cpp
    src/ui/ActiveIndicatorDelegate.h
    src/ui/ActiveIndicatorDelegate.cpp
    src/ui/StyleSheet.h
    src/ui/AboutDialog.ui
    src/ui/SettingsDialog.ui
    src/ui/ProfileEditDialog.ui
    src/ui/MainWindow.ui
)

qt_add_lrelease(
    IPAssistant
    TS_FILES ${APP_TS_FILES}
    QM_FILES_OUTPUT_VARIABLE APP_QM_FILES
)
qt_add_resources(
    IPAssistant "app_translations"
    PREFIX "/i18n"
    BASE "${CMAKE_CURRENT_BINARY_DIR}"
    FILES ${APP_QM_FILES}
)

target_include_directories(IPAssistant PRIVATE src)

target_link_libraries(IPAssistant PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Network
)

if(WIN32)
    target_link_libraries(IPAssistant PRIVATE iphlpapi ole32 oleaut32)
endif()

# Embed icon via windres (MinGW)
if(WIN32 AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/resources/app.rc")
    find_program(WINDRES windres)
    if(WINDRES)
        set(APP_RES_FILE "${CMAKE_BINARY_DIR}/app_icon.res")
        add_custom_command(
            OUTPUT "${APP_RES_FILE}"
            COMMAND ${WINDRES} -i "${CMAKE_CURRENT_SOURCE_DIR}/src/resources/app.rc"
                            -o "${APP_RES_FILE}" -O coff
            MAIN_DEPENDENCY "${CMAKE_CURRENT_SOURCE_DIR}/src/resources/app.rc"
            DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/resources/app.ico"
            COMMENT "Generating app_icon.res"
        )
        # Force .res into linker
        add_custom_target(IconResource ALL DEPENDS "${APP_RES_FILE}")
        add_dependencies(IPAssistant IconResource)
        set_property(TARGET IPAssistant APPEND_STRING PROPERTY LINK_FLAGS " \"${APP_RES_FILE}\"")
    endif()
endif()

# ====================================================================
#  Windows Service executable
# ====================================================================
if(WIN32)
    add_executable(IPAssistantService
        src/service/main.cpp
        src/service/IPAssistantService.h
        src/service/IPAssistantService.cpp
        src/service/ServiceInstaller.h
        src/service/ServiceInstaller.cpp
        src/core/Profile.h
        src/core/Profile.cpp
        src/core/NetworkAdapter.h
        src/core/NetworkAdapter.cpp
    )

    set_target_properties(IPAssistantService PROPERTIES WIN32_EXECUTABLE FALSE)

    target_include_directories(IPAssistantService PRIVATE src)

    target_link_libraries(IPAssistantService PRIVATE
        Qt6::Core
        Qt6::Network
        ole32 oleaut32 iphlpapi
    )

    if(WINDRES AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/resources/service.rc")
        set(SERVICE_RES_FILE "${CMAKE_BINARY_DIR}/service_version.res")
        add_custom_command(
            OUTPUT "${SERVICE_RES_FILE}"
            COMMAND ${WINDRES}
                    -i "${CMAKE_CURRENT_SOURCE_DIR}/src/resources/service.rc"
                    -o "${SERVICE_RES_FILE}"
                    -O coff
            MAIN_DEPENDENCY "${CMAKE_CURRENT_SOURCE_DIR}/src/resources/service.rc"
            COMMENT "Generating service_version.res"
        )
        add_custom_target(ServiceVersionResource ALL DEPENDS "${SERVICE_RES_FILE}")
        add_dependencies(IPAssistantService ServiceVersionResource)
        set_property(
            TARGET IPAssistantService
            APPEND_STRING PROPERTY LINK_FLAGS " \"${SERVICE_RES_FILE}\""
        )
    endif()
endif()

if(BUILD_TESTING)
    find_package(Qt6 6.5 REQUIRED COMPONENTS Test)

    add_executable(ProfileBindingTests
        tests/ProfileBindingTests.cpp
        src/core/Profile.h
        src/core/Profile.cpp
        src/core/NetworkAdapter.h
        src/core/NetworkAdapter.cpp
        src/core/LocaleManager.h
        src/core/LocaleManager.cpp
    )
    target_include_directories(ProfileBindingTests PRIVATE src)
    target_link_libraries(ProfileBindingTests PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Network
        Qt6::Widgets
        Qt6::Test
    )
    if(WIN32)
        target_link_libraries(ProfileBindingTests PRIVATE iphlpapi)
    endif()
    add_dependencies(ProfileBindingTests IPAssistant_lrelease)
    qt_add_resources(
        ProfileBindingTests "test_translations"
        PREFIX "/i18n"
        BASE "${CMAKE_CURRENT_BINARY_DIR}"
        FILES ${APP_QM_FILES}
    )

    add_test(NAME ProfileBindingTests COMMAND ProfileBindingTests)
endif()
