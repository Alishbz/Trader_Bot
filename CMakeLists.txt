cmake_minimum_required(VERSION 3.5)

project(trader_bot_v2 VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Gui Network Widgets 3dcore 3drender 3dinput 3dextras LinguistTools WebSockets Charts)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Gui Network Widgets 3dcore 3drender 3dinput 3dextras LinguistTools WebSockets Charts)

set(PROJECT_SOURCES
        main.cpp
        main_window.cpp
        main_window.h
        main_window.ui
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(trader_bot_v2
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
        asos_root/logger_page.h
        asos_root/account/account_interface.h asos_root/account/coin_list.h
        asos_root/charts/candle_graph.h asos_root/charts/line_graph.h
        asos_root/connections/binance_rest_api.h asos_root/connections/binance_web_socket.h
        asos_root/strategies/trade_strategy_interface.h
        asos_root/strategies/ex/simple_mx_strategy.h
        resource.qrc
        TODO
        asos_root/connections/rest_api_structs.h
        asos_root/generals/ohlc.h

        asos_root/history_page.h
        asos_root/fake_account_page.h
        asos_root/binance_account_page.h
        asos_root/custom_widgets/as_progressbar.h
        asos_root/custom_widgets/as_candle_chart.h
        asos_root/custom_widgets/as_date_selector.h
        asos_root/strategies/ex/asos_high_risk_strategy.h asos_root/strategies/ex/asos_low_risk_strategy.h
        asos_root/strategies/ex/template_strategy.h
        asos_root/strategies/ex/chatgpt_strategy.h
        asos_root/strategies/ex/fido_strategy.h
        asos_root/strategies/ex/simple_mx_pro_strategy.h
        asos_root/strategies/ex/chatgpt2_strategy.h
        asos_root/strategies/ex/fire_strategy.h
        asos_root/strategies/ex/fax_strategy.h
        asos_root/strategies/ex/fidorsi_strategy.h
        asos_root/strategies/ex/macd_strategy.h
        asos_root/account/configs.h
        asos_root/strategies/ex/fxal_strategy.h
        asos_root/strategies/ex/sf_strategy.h
        asos_root/strategies/ex/aaxe_long_st.h asos_root/strategies/ex/aaxe_mid_st.h asos_root/strategies/ex/aaxe_short_st.h

    )
# Define target properties for Android with Qt 6 as:
#    set_property(TARGET trader_bot_v2 APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
#                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
# For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation
else()
    if(ANDROID)
        add_library(trader_bot_v2 SHARED
            ${PROJECT_SOURCES}
        )
# Define properties for Android with Qt 5 after find_package() calls as:
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    else()
        add_executable(trader_bot_v2
            ${PROJECT_SOURCES}
        )
    endif()
endif()

target_link_libraries(trader_bot_v2 PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Network
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::3DCore
    Qt${QT_VERSION_MAJOR}::3DRender
    Qt${QT_VERSION_MAJOR}::3DInput
    Qt${QT_VERSION_MAJOR}::3DExtras
    Qt${QT_VERSION_MAJOR}::WebSockets
    Qt${QT_VERSION_MAJOR}::Charts
)

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
if(${QT_VERSION} VERSION_LESS 6.1.0)
  set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.trader_bot_v2)
endif()
set_target_properties(trader_bot_v2 PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS trader_bot_v2
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(trader_bot_v2)
endif()
