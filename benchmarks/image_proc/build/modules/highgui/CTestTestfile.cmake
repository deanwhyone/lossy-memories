# CMake generated Testfile for 
# Source directory: /home/deanwhyone/18742/sobel_benchmark/opencv/modules/highgui
# Build directory: /home/deanwhyone/18742/sobel_benchmark/build/modules/highgui
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test(opencv_test_highgui "/home/deanwhyone/18742/sobel_benchmark/build/bin/opencv_test_highgui" "--gtest_output=xml:opencv_test_highgui.xml")
set_tests_properties(opencv_test_highgui PROPERTIES  LABELS "Main;opencv_highgui;Accuracy" WORKING_DIRECTORY "/home/deanwhyone/18742/sobel_benchmark/build/test-reports/accuracy")
