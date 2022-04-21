#pragma once
#include <opencv2/core/core.hpp>
#include <span>
#include "lines.h"

cv::Mat lineNumMask(Lines lines, cv::Size size);
std::vector<cv::Mat> calculateCalibrationFactors(std::span<cv::Mat> in_images, Lines lines, float blacklevel);
