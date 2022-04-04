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

Lines offsetLines(Lines lines, int frame, int total_frames, int shift_dir = 1) {
	float offset_per_step = lines.distance / total_frames;
	lines.offset += frame * offset_per_step * shift_dir;
	return lines;
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

	auto cli = (
		option("-h", "--help").set(help) % "Show documentation." |
		(
			option("-p") & value("points", points),
			value("filename", filename)
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
		for (auto im : read_images) {
			cv::Mat fim;
			im.convertTo(fim, CV_32FC1);
			in_images.push_back(fim);
		}
	}

	cv::Size image_size = in_images.at(0).size();

	float blacklevel = 29.f;

	//Select or parse line defining points
	std::vector<cv::Point> points1;
	std::vector<cv::Point> points2;
	if (points == "") {
		points1 = clickPoints(in_images.at(0));
		points2 = clickPoints(in_images.at(60));
	} else {
		auto ps = parsePoints(points);
		if (ps.size() != 6) {
			std::cerr << "need 6 input points" << std::endl;
			return 1;
		}
		points1 = std::vector<cv::Point>(ps.begin(), ps.begin() + 3);
		points2 = std::vector<cv::Point>(ps.begin() + 3, ps.begin() + 6);
	}
	std::cout << points1 << std::endl;
	std::cout << points2 << std::endl;

	Lines lines1 = linesFromPoints(points1, 10);
	Lines lines2 = linesFromPoints(points2, 10);

	//Show line mask for one image
	cv::Mat mask1 = lineNumMask(lines1, image_size);
	cv::imshow("Mask", mask1);
	cv::imshow("Im1", in_images.at(0) / 255.f);


	std::vector<cv::Mat> calibration_factors(60);
	//Calculate calibration mask
	{
		struct Elem {
			Elem()
				: sum(0), num_elements(0) {
			}
			float sum;
			float num_elements;
		};

		std::map<int, Elem> mean_intensities; //sum, num_elements
		std::vector<float> frame_means;

		//Calculate mean of each frame and of each line
		int i = 0;
		for (int i = 0; i < 60; ++i) {
			cv::Mat frame = in_images.at(i) - blacklevel;
			//cv::imshow("Im", frame / 255.f);
			//while (cv::waitKey(1) != 'n') {
			//}
			//Add offset to lines
			Lines offset_lines = offsetLines(lines1, i, 60);
			cv::Mat mask = lineNumMask(offset_lines, in_images.at(0).size());

			cv::Rect mean_roi = cv::Rect(cv::Point(image_size) / 2 - cv::Point(200, 200), cv::Size(400, 400));
			float frame_mean = cv::mean(frame(mean_roi))[0];
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


		//Now generate calibration factors for each frame
		for(int j = 0; j < 60; ++j) {
			cv::Mat & calibration_fac = calibration_factors.at(i);
			calibration_fac.create(image_size, CV_32FC1);

			Lines offset_lines = offsetLines(lines1, i, 60);
			cv::Mat mask = lineNumMask(offset_lines, in_images.at(0).size());

			for (int y = 0; y < image_size.height; ++y) {
				for (int x = 0; x < image_size.width; ++x) {
					int8_t mask_value = mask.at<int8_t>(y, x);
					calibration_fac.at<float>(y, x) = 1.0 / frame_means[j] / mean_intensity[mask_value];
				}
			}
		}
	}
	cv::imshow("Cal", calibration_factors.at(0));


	while (cv::waitKey(1) != 'q') {

	}


	return 0;

}
