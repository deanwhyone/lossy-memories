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
    Mat image, src, src_gray;
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
    //![load]

    //![reduce_noise]
    // Remove noise by blurring with a Gaussian filter ( kernel size = 3 )
    GaussianBlur(image, src, Size(3, 3), 0, 0, BORDER_DEFAULT);
    //![reduce_noise]

    //![convert_to_gray]
    // Convert the image to grayscale
    cvtColor(src, src_gray, COLOR_BGR2GRAY);
    //![convert_to_gray]

    cout << "src_gray is located at " << &src_gray << std::endl;

    //![sobel]
    /// Generate grad_x and grad_y
    Mat grad_x, grad_y;
    Mat abs_grad_x, abs_grad_y;

    /// Gradient X
    Sobel(src_gray, grad_x, ddepth, 1, 0, ksize, scale, delta, BORDER_DEFAULT);

    /// Gradient Y
    Sobel(src_gray, grad_y, ddepth, 0, 1, ksize, scale, delta, BORDER_DEFAULT);
    //![sobel]

    //![convert]
    // converting back to CV_8U
    convertScaleAbs(grad_x, abs_grad_x);
    convertScaleAbs(grad_y, abs_grad_y);
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
