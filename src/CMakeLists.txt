qt_add_library(wxdash_lib STATIC
    models/WeatherReadings.h
    models/WeatherDataModel.h
    models/WeatherDataModel.cpp
    network/JsonParser.h
    network/JsonParser.cpp
)

target_link_libraries(wxdash_lib PUBLIC Qt6::Core Qt6::Network)
target_include_directories(wxdash_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

qt_add_executable(wxdash main.cpp)
target_link_libraries(wxdash PRIVATE wxdash_lib)
