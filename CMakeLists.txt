cmake_minimum_required(VERSION 3.24)
project(ToyJitSQLExecutor)
set(CMAKE_CXX_STANDARD 17)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_BINARY_DIR})
list(APPEND CMAKE_PREFIX_PATH ${CMAKE_BINARY_DIR})
include(conan.cmake)
conan_cmake_configure(REQUIRES llvm-core/15.0.7 taocpp-pegtl/3.2.7
        GENERATORS cmake_find_package)

conan_cmake_autodetect(settings)
conan_cmake_install(PATH_OR_REFERENCE .
        BUILD missing
        REMOTE conancenter
        SETTINGS ${settings})

find_package(SDL2 REQUIRED)
find_package(LLVM REQUIRED)
find_package(pegtl REQUIRED)

add_library(imgui STATIC imgui/imgui.cpp imgui/imgui_tables.cpp
        imgui/imgui_draw.cpp imgui/imgui_widgets.cpp
        imgui/imgui_demo.cpp imgui/backends/imgui_impl_sdl2.cpp
        imgui/backends/imgui_impl_sdlrenderer2.cpp imgui/misc/cpp/imgui_stdlib.cpp)

target_include_directories(imgui PUBLIC imgui/ ${SDL2_INCLUDE_DIRS} imgui/backends)
target_link_libraries(imgui PUBLIC ${SDL2_LIBRARIES})

add_library(w_manager STATIC windows_manager.cpp)
target_link_libraries(w_manager PUBLIC imgui)

set(JIT_INIT_PATH "${CMAKE_CURRENT_SOURCE_DIR}/jit_init.ll")
set(FONT_PATH "${CMAKE_CURRENT_SOURCE_DIR}/imgui/misc/fonts/Roboto-Medium.ttf")

add_executable(toy_sql_executor main.cpp)
target_compile_definitions(toy_sql_executor PRIVATE -DJIT_INIT_PATH="${JIT_INIT_PATH}" -DFONT_PATH="${FONT_PATH}")
target_link_libraries(toy_sql_executor PRIVATE w_manager LLVM::LLVM taocpp::pegtl)
