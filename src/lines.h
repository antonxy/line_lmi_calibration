#pragma once
#include <opencv2/core/core.hpp>
#include <string>
#include <vector>
#include <array>
//#include <eigen3/Eigen/Core>

struct Lines {
	float distance;
	float orientation;
	float offset;
};

Lines linesFromPoints(std::array<cv::Point, 3> points, int num_lines);

std::vector<cv::Point> parsePoints(std::string input);

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

	static MultiLine fromPoints(std::array<cv::Point, 3> points, int num_lines) {
		cv::Point p1 = points.at(0);
		cv::Point p2 = points.at(1);
		cv::Point p3 = points.at(2);

		cv::Point delta = p2 - p1;
		float ori = -atan2(delta.y, delta.x) + M_PI / 2;

		float dist = cv::norm((p2-p1).cross(p1-p3)) / cv::norm(p2-p1) / num_lines;

		cv::Point p0(0, 0);
		float off = cv::norm((p2-p1).cross(p1-p0)) / cv::norm(p2-p1);

		MultiLine lines;
		lines.zero_line.orientation = ori;
		lines.distance = dist;
		lines.zero_line.offset = off;
		return lines;
	}

	Line get_line(int index) {
		Line line = zero_line;
		line.offset += index * distance;
		return line;
	}

	MultiLine shifted(int index, int total_num, bool direction = true) {
		MultiLine shifted(*this);
		if (direction) {
			shifted.zero_line.offset += float(index) / total_num * distance;
		} else {
			shifted.zero_line.offset -= float(index) / total_num * distance;
		}
		return shifted;
	}

	float pointDistance(cv::Point point) {
		// rotate coords by orientation
		float x = - point.x * std::cos(this->zero_line.orientation - M_PI/2) - point.y * std::sin(this->zero_line.orientation - M_PI/2);

		// now calculate line num as is lines were vertical
		return std::abs(remainderf((x - this->zero_line.offset), this->distance));
	}

};

cv::Mat draw_lines(MultiLine lines, cv::Size size);


