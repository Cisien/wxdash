qt_add_library(wxdash_lib STATIC
    models/WeatherReadings.h
    models/WeatherDataModel.h
    models/WeatherDataModel.cpp
    network/JsonParser.h
    network/JsonParser.cpp
    network/HttpPoller.h
    network/HttpPoller.cpp
    network/UdpReceiver.h
    network/UdpReceiver.cpp
    network/PurpleAirPoller.h
    network/PurpleAirPoller.cpp
    network/NwsPoller.h
    network/NwsPoller.cpp
)

target_link_libraries(wxdash_lib PUBLIC Qt6::Core Qt6::Network)
target_include_directories(wxdash_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

qt_add_executable(wxdash main.cpp)

qt_add_resources(wxdash "icons"
    PREFIX "/icons"
    BASE "${CMAKE_CURRENT_SOURCE_DIR}/../assets"
    FILES "${CMAKE_CURRENT_SOURCE_DIR}/../assets/wxdash.svg"
)

qt_add_resources(wxdash "weather_icons"
    PREFIX "/icons/weather"
    BASE "${CMAKE_CURRENT_SOURCE_DIR}/../assets/icons/weather"
    FILES
        "${CMAKE_CURRENT_SOURCE_DIR}/../assets/icons/weather/sun.svg"
        "${CMAKE_CURRENT_SOURCE_DIR}/../assets/icons/weather/sun_cloud.svg"
        "${CMAKE_CURRENT_SOURCE_DIR}/../assets/icons/weather/partly_cloudy.svg"
        "${CMAKE_CURRENT_SOURCE_DIR}/../assets/icons/weather/mostly_cloudy.svg"
        "${CMAKE_CURRENT_SOURCE_DIR}/../assets/icons/weather/cloudy.svg"
        "${CMAKE_CURRENT_SOURCE_DIR}/../assets/icons/weather/rain.svg"
        "${CMAKE_CURRENT_SOURCE_DIR}/../assets/icons/weather/drizzle.svg"
        "${CMAKE_CURRENT_SOURCE_DIR}/../assets/icons/weather/rain_showers.svg"
        "${CMAKE_CURRENT_SOURCE_DIR}/../assets/icons/weather/snow.svg"
        "${CMAKE_CURRENT_SOURCE_DIR}/../assets/icons/weather/blizzard.svg"
        "${CMAKE_CURRENT_SOURCE_DIR}/../assets/icons/weather/rain_snow.svg"
        "${CMAKE_CURRENT_SOURCE_DIR}/../assets/icons/weather/freezing_rain.svg"
        "${CMAKE_CURRENT_SOURCE_DIR}/../assets/icons/weather/sleet.svg"
        "${CMAKE_CURRENT_SOURCE_DIR}/../assets/icons/weather/thunderstorm.svg"
        "${CMAKE_CURRENT_SOURCE_DIR}/../assets/icons/weather/windy.svg"
        "${CMAKE_CURRENT_SOURCE_DIR}/../assets/icons/weather/fog.svg"
        "${CMAKE_CURRENT_SOURCE_DIR}/../assets/icons/weather/unknown.svg"
)

qt_policy(SET QTP0004 NEW)
qt_add_qml_module(wxdash
    URI wxdash
    RESOURCE_PREFIX /qt/qml
    OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/qml/wxdash
    QML_FILES
        qml/Main.qml
        qml/ArcGauge.qml
        qml/DashboardGrid.qml
        qml/CompassRose.qml
        qml/ReservedCell.qml
        qml/AqiGauge.qml
        qml/RainGauge.qml
        qml/ForecastPanel.qml
)

target_link_libraries(wxdash PRIVATE wxdash_lib Qt6::Quick Qt6::Svg)

# --- Install rules ---
include(GNUInstallDirs)

# Install the wxdash executable
install(TARGETS wxdash
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    COMPONENT kiosk
)

# Install the wxdash QML module (Main.qml, ArcGauge.qml, etc.)
# Manually install from the build output directory to avoid qt_generate_deploy_qml_app_script
# RPATH-patching failures on Linux desktop development systems (kimg_ani.so lacks RPATH entry).
# On the Pi the Qt packages provide their own runtime libraries; only the user QML module is needed.
install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/qml/wxdash/
    DESTINATION ${CMAKE_INSTALL_BINDIR}/../qml/wxdash
    COMPONENT kiosk
    FILES_MATCHING
        PATTERN "*.qml"
        PATTERN "qmldir"
        PATTERN "*.qmltypes"
)
