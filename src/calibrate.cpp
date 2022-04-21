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

int main(int argc, char** argv) {
	bool help = false;
	std::string filename;
	std::string points;
	std::string output_filename;
	int num_images;
	int num_directions;
	float blacklevel;

	auto cli = (
		option("-h", "--help").set(help) % "Show documentation." |
		(
			option("-p") & value("points", points),
			required("-n") & value("num_images", num_images),
			required("-d") & value("num_directions", num_directions),
			required("-b") & value("blacklevel", blacklevel),
			value("filename", filename),
			option("-o") & value("output", output_filename)
		)
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


	// Load input images
	std::vector<cv::Mat> in_images;
	{
		std::vector<cv::Mat> read_images;
		if (!cv::imreadmulti(filename, read_images, cv::IMREAD_GRAYSCALE | cv::IMREAD_ANYDEPTH)) {
			std::cerr << "Could not read images" << std::endl;
			return 2;
		}
		if (num_directions * num_images > read_images.size()) {
			std::cerr << "Too few images in input file" << std::endl;
		}
		for (auto im : read_images) {
			cv::Mat fim;
			im.convertTo(fim, CV_32FC1);
			in_images.push_back(fim);
		}
	}

	cv::Size image_size = in_images.at(0).size();

	//Select or parse line defining points
	std::vector<std::array<cv::Point, 3>> line_defining_points(num_directions);
	if (points == "") {
		for (int direction_idx = 0; direction_idx < num_directions; ++direction_idx) {
			auto clicked_points = clickPoints(in_images.at(direction_idx * num_images));
			line_defining_points.at(direction_idx) = clicked_points;
			for (int i = 0; i < 3; ++i) {
				std::cout << clicked_points.at(i).x << "," << clicked_points.at(i).y << ";";
			}
		}
		std::cout << std::endl;
	} else {
		auto ps = parsePoints(points);
		if (ps.size() != num_directions * 3) {
			std::cerr << "need " << num_directions * 3 << " input points" << std::endl;
			return 1;
		}
		for (int direction_idx = 0; direction_idx < num_directions; ++direction_idx) {
			std::copy(ps.begin() + direction_idx * 3, ps.begin() + (direction_idx + 1) * 3, line_defining_points.at(direction_idx).begin());
		}
	}

	std::vector<cv::Mat> all_calibration_factors;
	for (int direction_idx = 0; direction_idx < num_directions; ++direction_idx) {
		std::span images{in_images.begin() + direction_idx * num_images, in_images.begin() + (direction_idx + 1) * num_images};

		std::array<cv::Point, 3> points = line_defining_points.at(direction_idx);
		Lines lines = linesFromPoints(points, 10);

		std::vector<cv::Mat> calibration_factors = calculateCalibrationFactors(images, lines, blacklevel);

		cv::imshow("Cal", calibration_factors.at(0));

		// Test calibration
		cv::Mat mip(image_size, CV_32FC1);
		{
			for (int i = 0; i < 60; ++i) {
				cv::Mat frame = images[i] - blacklevel;
				cv::Mat calibrated_frame;
				cv::multiply(frame, calibration_factors.at(i), calibrated_frame);
				mip = cv::max(calibrated_frame, mip);
			}
		}
		std::stringstream ss;
		ss << direction_idx << "MIP";
		cv::imshow(ss.str().c_str(), mip / 10.f);

		std::copy(calibration_factors.begin(), calibration_factors.end(), std::back_inserter(all_calibration_factors));
	}

	if (output_filename != "") {
		cv::imwrite(output_filename, all_calibration_factors);
	}

	while (cv::waitKey(1) != 'q') {

	}


	return 0;

}
