cmake_minimum_required(VERSION 4.3)
project(Yerelkesif)

set(CMAKE_CXX_STANDARD 20)
add_compile_definitions(ASIO_STANDALONE)

include(FetchContent)

# 1. GLFW Kütüphanesini Çek (Pencere ve OpenGL bağlamı için)
FetchContent_Declare(
        glfw
        GIT_REPOSITORY https://github.com/glfw/glfw.git
        GIT_TAG 3.3.8
)

# 2. Dear ImGui Kütüphanesini Çek
FetchContent_Declare(
        imgui
        GIT_REPOSITORY https://github.com/ocornut/imgui.git
        GIT_TAG master
)
FetchContent_Declare(
        asio
        GIT_REPOSITORY https://github.com/chriskohlhoff/asio.git
        GIT_TAG        master
)
FetchContent_MakeAvailable(glfw imgui asio)


add_library(imgui_lib STATIC
        ${imgui_SOURCE_DIR}/imgui.cpp
        ${imgui_SOURCE_DIR}/imgui_demo.cpp
        ${imgui_SOURCE_DIR}/imgui_draw.cpp
        ${imgui_SOURCE_DIR}/imgui_tables.cpp
        ${imgui_SOURCE_DIR}/imgui_widgets.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)

target_include_directories(imgui_lib PUBLIC
        ${imgui_SOURCE_DIR}
        ${imgui_SOURCE_DIR}/backends
)

target_link_libraries(imgui_lib PUBLIC glfw)

add_executable(Yerelkesif main.cpp
        ServerSocket.cpp
        ServerSocket.h
        PeerConnection.cpp
        PeerConnection.h
        ClientSocket.cpp
        ClientSocket.h)

target_include_directories(Yerelkesif PRIVATE ${asio_SOURCE_DIR}/include)

target_link_libraries(Yerelkesif PRIVATE imgui_lib glfw)

if(WIN32)
    target_link_libraries(Yerelkesif PRIVATE opengl32 gdi32)
endif()

if(WIN32)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        target_link_options(Yerelkesif PRIVATE -static-libgcc -static-libstdc++ -Wl,-Bstatic,--whole-archive -lwinpthread -Wl,-Bdynamic,--no-whole-archive)
    endif()
    add_compile_definitions(_WIN32_WINNT=0x0A00)
    target_link_libraries(Yerelkesif PRIVATE ws2_32 wsock32 ole32 shell32 shlwapi user32)
elseif(UNIX AND NOT APPLE)
    target_link_libraries(Yerelkesif PRIVATE pthread)
endif()

