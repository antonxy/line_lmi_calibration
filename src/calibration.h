#include <opencv2/core/core.hpp>
#include <span>

struct Lines {
	float distance;
	float orientation;
	float offset;
};

Lines linesFromPoints(std::array<cv::Point, 3> points, int num_lines);
cv::Mat lineNumMask(Lines lines, cv::Size size);
std::vector<cv::Mat> calculateCalibrationFactors(std::span<cv::Mat> in_images, Lines lines, float blacklevel);
