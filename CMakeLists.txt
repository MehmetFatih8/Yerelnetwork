cmake_minimum_required(VERSION 4.3)
project(Yerelkesif)

set(CMAKE_CXX_STANDARD 20)
add_compile_definitions(ASIO_STANDALONE)

include(FetchContent)

FetchContent_Declare(
        asio
        GIT_REPOSITORY https://github.com/chriskohlhoff/asio.git
        GIT_TAG        master
)
FetchContent_MakeAvailable(asio)


add_executable(Yerelkesif main.cpp
        ServerSocket.cpp
        ServerSocket.h
        PeerConnection.cpp
        PeerConnection.h
        ClientSocket.cpp
        ClientSocket.h
        FinderServer.cpp
        FinderServer.h
        FinderClient.cpp
        FinderClient.h)

target_include_directories(Yerelkesif PRIVATE ${asio_SOURCE_DIR}/include)


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

