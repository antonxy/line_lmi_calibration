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


void
Line( cv::Mat& img, cv::Point pt1, cv::Point pt2,
      const void* _color, int connectivity = 8 )
{
    if( connectivity == 0 )
        connectivity = 8;
    else if( connectivity == 1 )
        connectivity = 4;

    cv::LineIterator iterator(img, pt1, pt2, connectivity, true);
    int i, count = iterator.count;
    int pix_size = (int)img.elemSize();
    const uchar* color = (const uchar*)_color;

	std::cout << "Line: " << count << std::endl;

    if( pix_size == 3 )
    {
        for( i = 0; i < count; i++, ++iterator )
        {
            uchar* ptr = *iterator;
            ptr[0] = color[0];
            ptr[1] = color[1];
            ptr[2] = color[2];
        }
    }
    else
    {
        for( i = 0; i < count; i++, ++iterator )
        {
            uchar* ptr = *iterator;
            if( pix_size == 1 )
                ptr[0] = color[0];
            else
                memcpy( *iterator, color, pix_size );
        }
    }
}


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


	cv::Mat img(500, 500, CV_8UC3, cv::Scalar(0, 0, 0));
	double colr[4] = {0, 0, 255, 255};
	Line(img, cv::Point(40, 40), cv::Point(300, 300), colr);

	cv::Point p1(40, 40);
	cv::Point p2(300, 300);
	std::cout << cv::clipLine(img.size(), p1, p2) << std::endl;
	std::cout << p1 << p2 << std::endl;
	img(cv::Rect(0, 100, 10, 10)) = cv::Scalar(0, 0, 255);
	cv::imshow( "Kalman", img );

	double min, max;
	cv::minMaxLoc(in_images.at(0), &min, &max);
	cv::imshow("in", in_images.at(0) / max);

	MultiLine lines = detect_lines(in_images.at(0));

	cv::Mat line_im = draw_lines(lines, in_images.at(0).size());
	cv::imshow("lines", line_im);

	cv::Mat lines_im2 = in_images.at(0).clone();
	lines_im2.setTo(0, line_im);
	cv::imshow("lines2", lines_im2 / max);

	cv::Mat rescaled = rescale_lines(in_images.at(0), lines, 10);
	cv::imshow("rescaled", rescaled / max);

	while(cv::waitKey(1) != 'q');
}
