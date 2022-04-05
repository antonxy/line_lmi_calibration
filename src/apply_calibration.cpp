#include <string>
#include <stdexcept>
#include <stdint.h>
#include <vector>
#include <array>
#include "clipp.hpp"
#include <opencv2/opencv.hpp>
#include "calibration.h"

using namespace clipp;

int main(int argc, char** argv) {
	bool help = false;
	std::string images_filename;
	std::string calibration_filename;
	std::string output_filename;
	float blacklevel;

	auto cli = (
		option("-h", "--help").set(help) % "Show documentation." |
		(
			required("-b") & value("blacklevel", blacklevel),
			required("-i") & value("images", images_filename),
			required("-c") & value("calibration", calibration_filename),
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
		if (!cv::imreadmulti(images_filename, read_images, cv::IMREAD_GRAYSCALE | cv::IMREAD_ANYDEPTH)) {
			std::cerr << "Could not read images" << std::endl;
			return 2;
		}
		for (auto im : read_images) {
			cv::Mat fim;
			im.convertTo(fim, CV_32FC1);
			in_images.push_back(fim);
		}
	}

	//Load calibration images
	std::vector<cv::Mat> calibration_factors;
	{
		if (!cv::imreadmulti(calibration_filename, calibration_factors, cv::IMREAD_GRAYSCALE | cv::IMREAD_ANYDEPTH)) {
			std::cerr << "Could not read images" << std::endl;
			return 2;
		}
	}

	cv::Size image_size = in_images.at(0).size();

	std::vector<cv::Mat> calibrated_images;
	for (int i = 0; i < calibration_factors.size(); ++i) {
		cv::Mat image = in_images.at(i) - blacklevel;
		auto & cal = calibration_factors.at(i);
		cv::Mat calibrated;
		cv::multiply(image, cal, calibrated);
		calibrated_images.push_back(calibrated);
	}

	if (output_filename != "") {
		cv::imwrite(output_filename, calibrated_images);
	}



	return 0;

}
