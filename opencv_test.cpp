#include <opencv2/opencv.hpp>
#include <iostream>

int main() {
    std::cout << "OpenCV version: " << CV_VERSION << std::endl;

    cv::Mat image(100, 100, CV_8UC3, cv::Scalar(0, 0, 255));

    std::vector<int> params;
    params.push_back(cv::IMWRITE_PNG_COMPRESSION);
    params.push_back(9);

    try {
        std::cout << "Attempting to write PNG file..." << std::endl;
        bool success = cv::imwrite("test.png", image, params);
        if (success) {
            std::cout << "Successfully wrote PNG file" << std::endl;
        } else {
            std::cerr << "Failed to write PNG file" << std::endl;
        }
    } catch (const cv::Exception& ex) {
        std::cerr << "Exception converting image to PNG format: " << ex.what() << std::endl;
    }

    try {
        std::cout << "Attempting to write JPG file..." << std::endl;
        bool success = cv::imwrite("test.jpg", image);
        if (success) {
            std::cout << "Successfully wrote JPG file" << std::endl;
        } else {
            std::cerr << "Failed to write JPG file" << std::endl;
        }
    } catch (const cv::Exception& ex) {
        std::cerr << "Exception converting image to JPG format: " << ex.what() << std::endl;
    }

    return 0;
}
