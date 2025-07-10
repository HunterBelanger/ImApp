cmake_minimum_required(VERSION 3.11)

project(ImApp
  VERSION "0.1.0"
  DESCRIPTION "Library to make GUIs with Dear ImGui."
  LANGUAGES CXX
)

include(CMakePackageConfigHelpers)
include(FetchContent)

# Options
option(IMAPP_INSTALL "Install the ImApp library and header files. Default value is OFF." OFF)
option(IMAPP_USE_ZLIB "Use ZLIB for image compression. Default value is OFF." OFF)
option(IMAPP_BUILD_DEMO "Build demonstration application. Default value is OFF." OFF)

# Get GLFW, to create window for us, etc.
message(STATUS "Downloading GLFW v3.3.10")
FetchContent_Declare(glfw
  GIT_REPOSITORY https://github.com/glfw/glfw
  GIT_TAG        3.3.10
)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "Build the GLFW example programs")
set(GLFW_BUILD_TESTS OFF CACHE BOOL "Build the GLFW test programs")
set(GLFW_BUILD_DOCS OFF CACHE BOOL "Build the GLFW documentation")
set(GLFW_INSTALL OFF CACHE BOOL "Generate installation target")
FetchContent_MakeAvailable(glfw)

# Find OpenGL. This is required to render things to the screen.
find_package(OpenGL REQUIRED)

# Define library
add_library(ImApp STATIC ${CMAKE_CURRENT_SOURCE_DIR}/src/imapp.cpp
                         ${CMAKE_CURRENT_SOURCE_DIR}/src/imgui/imgui.cpp
                         ${CMAKE_CURRENT_SOURCE_DIR}/src/imgui/imgui_demo.cpp
                         ${CMAKE_CURRENT_SOURCE_DIR}/src/imgui/imgui_draw.cpp
                         ${CMAKE_CURRENT_SOURCE_DIR}/src/imgui/imgui_impl_glfw.cpp
                         ${CMAKE_CURRENT_SOURCE_DIR}/src/imgui/imgui_impl_opengl3.cpp
                         ${CMAKE_CURRENT_SOURCE_DIR}/src/imgui/imgui_tables.cpp
                         ${CMAKE_CURRENT_SOURCE_DIR}/src/imgui/imgui_widgets.cpp
                         ${CMAKE_CURRENT_SOURCE_DIR}/src/imgui/implot.cpp
                         ${CMAKE_CURRENT_SOURCE_DIR}/src/imgui/implot_items.cpp
                         ${CMAKE_CURRENT_SOURCE_DIR}/src/imgui/implot_demo.cpp)

# Add alias to make more friendly with FetchConent
add_library(ImApp::ImApp ALIAS ImApp)

# Add public interface includes directory
target_include_directories(ImApp PUBLIC 
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/ImApp>
  $<INSTALL_INTERFACE:include>
)

# Link with GLFW and OpenGL
target_link_libraries(ImApp PRIVATE glfw OpenGL::GL)

# Require C++17
target_compile_features(ImApp PUBLIC cxx_std_17)

# Compile position independent code to facilitate use in shared libraries
set_target_properties(ImApp PROPERTIES POSITION_INDEPENDENT_CODE TRUE)

if (IMAPP_USE_ZLIB)
  # Find Zlib. This is an optional dependency, for better compression
  # when writing image files.
  find_package(ZLIB REQUIRED)

  # If we have Zlib available, use it for PNG compression
  target_link_libraries(ImApp PRIVATE ZLIB)
  target_compile_definitions(ImApp PRIVATE IMAPP_USE_ZLIB)
endif()

if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC") # Comile options for Windows
  target_compile_options(ImApp PRIVATE /W4)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU") # Compile options for GCC
  target_compile_options(ImApp PRIVATE -W -Wall -Wextra -Wpedantic)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang") # Compile options for Clang
  target_compile_options(ImApp PRIVATE -W -Wall -Wextra -Wpedantic)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Intel") # Compile options for Intel
  target_compile_options(ImApp PRIVATE -W -Wall -Wextra -Wpedantic)
endif()

if(IMAPP_BUILD_DEMO)
  message(STATUS "Building ImApp demo application")
  add_executable(demo ${CMAKE_CURRENT_SOURCE_DIR}/src/demo.cpp)
  target_link_libraries(demo ImApp)
endif()

# Install ImApp
if(IMAPP_INSTALL)
  configure_package_config_file(${CMAKE_CURRENT_SOURCE_DIR}/ImAppConfig.cmake.in
    "${CMAKE_CURRENT_BINARY_DIR}/ImAppConfig.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/ImApp
  )

  write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/ImAppConfigVersion.cmake"
    VERSION "${ImApp_VERSION}"
    COMPATIBILITY SameMinorVersion
  )

  install(TARGETS ImApp
          EXPORT ImAppTargets
          LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
          ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
          RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
          INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  )

  install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/ImApp DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

  install(EXPORT ImAppTargets
          FILE ImAppTargets.cmake
          NAMESPACE ImApp::
          DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/ImApp
  )

  install(FILES
            "${CMAKE_CURRENT_BINARY_DIR}/ImAppConfig.cmake"
            "${CMAKE_CURRENT_BINARY_DIR}/ImAppConfigVersion.cmake"
            DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/ImApp
  )
endif()
