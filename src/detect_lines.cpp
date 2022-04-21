#include "detect_lines.h"

#include <opencv2/opencv.hpp>

Lines detect_lines(cv::Mat image) {
	cv::Mat dft_res;
	cv::dft(image, dft_res, cv::DFT_COMPLEX_OUTPUT);

	std::vector<cv::Mat> dfts(2);
	cv::split(dft_res, dfts);

	cv::Mat mag;
	cv::Mat phase;
	cv::cartToPolar(dfts[0], dfts[1], mag, phase);

	cv::Mat top_left = mag(cv::Rect(1, 1, 200, 200));
	top_left(cv::Rect(0, 0, 15, 15)) = 0;
	top_left(cv::Rect(0, 0, 200, 1)) = 0;
	top_left(cv::Rect(0, 0, 1, 200)) = 0;

	cv::imshow("dft", top_left/1000000);
	cv::imshow("dftwin", mag/1000000);

	double min, max;
	cv::Point minloc, maxloc2;
	cv::minMaxLoc(top_left, &min, &max, &minloc, &maxloc2);

	std::cout << maxloc2 << std::endl;

	// Find higher order frequency
	int higher_fac = 5;
	cv::Point higher_loc_approx = maxloc2*higher_fac;
	cv::Mat higher = mag(cv::Rect(higher_loc_approx - cv::Point(10, 5), cv::Size(20, 20)));
	cv::imshow("higher", higher/1000000);
	cv::Point maxloc;
	cv::minMaxLoc(higher, &min, &max, &minloc, &maxloc);
	maxloc += higher_loc_approx - cv::Point(10, 5);
	cv::Point2f freq2d = cv::Point2f(maxloc) / higher_fac;

	std::cout << maxloc << std::endl;
	std::cout << freq2d<< std::endl;

	double fdx = freq2d.x / image.cols;
	double fdy = -freq2d.y / image.rows;

	std::cout << fdx << ", " << fdy << std::endl;

	Lines lines;
	double frequency = std::sqrt(std::pow(fdx, 2) + std::pow(fdy, 2));
	lines.distance = 1 / frequency;
	lines.orientation = std::atan2(fdy, fdx);

	std::cout << phase.at<float>(maxloc) << std::endl;
	lines.offset = -(phase.at<float>(maxloc) / 2*M_PI + 0.5) * lines.distance;

	std::cout << lines.distance << " | " << lines.orientation << " | " << lines.offset << std::endl;

	return lines;
}
