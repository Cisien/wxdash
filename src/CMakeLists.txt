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
)

target_link_libraries(wxdash_lib PUBLIC Qt6::Core Qt6::Network)
target_include_directories(wxdash_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

qt_add_executable(wxdash main.cpp icons.qrc)

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
)

target_link_libraries(wxdash PRIVATE wxdash_lib Qt6::Quick Qt6::Svg)
