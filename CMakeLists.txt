cmake_minimum_required(VERSION 3.13)
project("LD51")

set(CMAKE_CXX_STANDARD 17)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/dependencies/tako/cmake)

add_subdirectory("dependencies/tako")
include(tako)
SET(EXECUTABLE BaseClock)
add_executable(${EXECUTABLE}
	"src/Main.cpp"
	"src/Game.hpp"
	"src/Comps.hpp"
	"src/Comps.cpp"
	"src/Player.hpp"
	"src/FrameData.hpp"
)

tako_setup(${EXECUTABLE})
target_link_libraries(${EXECUTABLE} PUBLIC tako)

tako_assets_dir("${CMAKE_CURRENT_SOURCE_DIR}/Assets")
