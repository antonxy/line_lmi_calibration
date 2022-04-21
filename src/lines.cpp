#include "lines.h"
#include <cmath>
#include <opencv2/opencv.hpp>

Lines linesFromPoints(std::array<cv::Point, 3> points, int num_lines) {
	cv::Point p1 = points.at(0);
	cv::Point p2 = points.at(1);
	cv::Point p3 = points.at(2);

	cv::Point delta = p2 - p1;
	float ori = -atan2(delta.y, delta.x) + M_PI / 2;

	float dist = cv::norm((p2-p1).cross(p1-p3)) / cv::norm(p2-p1) / num_lines;

	cv::Point p0(0, 0);
	float off = cv::norm((p2-p1).cross(p1-p0)) / cv::norm(p2-p1) + dist / 2;

	Lines lines;
	lines.orientation = ori;
	lines.distance = dist;
	lines.offset = off;
	return lines;
}

int8_t lineNumAtPoint(Lines lines, cv::Point point) {
    // rotate coords by orientation
	float x = point.x * std::cos(lines.orientation) - point.y * std::sin(lines.orientation);

	// now calculate line num as is lines were vertical
	return std::abs(fmod((x - lines.offset), lines.distance)) < 1;
}

cv::Mat draw_lines(Lines lines, cv::Size size) {
	cv::Mat mask(size, CV_8UC1);
	for (int y = 0; y < size.height; ++y) {
		for (int x = 0; x < size.width; ++x) {
			int8_t index = lineNumAtPoint(lines, cv::Point(x, y)) * 255;
			mask.at<int8_t>(y, x) = index;
		}
	}
	return mask;
}

