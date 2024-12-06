/* slideextract -- implementation
 *
 * Copyright (c) 2013, Angelo Haller <angelo@szanni.org>
 * Copyright (c) 2022, Oleksiy Klymenko
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "slideextract.h"
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <string>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/videoio.hpp>

using namespace cv;

static bool selected_roi = 0;
static Point point1;
static Point point2;

static void
_se_mouseHandler(int event, int x, int y, int flags, void *param)
{
	Mat clone;
	Mat frame = *(Mat*)param;

	if (event == EVENT_LBUTTONDOWN && (flags & EVENT_FLAG_LBUTTON)) {
		point1 = Point (x, y);
		point2 = Point (x, y);
	}

	if (!(flags & EVENT_FLAG_LBUTTON))
		return;

	if (event != EVENT_MOUSEMOVE && event != EVENT_LBUTTONUP)
		return;

	point2 = Point(x, y);
	selected_roi = (point1.x != point2.x && point1.y != point2.y);

	clone = frame.clone();
	rectangle(clone, point1, point2, CV_RGB(0, 255, 0), 1, 8, 0);
	imshow("frame", clone);
}

int
se_select_roi(const char *file, struct roi *roi)
{
	VideoCapture capture = VideoCapture(file, CAP_ANY);

	if (!capture.isOpened())
		return -1;

	int key = -1;
	Mat frame;
	Mat clone;
	namedWindow("frame", WINDOW_AUTOSIZE);

	while (key == -1)
	{
		capture.read(frame);
		if (frame.empty())
			break;

		setMouseCallback("frame", _se_mouseHandler, &frame);

		if (selected_roi)
		{
			clone = frame.clone();
			rectangle(clone, point1, point2, CV_RGB(0, 255, 0), 1, 8, 0);
			imshow("frame", clone);
		}
		else
		{
			imshow("frame", frame);
		}

		key = waitKey (10);
	}

	startWindowThread();

	destroyAllWindows();

	if (selected_roi)
	{
		roi->x = (point1.x < point2.x) ? point1.x : point2.x;
		roi->y = (point1.y < point2.y) ? point1.y : point2.y;
		roi->width = abs(point2.x - point1.x);
		roi->height = abs(point2.y - point1.y);
		return 0;
	}

	return -1;
}

static double
_se_compare_image(Mat &img1, Mat &img2, struct roi *roi)
{
	double correlation;
	Mat result = Mat(1, 1, CV_32F);
	Mat i1, i2;

	if (roi != NULL) {
		Rect r = Rect(roi->x, roi->y, roi->width, roi->height);
		i1 = img1.clone()(r);
		i2 = img2.clone()(r);
	}
	else {
		i1 = img1;
		i2 = img2;
	}

	matchTemplate(i1, i2, result, TM_CCORR_NORMED);
	minMaxLoc(result, NULL, &correlation);

	return correlation;
}

int
se_extract_slides(const char *file, const char *outprefix, struct roi *roi, double threshold)
{
    int num = 0;
    char str[256];

    std::vector<int> png_params;
    png_params.push_back(cv::IMWRITE_PNG_COMPRESSION);
    png_params.push_back(9);

    std::vector<int> jpg_params;
    jpg_params.push_back(cv::IMWRITE_JPEG_QUALITY);
    jpg_params.push_back(95);

    Mat current;
    Mat last_exported;

    VideoCapture capture = VideoCapture(file, CAP_ANY);
    if (!capture.isOpened()) {
        std::cerr << "Error: Unable to open video file: " << file << std::endl;
        return -1;
    }

    bool is_final_frame = false;
    while (!is_final_frame) {
        capture.read(current);

        is_final_frame = current.empty();
        bool is_changed_frame = !current.empty() && !last_exported.empty()
            && _se_compare_image (last_exported, current, roi) <= threshold;

        if (is_final_frame || is_changed_frame || last_exported.empty()) {
            if (!current.empty()) {
                double timestamp_ms = capture.get(cv::CAP_PROP_POS_MSEC);
                int hours = static_cast<int>(timestamp_ms / (1000 * 60 * 60));
                int minutes = static_cast<int>((timestamp_ms / (1000 * 60)) - (hours * 60));
                int seconds = static_cast<int>((timestamp_ms / 1000) - (hours * 3600) - (minutes * 60));
                int milliseconds = static_cast<int>(timestamp_ms) % 1000;

                snprintf(str, sizeof(str), "%s%02d_%02d_%02d_%03d.png", 
                         outprefix, hours, minutes, seconds, milliseconds);
                
                std::cout << "Attempting to write file: " << str << std::endl;
                std::cout << "Image size: " << current.cols << "x" << current.rows << std::endl;
                std::cout << "Image type: " << current.type() << std::endl;

                try {
                    if (cv::imwrite(str, current, png_params)) {
                        std::cout << "Successfully wrote PNG image: " << str << std::endl;
                        last_exported = current.clone();
                    } else {
                        std::cerr << "Failed to write PNG image: " << str << std::endl;
                        std::string jpg_filename = std::string(str);
                        jpg_filename = jpg_filename.substr(0, jpg_filename.find_last_of('.')) + ".jpg";
                        if (cv::imwrite(jpg_filename, current, jpg_params)) {
                            std::cout << "Successfully wrote JPEG image: " << jpg_filename << std::endl;
                            last_exported = current.clone();
                        } else {
                            std::cerr << "Failed to write JPEG image: " << jpg_filename << std::endl;
                        }
                    }
                } catch (const cv::Exception& ex) {
                    std::cerr << "Exception writing image: " << ex.what() << std::endl;
                }

                num++;
            }
        }
    }

    std::cout << "Total frames extracted: " << num << std::endl;
    return 0;
}
