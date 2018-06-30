
# CMake 最低版本号要求
cmake_minimum_required (VERSION 3.0)

# 项目信息
project (Dual-Camera-Bokeh-Perview)

# 设置OpenCV所在目录
set(OpenCV_DIR "D:/Program Files/Open CV 2.4.11/opencv")

# 设置包含路径
include_directories( ${OpenCV_DIR}/build/include)
include_directories( ${OpenCV_DIR}/build/include/opencv)
include_directories( ${OpenCV_DIR}/build/include/opencv2)

# 设置链接库的搜索路径
link_directories(${OpenCV_DIR}/build/x86/vc12/lib)

# 设置库名称
set(OpenCV_LIBS opencv_core2411d.lib;opencv_imgproc2411d.lib;opencv_highgui2411d.lib;opencv_ml2411d.lib;opencv_video2411d.lib;opencv_features2d2411d.lib;opencv_calib3d2411d.lib;opencv_objdetect2411d.lib;opencv_contrib2411d.lib;opencv_legacy2411d.lib;opencv_flann2411d.lib;)

# List source files
file(GLOB SRC_FILES src/*.cpp)

# Additional include directories
include_directories(src/)

# 指定生成目标
add_executable(Bokeh ${SRC_FILES})

# 添加链接OpenCV库
target_link_libraries(Bokeh ${OpenCV_LIBS})
































