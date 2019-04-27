cmake_minimum_required(VERSION 3.13)
project(ffmpeg_test)

set(CMAKE_CXX_STANDARD 14)

if(APPLE)
    add_definitions(-DMACOS)
    include_directories(/usr/local/include/)
    link_directories(/usr/local/lib/)

    include_directories(/usr/local/Cellar/sdl2/2.0.9/include/)
    link_directories(/usr/local/Cellar/sdl2/2.0.9/lib)
elseif(UNIX)
    add_definitions(-DUNIX)

endif()


add_executable(ffmpeg_test main.cpp UserWindow.cpp UserWindow.h DeviceInput.cpp DeviceInput.h FileOutput.cpp FileOutput.h Upload.cpp Upload.h)

target_link_libraries(
        ffmpeg_test
        avcodec
        avdevice
        avfilter
        avformat
        avutil
        postproc
        swresample
        swscale
        SDL2
)

