#include "calibration.h"
#include <stdexcept>
#include <map>
#include <stdint.h>
#include <iostream>
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

Lines offsetLines(Lines lines, int frame, int total_frames, int shift_dir = 1) {
	float offset_per_step = lines.distance / total_frames;
	lines.offset += frame * offset_per_step * shift_dir;
	return lines;
}

std::vector<cv::Mat> calculateCalibrationFactors(std::span<cv::Mat> in_images, Lines lines, float blacklevel) {
	struct Elem {
		Elem()
			: sum(0), num_elements(0) {
			}
		float sum;
		float num_elements;
	};

	//Per line mean over all frames
	std::map<int, Elem> mean_intensities; //sum, num_elements
	//Per frame mean
	std::vector<float> frame_means;

	int num_steps = in_images.size();
	cv::Size image_size = in_images[0].size();

	//Calculate mean of each frame and of each line
	for (int i = 0; i < num_steps; ++i) {
		cv::Mat frame = in_images[i] - blacklevel;
		Lines offset_lines = offsetLines(lines, i, num_steps);
		cv::Mat mask = lineNumMask(offset_lines, in_images[0].size());

		//Calculate mean of frame (only in the center of the image)
		cv::Rect mean_roi = cv::Rect(cv::Point(image_size) / 2 - cv::Point(200, 200), cv::Size(400, 400));
		//float frame_mean = cv::mean(frame(mean_roi))[0];
		//Calculate mean only of bright pixels (part of the lines)
		cv::Mat mean_mask;
		cv::Mat t = (frame > 5.f);
		t.convertTo(mean_mask, CV_32FC1);
		cv::Mat masked_frame;
		cv::multiply(frame, mean_mask, masked_frame);
		float frame_mean = cv::sum(masked_frame(mean_roi))[0] / cv::sum(mean_mask(mean_roi))[0];
		frame_means.push_back(frame_mean);

		for (int y = 0; y < image_size.height; ++y) {
			for (int x = 0; x < image_size.width; ++x) {
				float value = frame.at<float>(y, x);
				int8_t mask_value = mask.at<int8_t>(y, x);
				if (value > 5) {
					// calculate mean as if frames were normalized
					mean_intensities[mask_value].sum += value / frame_mean;
					mean_intensities[mask_value].num_elements += 1;
				}
			}
		}
	}

	std::map<int, float> mean_intensity;
	auto calc_mean = [] (auto & pair)
	{
		return std::make_pair(pair.first, pair.second.sum / pair.second.num_elements);
	};
	std::transform(mean_intensities.begin(), mean_intensities.end(), std::inserter(mean_intensity, mean_intensity.end()), calc_mean);

	for (auto const & value : frame_means)
		std::cout << value << std::endl;
	for (auto const & [key, value] : mean_intensity)
		std::cout << key << ", " << value << std::endl;


	std::vector<cv::Mat> calibration_factors(num_steps);
	//Now generate calibration factors for each frame
	for(int i = 0; i < num_steps; ++i) {
		cv::Mat & calibration_fac = calibration_factors[i];
		calibration_fac.create(image_size, CV_32FC1);

		Lines offset_lines = offsetLines(lines, i, num_steps);
		cv::Mat mask = lineNumMask(offset_lines, image_size);

		for (int y = 0; y < image_size.height; ++y) {
			for (int x = 0; x < image_size.width; ++x) {
				int8_t mask_value = mask.at<int8_t>(y, x);
				calibration_fac.at<float>(y, x) = 1.0 / frame_means[i] / mean_intensity[mask_value];
			}
		}
	}

	return calibration_factors;
}

