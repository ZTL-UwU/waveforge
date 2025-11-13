cmake_minimum_required(VERSION 3.26)
project(waveforge VERSION 0.1 LANGUAGES CXX)

include(FetchContent)

# SFML
FetchContent_Declare(SFML
  GIT_REPOSITORY https://github.com/SFML/SFML.git
  GIT_TAG 3.0.2
  GIT_SHALLOW ON
  EXCLUDE_FROM_ALL SYSTEM
)
option(SFML_USE_SYSTEM_DEPS ON)
FetchContent_MakeAvailable(SFML)

# msft_proxy
FetchContent_Declare(msft_proxy4
  GIT_REPOSITORY https://github.com/microsoft/proxy.git
  GIT_TAG 4.0.1
  GIT_SHALLOW ON
  EXCLUDE_FROM_ALL SYSTEM
)
FetchContent_MakeAvailable(msft_proxy4)

# cpptrace
FetchContent_Declare(
  cpptrace
  GIT_REPOSITORY https://github.com/jeremy-rifkin/cpptrace.git
  GIT_TAG        v1.0.4
  GIT_SHALLOW ON
  EXCLUDE_FROM_ALL SYSTEM
)
FetchContent_MakeAvailable(cpptrace)

# nlohmann_json
FetchContent_Declare(
	json
	URL https://github.com/nlohmann/json/releases/download/v3.12.0/json.tar.xz
	EXCLUDE_FROM_ALL SYSTEM
)
FetchContent_MakeAvailable(json)

add_executable(waveforge
  src/main.cpp
  src/fallsand/world.cpp
  src/fallsand/air.cpp
  src/fallsand/fluidflow.cpp
  src/fallsand/stone.cpp
  src/fallsand/sand.cpp
  src/fallsand/fluids.cpp
  src/level.cpp
  src/duck.cpp
  src/assets.cpp
  src/goal.cpp
  src/2d.cpp
  src/xoroshiro.cpp
  src/fallsand/thermal.cpp
)

target_compile_features(waveforge PRIVATE cxx_std_23)

target_include_directories(waveforge PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(waveforge PRIVATE
  SFML::Graphics
  SFML::Window
  SFML::System
  SFML::Audio
  msft_proxy4::proxy
  cpptrace::cpptrace
  nlohmann_json::nlohmann_json
)
