#include <string>
#include <stdexcept>
#include <stdint.h>
#include <vector>
#include <array>
#include <opencv2/opencv.hpp>
#include "clipp.hpp"
#include "detect_lines.h"
#include <filesystem>
using namespace clipp;

cv::Mat on_mask(MultiLine lines, cv::Size size, float width = 1.0) {
	cv::Mat mask(size, CV_32FC1);
	for (int y = 0; y < size.height; ++y) {
		for (int x = 0; x < size.width; ++x) {
			float dist = lines.pointDistance(cv::Point(x, y)) / width;
			float val = std::exp(-(dist*dist));
			mask.at<float>(x, y) = val;
		}
	}
	return mask;
}

int main(int argc, char** argv) {
	bool help = false;
	std::vector<std::string> image_filenames;
	std::string points;
	float alpha_fac = 1.0;
	float blacklevel;
	std::string output_folder;
	bool debug = false;
	bool no_subtract = false;
	bool widefield = false;

	auto cli = (
		option("-h", "--help").set(help) % "Show documentation." |
		(
			option("-d").set(debug),
			option("-w").set(widefield),
			option("--no-subtract").set(no_subtract),
			option("-a") & value("alpha factor", alpha_fac),
			required("-p") & value("points", points),
			required("-b") & value("blacklevel", blacklevel),
			required("-i") & values("images", image_filenames),
			option("-o") & value("output folder", output_folder)
		)
	);

	auto fmt = doc_formatting{}.doc_column(30);
	const char* exe_name = "scasub";
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

	auto ps = parsePoints(points);
	if (ps.size() != 3) {
		std::cerr << "need 3 input points" << std::endl;
		return 1;
	}
	std::array<cv::Point, 3> line_defining_points;
	std::copy(ps.begin(), ps.end(), line_defining_points.begin());

	auto lines = MultiLine::fromPoints(line_defining_points, 10);

	std::vector<cv::Mat> masks;
	std::vector<cv::Mat> off_masks;


	std::vector<float> normalization_factors;
	//Calculate normalization per scanning angle
	//Actually this should take multiscan into account. But then it gets more complicated...
	{
		// Load input images
		std::vector<cv::Mat> in_images;
		{
			std::vector<cv::Mat> read_images;
			if (!cv::imreadmulti(image_filenames.at(image_filenames.size() - 1), read_images, cv::IMREAD_GRAYSCALE | cv::IMREAD_ANYDEPTH)) {
				std::cerr << "Could not read images " << image_filenames.at(image_filenames.size() - 1) << std::endl;
				return 2;
			}
			for (auto im : read_images) {
				cv::Mat fim;
				im.convertTo(fim, CV_32FC1);
				in_images.push_back(fim - blacklevel);
			}
		}

		std::vector<float> means;
		float sum_of_means = 0;
		for (int i = 0; i < in_images.size(); ++i) {
			float avg = cv::mean(in_images[i])[0];
			sum_of_means += avg;
			means.push_back(avg);
		}
		float mean_of_means = sum_of_means / in_images.size();
		for (int i = 0; i < in_images.size(); ++i) {
			normalization_factors.push_back(1 / (means[i] / mean_of_means));
		}
	}


	for (std::string image_filename : image_filenames) {
		std::cerr << "File " << image_filename << std::endl;
		// Load input images
		std::vector<cv::Mat> in_images;
		{
			std::vector<cv::Mat> read_images;
			if (!cv::imreadmulti(image_filename, read_images, cv::IMREAD_GRAYSCALE | cv::IMREAD_ANYDEPTH)) {
				std::cerr << "Could not read images " << image_filename << std::endl;
				return 2;
			}
			for (auto im : read_images) {
				cv::Mat fim;
				im.convertTo(fim, CV_32FC1);
				in_images.push_back(fim - blacklevel);
			}
		}

		std::vector<float> means;
		float sum_of_means = 0;
		for (int i = 0; i < in_images.size(); ++i) {
			float avg = cv::mean(in_images[i])[0];
			sum_of_means += avg;
			means.push_back(avg);
		}
		float mean_of_means = sum_of_means / in_images.size();
		std::vector<float> normalization_factors;
		for (int i = 0; i < in_images.size(); ++i) {
			normalization_factors.push_back(1 / (means[i] / mean_of_means));
		}



		cv::Mat on_result = cv::Mat::zeros(in_images.at(0).size(), CV_32FC1);
		cv::Mat off_result = cv::Mat::zeros(in_images.at(0).size(), CV_32FC1);

		for (int i = 0; i < in_images.size(); ++i) {
			if (debug) std::cerr << "Iteration " << i << std::endl;
			cv::Mat image = in_images.at(i) * normalization_factors[i];

			if (debug) {
				double min, max;
				cv::minMaxLoc(image, &min, &max);
				cv::Mat image_norm = image / max;
				cv::imshow("in", image_norm);
			}

			cv::Mat mask;
			cv::Mat off_mask;
			if (masks.size() <= i) {
				MultiLine shifted = lines.shifted(i, in_images.size());

				mask = on_mask(shifted, image.size(), 2.0);
				masks.push_back(mask);
				//cv::Mat off_mask = (1.f - mask) * (1.f / (in_images.size() - 1));
				off_mask = on_mask(shifted.shifted(1, 2), image.size(), 4.0) / 2.f;
				off_masks.push_back(off_mask);
			} else {
				mask = masks.at(i);
				off_mask = off_masks.at(i);
			}
			if (debug) cv::imshow("mask", mask);
			if (debug) cv::imshow("off_mask", off_mask);

			if (widefield) {
				on_result += image;
			} else {
				on_result += mask.mul(image);
				off_result += off_mask.mul(image);
			}

		}

		cv::Mat result;
		if (no_subtract || widefield) {
			result = on_result;
		} else {
			result = on_result - alpha_fac * off_result;
		}
		if (debug) {
			double min, max;
			cv::minMaxLoc(on_result, &min, &max);
			cv::imshow("on_res", on_result / max);
			cv::minMaxLoc(off_result, &min, &max);
			cv::imshow("off_res", off_result / max);

			cv::minMaxLoc(result, &min, &max);
			cv::imshow("result", result / max);
		}

		if (!output_folder.empty()) {
			std::filesystem::path out_filename = std::filesystem::path(output_folder) / std::filesystem::path(image_filename).filename();
			cv::imwrite(out_filename.string(), result);
		}

		if (debug) {
			while(int key = cv::waitKey(1)) {
				if (key == 'q') {
					return 0;
				}
			}
		}
	}
}
