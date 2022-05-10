#pragma once
#include <opencv2/core/core.hpp>
//#include <eigen3/Eigen/Core>

struct Lines {
	float distance;
	float orientation;
	float offset;
};

Lines linesFromPoints(std::array<cv::Point, 3> points, int num_lines);


struct Line {
	float orientation; //rad
	float offset; //distance from origin

	cv::Point2f origin() {
		return cv::Point(std::cos(orientation + M_PI / 2) * offset, std::sin(orientation + M_PI / 2) * offset);
	}

	cv::Point2f direction() {
		return cv::Point2f(std::cos(orientation), std::sin(orientation));
	}

	float evaluate_x(float x) {
		float r = (x - origin().x) / direction().x;
		return origin().y + direction().y * r;
	}
};

struct MultiLine {
	Line zero_line;
	float distance;

	Line get_line(int index) {
		Line line = zero_line;
		line.offset += index * distance;
		return line;
	}


};

cv::Mat draw_lines(MultiLine lines, cv::Size size);


