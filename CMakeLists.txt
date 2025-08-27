cmake_minimum_required(VERSION 3.13)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(Weed-Spotter CXX)
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBCAMERA REQUIRED libcamera)
pkg_check_modules(JPEG REQUIRED libjpeg)

#TODO remove
set(CMAKE_BUILD_TYPE Debug)

add_executable(Weed-Spotter
	src/main.cpp
	src/cameraaccess.cpp
	src/cameraimage.cpp
)

target_include_directories(Weed-Spotter  PRIVATE
	${CMAKE_CURRENT_LIST_DIR}/src
	${JPEG_INCLUDE_DIRS}
	${LIBCAMERA_INCLUDE_DIRS}
)

target_link_libraries(Weed-Spotter  PRIVATE
	${LIBCAMERA_LIBRARIES}
	${JPEG_LIBRARIES}
)

target_compile_options(Weed-Spotter PRIVATE ${LIBCAMERA_CFLAGS_OTHER})
