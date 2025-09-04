cmake_minimum_required(VERSION 3.13)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(Weed-Spotter CXX)
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBCAMERA REQUIRED libcamera)
pkg_check_modules(JPEG REQUIRED libjpeg)
pkg_check_modules(GSTREAMER REQUIRED gstreamer-1.0)
pkg_check_modules(GSTREAMER_RTSP REQUIRED gstreamer-rtsp-server-1.0)
find_package(civetweb CONFIG REQUIRED)

add_executable(Weed-Spotter
	src/main.cpp
	src/cameraaccess.cpp
	src/cameraimage.cpp
	src/streamer.cpp
	src/httpserver.cpp
	src/picoi2c.cpp
)


target_include_directories(Weed-Spotter  PRIVATE
	${CMAKE_CURRENT_LIST_DIR}/src
	${CMAKE_CURRENT_LIST_DIR}/lib
	${JPEG_INCLUDE_DIRS}
	${LIBCAMERA_INCLUDE_DIRS}
	${GSTREAMER_INCLUDE_DIRS}
	${GSTREAMER_RTSP_INCLUDE_DIRS}
)

target_link_libraries(Weed-Spotter  PRIVATE
	${LIBCAMERA_LIBRARIES}
	${JPEG_LIBRARIES}
	${GSTREAMER_LIBRARIES}
	${GSTREAMER_RTSP_LIBRARIES}
	civetweb::civetweb-cpp
)

target_compile_options(Weed-Spotter PRIVATE
	${LIBCAMERA_CFLAGS_OTHER}
)
