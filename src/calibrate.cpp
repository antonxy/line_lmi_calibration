#include <string>
#include <stdexcept>
#include <stdint.h>
#include "clipp.hpp"
#include <opencv2/opencv.hpp>

using namespace clipp;

struct Lines {
	float distance;
	float orientation;
	float offset;
};

void clickLineCb(int evt, int x, int y, int flags, void* param) {
	std::vector<cv::Point> * clicked_points = (std::vector<cv::Point> *)param;
    if(evt == cv::EVENT_LBUTTONDOWN) {
        clicked_points->push_back(cv::Point(x,y));
    }
}

Lines linesFromPoints(std::vector<cv::Point> points, int num_lines) {
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
	return int(x - lines.offset) / lines.distance;
}

cv::Mat lineNumMask(Lines lines, cv::Size size) {
	cv::Mat mask(size, CV_8SC1);
	for (int y = 0; y < size.height; ++y) {
		for (int x = 0; x < size.width; ++x) {
			int8_t index = lineNumAtPoint(lines, cv::Point(x, y));
			mask.at<int8_t>(y, x) = index;
		}
	}
	return mask;
}

std::vector<cv::Point> clickPoints(cv::Mat image) {
	std::vector<cv::Point> clicked_points;
	cv::Mat click_image;
	image.convertTo(click_image, CV_8UC3);

	cv::imshow("Click Line", click_image);
	cv::setMouseCallback("Click Line", clickLineCb, (void*)&clicked_points);

	unsigned int drawnCircles = 0;
	while (clicked_points.size() < 3) {
		if (cv::waitKey(1) == 'q') {
			throw std::runtime_error("User abort");
		}
		if (clicked_points.size() > drawnCircles) {
			cv::circle(click_image, clicked_points.at(drawnCircles), 10, cv::Scalar(255, 255, 255), -1);
			cv::imshow("Click Line", click_image);
			drawnCircles++;
		}
	}
	cv::destroyWindow("Click Line");

	return clicked_points;
}

int main(int argc, char** argv) {
	bool help = false;
	std::string filename;

	auto cli = (
		option("-h", "--help").set(help) % "Show documentation." |
		value("filename", filename)
	);

	auto fmt = doc_formatting{}.doc_column(30);
	const char* exe_name = "calibrate";
	parsing_result parse_result = parse(argc, argv, cli);
	if (!parse_result) {
		std::cerr << "Invalid arguments. See arguments below or use " << exe_name << " -h for more info\n";
		std::cerr << usage_lines(cli, exe_name, fmt) << '\n';
		return 1;
	}

	if (help) {
		std::cout << make_man_page(cli, exe_name, fmt) << '\n';
		return 0;
	}


	std::vector<cv::Mat> in_images;
	{
		std::vector<cv::Mat> read_images;
		if (!cv::imreadmulti(filename, read_images, cv::IMREAD_GRAYSCALE | cv::IMREAD_ANYDEPTH)) {
			std::cerr << "Could not read images" << std::endl;
			return 2;
		}
		for (auto im : read_images) {
			cv::Mat fim;
			im.convertTo(fim, CV_32FC1);
			in_images.push_back(fim);
		}
	}

	std::vector<cv::Point> points1 = clickPoints(in_images.at(0));
	std::vector<cv::Point> points2 = clickPoints(in_images.at(60));


	Lines lines1 = linesFromPoints(points1, 10);
	Lines lines2 = linesFromPoints(points2, 10);

	cv::Mat mask1 = lineNumMask(lines1, in_images.at(0).size());
	cv::imshow("Mask", mask1);
	cv::imshow("Im1", in_images.at(0) / 255.f);


	while (cv::waitKey(1) != 'q') {

	}


	return 0;

}
