cmake_minimum_required(VERSION 2.8)
project( BlurVideo )
find_package( OpenCV REQUIRED )
include_directories( /home/david/opencv/include/opencv2 )
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
add_executable( BlurVideo BlurVideo.cpp )
target_link_libraries( BlurVideo ${OpenCV_LIBS} )

