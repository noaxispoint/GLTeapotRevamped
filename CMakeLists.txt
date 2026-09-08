cmake_minimum_required(VERSION 3.16)
project(GLTeapot CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

include(FetchContent)

# --- GLFW (windowing/input/GL context) -------------------------------------
find_package(glfw3 3.3 QUIET)
if(NOT glfw3_FOUND)
  message(STATUS "System glfw3 not found, fetching GLFW 3.4 from source")
  set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
  set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
  set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
  set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
  # Default the Wayland backend off: building it requires wayland-scanner
  # (and the wayland-protocols/libxkbcommon dev packages) even on X11-only
  # machines, none of which is otherwise needed. Pass
  # -DGLFW_BUILD_WAYLAND=ON yourself if you do want it.
  set(GLFW_BUILD_WAYLAND OFF CACHE BOOL "")
  FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG 3.4
    GIT_SHALLOW TRUE
  )
  FetchContent_MakeAvailable(glfw)
endif()

# --- Dear ImGui (in-window menu bar / overlays / context menu) -------------
FetchContent_Declare(
  imgui
  GIT_REPOSITORY https://github.com/ocornut/imgui.git
  GIT_TAG v1.92.9
  GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(imgui)

add_library(imgui STATIC
  ${imgui_SOURCE_DIR}/imgui.cpp
  ${imgui_SOURCE_DIR}/imgui_draw.cpp
  ${imgui_SOURCE_DIR}/imgui_tables.cpp
  ${imgui_SOURCE_DIR}/imgui_widgets.cpp
  ${imgui_SOURCE_DIR}/imgui_demo.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl2.cpp
)
target_include_directories(imgui PUBLIC
  ${imgui_SOURCE_DIR}
  ${imgui_SOURCE_DIR}/backends
)
target_link_libraries(imgui PUBLIC glfw)

find_package(OpenGL REQUIRED)

add_executable(GLTeapot
  src/main.cpp
  src/App.cpp
  src/Scene.cpp
  src/Object.cpp
  src/TeapotGeometry.cpp
)
target_include_directories(GLTeapot PRIVATE src)
target_link_libraries(GLTeapot PRIVATE imgui glfw OpenGL::GL)
target_compile_definitions(GLTeapot PRIVATE GL_SILENCE_DEPRECATION)

if(APPLE)
  target_link_libraries(GLTeapot PRIVATE "-framework Cocoa" "-framework IOKit" "-framework CoreVideo")
endif()

install(TARGETS GLTeapot RUNTIME DESTINATION bin)
