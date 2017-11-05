#cmake_minimum_required(VERSION 3.8)
cmake_minimum_required( VERSION 3.0 FATAL_ERROR )
project(depth_to_mesh)

#set(CMAKE_CXX_STANDARD 11)

#set(SOURCE_FILES main.cpp)
#add_executable(depth_to_mesh ${SOURCE_FILES})


get_filename_component( CINDER_PATH "/home/default/Documents/Dev/Cinder/Cinder" ABSOLUTE )
get_filename_component( APP_PATH "${CMAKE_CURRENT_SOURCE_DIR}/" ABSOLUTE )

include( "${CINDER_PATH}/proj/cmake/modules/cinderMakeApp.cmake" )

ci_make_app(
        SOURCES     ${APP_PATH}/CinderKinectMesh.cpp
        CINDER_PATH ${CINDER_PATH}
)