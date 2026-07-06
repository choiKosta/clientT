cmake_minimum_required(VERSION 3.21)

project(camera_client LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Network)
find_package(OpenCV REQUIRED)

add_executable(camera_client
    src/main.cpp
    src/MainWindow.cpp
    src/MainWindow.h
    src/ServerApiClient.cpp
    src/ServerApiClient.h
    src/VideoWorker.cpp
    src/VideoWorker.h
)

target_include_directories(camera_client PRIVATE src)

target_link_libraries(camera_client
    PRIVATE
        Qt6::Widgets
        Qt6::Network
        ${OpenCV_LIBS}
)