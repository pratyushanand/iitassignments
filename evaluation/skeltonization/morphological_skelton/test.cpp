#include <stdio.h>
#include <cv.h>
#include <cxtypes.h>
#include <highgui.h>
#include <cvaux.h>

int main (int argc, char **argv)
{
	cv::Mat img = cv::imread(argv[1], 0);
	cv::threshold(img, img, 127, 255, cv::THRESH_BINARY);

	cv::Mat skel(img.size(), CV_8UC1, cv::Scalar(0));
//	cv::Mat temp(img.size(), CV_8UC1);

	cv::Mat element = cv::getStructuringElement(cv::MORPH_CROSS, cv::Size(3, 3));

	cv::Mat temp;
	cv::Mat eroded;

	bool done;
	do
	{
#if 0
		cv::morphologyEx(img, temp, cv::MORPH_OPEN, element);
		cv::bitwise_not(temp, temp);
		cv::bitwise_and(img, temp, temp);
		cv::bitwise_or(skel, temp, skel);
		cv::erode(img, img, element);

		double max;
		cv::minMaxLoc(img, 0, &max);
		done = (max == 0);
#else
		cv::erode(img, eroded, element);
		cv::dilate(eroded, temp, element); // temp = open(img)
		cv::subtract(img, temp, temp);
		cv::bitwise_or(skel, temp, skel);
		eroded.copyTo(img);

		done = (cv::countNonZero(img) == 0);

#endif
	} while (!done);

	cv::imshow("Skeleton", skel);
	cv::waitKey(0);

	 
	do
	{
		} while (!done);
}
