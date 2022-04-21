#pragma once
#include <opencv2/core/core.hpp>

struct Lines {
	float distance;
	float orientation;
	float offset;
};

Lines linesFromPoints(std::array<cv::Point, 3> points, int num_lines);

cv::Mat draw_lines(Lines lines, cv::Size size);
