# Install script for directory: /home/deanwhyone/18742/sobel_benchmark/opencv/samples/cpp

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
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/OpenCV/samples/cpp" TYPE FILE FILES
    "/home/deanwhyone/18742/sobel_benchmark/opencv/samples/cpp/3calibration.cpp"
    "/home/deanwhyone/18742/sobel_benchmark/opencv/samples/cpp/application_trace.cpp"
    "/home/deanwhyone/18742/sobel_benchmark/opencv/samples/cpp/asift.cpp"
    "/home/deanwhyone/18742/sobel_benchmark/opencv/samples/cpp/bgfg_segm.cpp"
    "/home/deanwhyone/18742/sobel_benchmark/opencv/samples/cpp/calibration.cpp"
    "/home/deanwhyone/18742/sobel_benchmark/opencv/samples/cpp/camshiftdemo.cpp"
    "/home/deanwhyone/18742/sobel_benchmark/opencv/samples/cpp/cloning_demo.cpp"
    "/home/deanwhyone/18742/sobel_benchmark/opencv/samples/cpp/cloning_gui.cpp"
    "/home/deanwhyone/18742/sobel_benchmark/opencv/samples/cpp/connected_components.cpp"
    "/home/deanwhyone/18742/sobel_benchmark/opencv/samples/cpp/contours2.cpp"
    "/home/deanwhyone/18742/sobel_benchmark/opencv/samples/cpp/convexhull.cpp"
    "/home/deanwhyone/18742/sobel_benchmark/opencv/samples/cpp/cout_mat.cpp"
    "/home/deanwhyone/18742/sobel_benchmark/opencv/samples/cpp/create_mask.cpp"
    "/home/deanwhyone/18742/sobel_benchmark/opencv/samples/cpp/dbt_face_detection.cpp"
    "/home/deanwhyone/18742/sobel_benchmark/opencv/samples/cpp/delaunay2.cpp"
    "/home/deanwhyone/18742/sobel_benchmark/opencv/samples/cpp/demhist.cpp"
    "/home/deanwhyone/18742/sobel_benchmark/opencv/samples/cpp/detect_blob.cpp"
    "/home/deanwhyone/18742/sobel_benchmark/opencv/samples/cpp/detect_mser.cpp"
    "/home/deanwhyone/18742/sobel_benchmark/opencv/samples/cpp/dft.cpp"
    "/home/deanwhyone/18742/sobel_benchmark/opencv/samples/cpp/digits.cpp"
    "/home/deanwhyone/18742/sobel_benchmark/opencv/samples/cpp/distrans.cpp"
    "/home/deanwhyone/18742/sobel_benchmark/opencv/samples/cpp/drawing.cpp"
    "/home/deanwhyone/18742/sobel_benchmark/opencv/samples/cpp/edge.cpp"
    "/home/deanwhyone/18742/sobel_benchmark/opencv/samples/cpp/em.cpp"
    "/home/deanwhyone/18742/sobel_benchmark/opencv/samples/cpp/facedetect.cpp"
    "/home/deanwhyone/18742/sobel_benchmark/opencv/samples/cpp/facial_features.cpp"
    "/home/deanwhyone/18742/sobel_benchmark/opencv/samples/cpp/falsecolor.cpp"
    "/home/deanwhyone/18742/sobel_benchmark/opencv/samples/cpp/fback.cpp"
    "/home/deanwhyone/18742/sobel_benchmark/opencv/samples/cpp/ffilldemo.cpp"
    "/home/deanwhyone/18742/sobel_benchmark/opencv/samples/cpp/filestorage.cpp"
    "/home/deanwhyone/18742/sobel_benchmark/opencv/samples/cpp/fitellipse.cpp"
    "/home/deanwhyone/18742/sobel_benchmark/opencv/samples/cpp/flann_search_dataset.cpp"
    "/home/deanwhyone/18742/sobel_benchmark/opencv/samples/cpp/grabcut.cpp"
    "/home/deanwhyone/18742/sobel_benchmark/opencv/samples/cpp/image.cpp"
    "/home/deanwhyone/18742/sobel_benchmark/opencv/samples/cpp/image_alignment.cpp"
    "/home/deanwhyone/18742/sobel_benchmark/opencv/samples/cpp/imagelist_creator.cpp"
    "/home/deanwhyone/18742/sobel_benchmark/opencv/samples/cpp/imagelist_reader.cpp"
    "/home/deanwhyone/18742/sobel_benchmark/opencv/samples/cpp/inpaint.cpp"
    "/home/deanwhyone/18742/sobel_benchmark/opencv/samples/cpp/intelligent_scissors.cpp"
    "/home/deanwhyone/18742/sobel_benchmark/opencv/samples/cpp/intersectExample.cpp"
    "/home/deanwhyone/18742/sobel_benchmark/opencv/samples/cpp/kalman.cpp"
    "/home/deanwhyone/18742/sobel_benchmark/opencv/samples/cpp/kmeans.cpp"
    "/home/deanwhyone/18742/sobel_benchmark/opencv/samples/cpp/laplace.cpp"
    "/home/deanwhyone/18742/sobel_benchmark/opencv/samples/cpp/letter_recog.cpp"
    "/home/deanwhyone/18742/sobel_benchmark/opencv/samples/cpp/lkdemo.cpp"
    "/home/deanwhyone/18742/sobel_benchmark/opencv/samples/cpp/logistic_regression.cpp"
    "/home/deanwhyone/18742/sobel_benchmark/opencv/samples/cpp/mask_tmpl.cpp"
    "/home/deanwhyone/18742/sobel_benchmark/opencv/samples/cpp/matchmethod_orb_akaze_brisk.cpp"
    "/home/deanwhyone/18742/sobel_benchmark/opencv/samples/cpp/minarea.cpp"
    "/home/deanwhyone/18742/sobel_benchmark/opencv/samples/cpp/morphology2.cpp"
    "/home/deanwhyone/18742/sobel_benchmark/opencv/samples/cpp/neural_network.cpp"
    "/home/deanwhyone/18742/sobel_benchmark/opencv/samples/cpp/npr_demo.cpp"
    "/home/deanwhyone/18742/sobel_benchmark/opencv/samples/cpp/opencv_version.cpp"
    "/home/deanwhyone/18742/sobel_benchmark/opencv/samples/cpp/pca.cpp"
    "/home/deanwhyone/18742/sobel_benchmark/opencv/samples/cpp/peopledetect.cpp"
    "/home/deanwhyone/18742/sobel_benchmark/opencv/samples/cpp/phase_corr.cpp"
    "/home/deanwhyone/18742/sobel_benchmark/opencv/samples/cpp/points_classifier.cpp"
    "/home/deanwhyone/18742/sobel_benchmark/opencv/samples/cpp/polar_transforms.cpp"
    "/home/deanwhyone/18742/sobel_benchmark/opencv/samples/cpp/qrcode.cpp"
    "/home/deanwhyone/18742/sobel_benchmark/opencv/samples/cpp/segment_objects.cpp"
    "/home/deanwhyone/18742/sobel_benchmark/opencv/samples/cpp/select3dobj.cpp"
    "/home/deanwhyone/18742/sobel_benchmark/opencv/samples/cpp/shape_example.cpp"
    "/home/deanwhyone/18742/sobel_benchmark/opencv/samples/cpp/simd_basic.cpp"
    "/home/deanwhyone/18742/sobel_benchmark/opencv/samples/cpp/smiledetect.cpp"
    "/home/deanwhyone/18742/sobel_benchmark/opencv/samples/cpp/squares.cpp"
    "/home/deanwhyone/18742/sobel_benchmark/opencv/samples/cpp/stereo_calib.cpp"
    "/home/deanwhyone/18742/sobel_benchmark/opencv/samples/cpp/stereo_match.cpp"
    "/home/deanwhyone/18742/sobel_benchmark/opencv/samples/cpp/stitching.cpp"
    "/home/deanwhyone/18742/sobel_benchmark/opencv/samples/cpp/stitching_detailed.cpp"
    "/home/deanwhyone/18742/sobel_benchmark/opencv/samples/cpp/text_skewness_correction.cpp"
    "/home/deanwhyone/18742/sobel_benchmark/opencv/samples/cpp/train_HOG.cpp"
    "/home/deanwhyone/18742/sobel_benchmark/opencv/samples/cpp/train_svmsgd.cpp"
    "/home/deanwhyone/18742/sobel_benchmark/opencv/samples/cpp/travelsalesman.cpp"
    "/home/deanwhyone/18742/sobel_benchmark/opencv/samples/cpp/tree_engine.cpp"
    "/home/deanwhyone/18742/sobel_benchmark/opencv/samples/cpp/tvl1_optical_flow.cpp"
    "/home/deanwhyone/18742/sobel_benchmark/opencv/samples/cpp/videocapture_basic.cpp"
    "/home/deanwhyone/18742/sobel_benchmark/opencv/samples/cpp/videocapture_camera.cpp"
    "/home/deanwhyone/18742/sobel_benchmark/opencv/samples/cpp/videocapture_gphoto2_autofocus.cpp"
    "/home/deanwhyone/18742/sobel_benchmark/opencv/samples/cpp/videocapture_gstreamer_pipeline.cpp"
    "/home/deanwhyone/18742/sobel_benchmark/opencv/samples/cpp/videocapture_image_sequence.cpp"
    "/home/deanwhyone/18742/sobel_benchmark/opencv/samples/cpp/videocapture_intelperc.cpp"
    "/home/deanwhyone/18742/sobel_benchmark/opencv/samples/cpp/videocapture_openni.cpp"
    "/home/deanwhyone/18742/sobel_benchmark/opencv/samples/cpp/videocapture_starter.cpp"
    "/home/deanwhyone/18742/sobel_benchmark/opencv/samples/cpp/videostab.cpp"
    "/home/deanwhyone/18742/sobel_benchmark/opencv/samples/cpp/videowriter_basic.cpp"
    "/home/deanwhyone/18742/sobel_benchmark/opencv/samples/cpp/warpPerspective_demo.cpp"
    "/home/deanwhyone/18742/sobel_benchmark/opencv/samples/cpp/watershed.cpp"
    "/home/deanwhyone/18742/sobel_benchmark/opencv/samples/cpp/CMakeLists.txt"
    )
endif()

