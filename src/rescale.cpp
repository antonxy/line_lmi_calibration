#include "rescale.h"

#include <stdexcept>
#include <stdint.h>
#include <iostream>
#include <opencv2/opencv.hpp>

cv::Mat rescale_lines(cv::Mat input_image, Lines boundary_lines, int insert_pixels) {
	int line_distance_vertical;
	std::vector<int> line_position_vertical;


	//Assuming line is approximately horizontal

	//Find start and end point of line at left and right image boundary
	cv::Point line_start;
	cv::Point line_end;

	//Find vertical line position for each horizontal position
//	cv::LineIterator (
}
