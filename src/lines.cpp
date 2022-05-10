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

int8_t lineNumAtPoint(MultiLine lines, cv::Point point) {
    // rotate coords by orientation
	float x = point.x * std::cos(lines.zero_line.orientation) - point.y * std::sin(lines.zero_line.orientation);

	// now calculate line num as is lines were vertical
	return std::abs(fmod((x - lines.zero_line.offset), lines.distance)) < 3;
}

cv::Mat draw_lines(MultiLine lines, cv::Size size) {
	cv::Mat mask(size, CV_8UC1);
	for (int y = 0; y < size.height; ++y) {
		for (int x = 0; x < size.width; ++x) {
			int8_t index = lineNumAtPoint(lines, cv::Point(x, y)) * 255;
			mask.at<int8_t>(x, y) = index;
		}
	}
	return mask;
}

std::vector<cv::Point> parsePoints(std::string input) {
	std::vector<cv::Point> points;
	std::istringstream iss;
	iss.str(input);
	for (std::string line; std::getline(iss, line, ';');) {
		std::vector<int> vals;
		std::istringstream iss2;
		iss2.str(line);
		for (std::string val; std::getline(iss2, val, ',');) {
			int p = std::stoi(val);
			vals.push_back(p);
		}
		points.push_back(cv::Point(vals.at(0), vals.at(1)));
	}
	return points;
}

