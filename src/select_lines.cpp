#include <string>
#include <stdexcept>
#include <stdint.h>
#include <vector>
#include <array>
#include "clipp.hpp"
#include <opencv2/opencv.hpp>
#include "calibration.h"

using namespace clipp;

void clickLineCb(int evt, int x, int y, int flags, void* param) {
	std::vector<cv::Point> * clicked_points = (std::vector<cv::Point> *)param;
    if(evt == cv::EVENT_LBUTTONDOWN) {
        clicked_points->push_back(cv::Point(x,y));
    }
}


std::array<cv::Point, 3> clickPoints(cv::Mat image) {
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

	std::array<cv::Point, 3> clicked_points_arr;
	std::copy(clicked_points.begin(), clicked_points.end(), clicked_points_arr.begin());
	return clicked_points_arr;
}

int main(int argc, char** argv) {
	bool help = false;
	std::string filename;
	float exposure_scale = 1.0;

	auto cli = (
		option("-h", "--help").set(help) % "Show documentation." |
		(
			value("filename", filename),
			option("-s") & value("exposure scale", exposure_scale)
		)
	);

	auto fmt = doc_formatting{}.doc_column(30);
	const char* exe_name = "select_lines";
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


	// Load input images
	cv::Mat image_u16;
	{
		std::vector<cv::Mat> read_images;
		if (!cv::imreadmulti(filename, read_images, cv::IMREAD_GRAYSCALE | cv::IMREAD_ANYDEPTH)) {
			std::cerr << "Could not read images " << filename << std::endl;
			return 2;
		}
		image_u16 = read_images.at(0);
	}

	cv::Mat image;
	image_u16.convertTo(image, CV_32FC1);

	double min, max;
	cv::minMaxLoc(image, &min, &max);
	image /= max;

	//Select or parse line defining points
	std::array<cv::Point, 3> line_defining_points = clickPoints(image * 255.f * exposure_scale);
	for (int i = 0; i < 3; ++i) {
		std::cout << line_defining_points[i].x << "," << line_defining_points[i].y;
		if (i != 2) {
			std::cout << ";";
		}
	}
	std::cout << std::endl;

	return 0;

}
