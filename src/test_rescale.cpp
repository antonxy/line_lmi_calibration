#include <string>
#include <stdexcept>
#include <stdint.h>
#include <vector>
#include <array>
#include <opencv2/opencv.hpp>
#include "clipp.hpp"
#include "detect_lines.h"
#include "rescale.h"
using namespace clipp;

int main(int argc, char** argv) {
	bool help = false;
	std::string images_filename;

	auto cli = (
		option("-h", "--help").set(help) % "Show documentation." |
		(
			required("-i") & value("images", images_filename)
		)
	);

	auto fmt = doc_formatting{}.doc_column(30);
	const char* exe_name = "apply_calibration";
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

	double min, max;
	cv::minMaxLoc(in_images.at(0), &min, &max);
	cv::imshow("in", in_images.at(0) / max);

	Lines lines = detect_lines(in_images.at(0));


	cv::Mat line_im = draw_lines(lines, in_images.at(0).size());
	cv::imshow("lines", line_im);

	cv::Mat lines_im2 = in_images.at(0).clone();
	lines_im2.setTo(0, line_im);
	cv::imshow("lines2", lines_im2 / max);

	while(cv::waitKey(1) != 'q');
}
