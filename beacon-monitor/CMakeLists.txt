cmake_minimum_required (VERSION 2.8.12)

project (BeaconMonitorApp LANGUAGES CXX)

find_package(wxWidgets REQUIRED COMPONENTS net core base)
include(${wxWidgets_USE_FILE})

SET (radio_app_sources 
    ${radio_app_sources}
    src/BeaconMonitorApp.cpp
    src/RadioFrame.cpp
    src/WorldMapMercator.cpp
    src/BeaconMonitorApp.rc
     )
        
add_executable(BeaconMonitorApp ${radio_app_sources})
target_link_libraries(BeaconMonitorApp ${wxWidgets_LIBRARIES})


  