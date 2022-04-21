#pragma once
#include "lines.h"

cv::Mat rescale_lines(cv::Mat input_image, Lines boundary_lines, int insert_pixels);
