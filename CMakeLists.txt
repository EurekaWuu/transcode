cmake_minimum_required(VERSION 3.10)
project(VideoTranscoder)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(FFMPEG_SO ${CMAKE_SOURCE_DIR}/libffmpeg-whp.so)

set(OpenGL_GL_PREFERENCE GLVND)

find_package(OpenGL REQUIRED)
find_package(glfw3 REQUIRED)
find_package(GLEW REQUIRED)

set(SOURCES
    transcode.cpp
    demuxer.cpp
    queue.cpp
    video_decoder.cpp
    audio_decoder.cpp
    video_encoder.cpp
    gl.cpp
    muxer.cpp
)

add_executable(transcode ${SOURCES})

target_link_libraries(transcode 
    ${FFMPEG_SO}
    pthread
    OpenGL::GL
    glfw
    GLEW::GLEW
    ${CMAKE_DL_LIBS}
)

install(TARGETS transcode DESTINATION bin)