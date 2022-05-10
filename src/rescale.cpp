#include "rescale.h"

#include <stdexcept>
#include <stdint.h>
#include <iostream>
#include <opencv2/opencv.hpp>

cv::Mat rescale_lines(cv::Mat input_image, MultiLine boundary_lines, int insert_pixels) {
	cv::Mat debug_image;
	cv::normalize(input_image, debug_image, 255.0, 0.0, cv::NORM_MINMAX);
	debug_image.convertTo(debug_image, CV_8UC3);
	cv::cvtColor(debug_image, debug_image, cv::COLOR_GRAY2BGR);

	int line_distance_vertical;
	std::vector<int> line_position_vertical;

	int width = input_image.cols;

	//Assuming line is approximately horizontal

	Line line = boundary_lines.get_line(-10);

	//Find start and end point of line at left and right image boundary
	cv::Point line_start(0, line.evaluate_x(0));
	cv::Point line_end(width, line.evaluate_x(width));

	cv::line(debug_image, line_start, line_end, cv::Scalar(0, 255, 0), 3);
	debug_image(cv::Rect(line_start, cv::Size(10, 10))) = cv::Scalar(255, 0, 0);
	//debug_image(cv::Rect(line_end, cv::Size(10, 10))) = cv::Scalar(255, 0, 0);

	std::cout << "origin" << line.origin() << std::endl;
	debug_image(cv::Rect(line.origin(), cv::Size(10, 10))) = cv::Scalar(0, 255, 0);
	debug_image(cv::Rect(line.origin() + line.direction() * 20, cv::Size(10, 10))) = cv::Scalar(0, 255, 0);
	cv::imshow("debugrescale", debug_image);

	std::cout << line_start << line_end << std::endl;

	//Find vertical line position for each horizontal position
//	cv::LineIterator (

	return input_image;
}
