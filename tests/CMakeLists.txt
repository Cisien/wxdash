find_package(Qt6 REQUIRED COMPONENTS Core Test)

qt_add_executable(tst_JsonParser tst_JsonParser.cpp)
target_link_libraries(tst_JsonParser PRIVATE wxdash_lib Qt6::Test)
add_test(NAME tst_JsonParser COMMAND tst_JsonParser)

qt_add_executable(tst_WeatherDataModel tst_WeatherDataModel.cpp)
target_link_libraries(tst_WeatherDataModel PRIVATE wxdash_lib Qt6::Test)
add_test(NAME tst_WeatherDataModel COMMAND tst_WeatherDataModel)
