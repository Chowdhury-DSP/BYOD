cmake_minimum_required(VERSION 3.15)
set(CMAKE_CXX_STANDARD 17)
project(MuffPi VERSION 1.0.0)

add_executable(muff_pi muff_pi.cpp)

# link to python
find_package(Python3 COMPONENTS Development NumPy)
target_include_directories(muff_pi PRIVATE ${Python3_INCLUDE_DIRS} ${Python3_NumPy_INCLUDE_DIRS})
target_link_libraries(muff_pi PRIVATE Python3::Python Python3::NumPy)

# link to WDF lib
target_include_directories(muff_pi PRIVATE ../../modules/chowdsp_utils/modules/chowdsp_dsp/WDF)

# link to Eigen
target_include_directories(muff_pi PRIVATE ../../modules/RTNeural/modules/Eigen)
