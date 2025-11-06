cmake_minimum_required(VERSION 4.1)
project(waveforge VERSION 0.1 LANGUAGES CXX)

include(FetchContent)

# SFML
FetchContent_Declare(SFML
  GIT_REPOSITORY https://github.com/SFML/SFML.git
  GIT_TAG 3.0.2
  GIT_SHALLOW ON
  EXCLUDE_FROM_ALL
  SYSTEM
)
FetchContent_MakeAvailable(SFML)

# msft_proxy
FetchContent_Declare(msft_proxy4
  GIT_REPOSITORY https://github.com/microsoft/proxy.git
  GIT_TAG 4.0.1
  GIT_SHALLOW ON
  EXCLUDE_FROM_ALL
  SYSTEM
)
FetchContent_MakeAvailable(msft_proxy4)

include(FetchContent)
FetchContent_Declare(
  cpptrace
  GIT_REPOSITORY https://github.com/jeremy-rifkin/cpptrace.git
  GIT_TAG        v1.0.4 # <HASH or TAG>
)
FetchContent_MakeAvailable(cpptrace)

add_executable(waveforge
  src/main.cpp
  src/fallsand/world.cpp
  src/fallsand/air.cpp
  src/fallsand/fluid.cpp
  src/fallsand/stone.cpp
  src/fallsand/sand.cpp
  src/fallsand/water.cpp
  src/level.cpp
  src/assets.cpp
  src/2d.cpp
  src/render.cpp
)

target_compile_features(waveforge PRIVATE cxx_std_23)

# Include paths
target_include_directories(waveforge PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Link SFML components.
target_link_libraries(waveforge PRIVATE
  SFML::Graphics
  SFML::Window
  SFML::System
  SFML::Audio
  msft_proxy4::proxy
  cpptrace::cpptrace
)
