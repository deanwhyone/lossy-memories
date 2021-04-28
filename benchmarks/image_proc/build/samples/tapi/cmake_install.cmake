# Install script for directory: /home/deanwhyone/18742/sobel_benchmark/opencv/samples/tapi

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "/usr/local")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "Release")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Install shared libraries without execute permission?
if(NOT DEFINED CMAKE_INSTALL_SO_NO_EXE)
  set(CMAKE_INSTALL_SO_NO_EXE "1")
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "FALSE")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xsamplesx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/OpenCV/samples/tapi" TYPE FILE FILES
    "/home/deanwhyone/18742/sobel_benchmark/opencv/samples/tapi/bgfg_segm.cpp"
    "/home/deanwhyone/18742/sobel_benchmark/opencv/samples/tapi/camshift.cpp"
    "/home/deanwhyone/18742/sobel_benchmark/opencv/samples/tapi/clahe.cpp"
    "/home/deanwhyone/18742/sobel_benchmark/opencv/samples/tapi/dense_optical_flow.cpp"
    "/home/deanwhyone/18742/sobel_benchmark/opencv/samples/tapi/hog.cpp"
    "/home/deanwhyone/18742/sobel_benchmark/opencv/samples/tapi/opencl_custom_kernel.cpp"
    "/home/deanwhyone/18742/sobel_benchmark/opencv/samples/tapi/pyrlk_optical_flow.cpp"
    "/home/deanwhyone/18742/sobel_benchmark/opencv/samples/tapi/squares.cpp"
    "/home/deanwhyone/18742/sobel_benchmark/opencv/samples/tapi/ufacedetect.cpp"
    "/home/deanwhyone/18742/sobel_benchmark/opencv/samples/tapi/CMakeLists.txt"
    )
endif()

