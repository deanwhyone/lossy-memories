/**
 * @file Sobel_Demo.cpp
 * @brief Sample code uses Sobel or Scharr OpenCV functions for edge detection
 * @author OpenCV team
 */

#include <iostream>
#include "opencv2/core/utility.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/imgproc.hpp"

using namespace cv;
using namespace std;

void __COMPRESS__(void *ptr, size_t size) {
   (void)ptr;
   (void)size;
   return;
}

string typeToStr(int type) {
  string r;

  uchar depth = type & CV_MAT_DEPTH_MASK;
  uchar chans = 1 + (type >> CV_CN_SHIFT);

  switch ( depth ) {
    case CV_8U:  r = "8U"; break;
    case CV_8S:  r = "8S"; break;
    case CV_16U: r = "16U"; break;
    case CV_16S: r = "16S"; break;
    case CV_32S: r = "32S"; break;
    case CV_32F: r = "32F"; break;
    case CV_64F: r = "64F"; break;
    default:     r = "User"; break;
  }

  r += "C";
  r += (chans+'0');

  return r;
}

void markMatCompressible(Mat mat) {
    Size mat_size = mat.size();
    size_t mat_elem_size = mat.elemSize();

    unsigned char* mat_data_ptr = mat.data;
    if (mat.isContinuous()) {
        __COMPRESS__(mat_data_ptr,
                    mat_elem_size * mat_size.height * mat_size.width
        );
    } else {
        for (size_t col_idx = 0; col_idx < mat_size.height; ++col_idx) {
            for (size_t row_idx = 0; row_idx < mat_size.width; ++row_idx) {
                __COMPRESS__(
                    (mat_data_ptr + mat_elem_size * (mat_size.width * col_idx + row_idx)),
                    mat_elem_size
                );
            }
        }
    }
}

/**
 * @function main
 */
int main( int argc, char** argv )
{
    cv::CommandLineParser parser(argc, argv,
                                "{@input   |lena.jpg|input image}"
                                "{ksize   k|3|ksize}"
                                "{scale   s|1|scale}"
                                "{delta   d|0|delta}"
                                "{help    h|false|show help message}");

    cout << "The sample uses Sobel OpenCV functions for edge detection\n\n";
    parser.printMessage();
    cout << "\n";

    cv::setNumThreads(0);

    //![variables]
    // First we declare the variables we are going to use
    Mat image, src, src_grey;
    Mat grad;
    const String window_name = "Sobel Demo - Simple Edge Detector";
    int ksize = parser.get<int>("ksize");
    int scale = parser.get<int>("scale");
    int delta = parser.get<int>("delta");
    int ddepth = CV_16S;
    //![variables]

    //![load]
    String imageName = parser.get<String>("@input");
    // As usual we load our source image (src)
    image = imread( samples::findFile( imageName ), IMREAD_COLOR ); // Load an image

    // Check if image is loaded fine
    if( image.empty() ) {
        printf("Error opening image: %s\n", imageName.c_str());
        return EXIT_FAILURE;
    }
    markMatCompressible(image);
    //![load]

    //![reduce_noise]
    // Remove noise by blurring with a Gaussian filter ( kernel size = 3 )
    GaussianBlur(image, src, Size(3, 3), 0, 0, BORDER_DEFAULT);
    //![reduce_noise]
    markMatCompressible(src);

    //![convert_to_gray]
    // Convert the image to grayscale
    cvtColor(src, src_grey, COLOR_BGR2GRAY);
    //![convert_to_gray]
    markMatCompressible(src_grey);

    //![sobel]
    /// Generate grad_x and grad_y
    Mat grad_x, grad_y;
    Mat abs_grad_x, abs_grad_y;

    /// Gradient X
    Sobel(src_grey, grad_x, ddepth, 1, 0, ksize, scale, delta, BORDER_DEFAULT);
    markMatCompressible(grad_x);

    /// Gradient Y
    Sobel(src_grey, grad_y, ddepth, 0, 1, ksize, scale, delta, BORDER_DEFAULT);
    markMatCompressible(grad_y);
    //![sobel]

    //![convert]
    // converting back to CV_8U
    convertScaleAbs(grad_x, abs_grad_x);
    convertScaleAbs(grad_y, abs_grad_y);
    markMatCompressible(abs_grad_x);
    markMatCompressible(abs_grad_y);
    //![convert]

    //![blend]
    /// Total Gradient (approximate)
    addWeighted(abs_grad_x, 0.5, abs_grad_y, 0.5, 0, grad);
    //![blend]

    //![display]
    // imshow(window_name, grad);
    // char key = (char)waitKey(0);
    imwrite("edges_" + imageName, grad);
    //![display]
    return EXIT_SUCCESS;
}
