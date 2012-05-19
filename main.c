#include "vibe-background.h"
#include <cv.h>
#include <cxtypes.h>
#include <highgui.h>
#define MIN_AREA 1000

static int32_t get_image_width(CvCapture *stream)
{
	return cvGetCaptureProperty(stream, CV_CAP_PROP_FRAME_WIDTH);
}

static int32_t get_image_height(CvCapture *stream)
{
	return cvGetCaptureProperty(stream, CV_CAP_PROP_FRAME_HEIGHT);
}

static int32_t get_image_stride(CvCapture *stream)
{
	/* we insure that it input image will always be gray scale */
	return cvGetCaptureProperty(stream, CV_CAP_PROP_FRAME_WIDTH);
}

static int32_t acquire_grayscale_image(CvCapture *stream, IplImage *gray)
{
	IplImage *img = cvQueryFrame(stream);

	if (!img)
		return -1;

	cvCvtColor(img, gray, CV_BGR2GRAY);
	return 0;
}

int main(int argc, char **argv)
{
	CvCapture* stream = cvCaptureFromFile("../test_video/test.avi");
	CvMemStorage* storage = cvCreateMemStorage(0);
	int32_t width = get_image_width(stream);
	int32_t height = get_image_height(stream);
	int32_t stride = get_image_stride(stream);
	CvSeq* contours = NULL;
	CvSeq* c = NULL;
	int count = 0;
	int nc = 0;

	cvNamedWindow("TEST", 1);
	/* Get a model data structure */
	/*
	 * Library for background detection from following paper.
	 * O. Barnich and M. Van Droogenbroeck.
	 * ViBe: A universal background subtraction algorithm for video sequences.
	 * In IEEE Transactions on Image Processing, 20(6):1709-1724, June 2011.
	*/
	vibeModel_t *model = libvibeModelNew();

	/* Allocates memory to store the input images and the segmentation maps */
	IplImage *gray = cvCreateImage(cvSize(width, height),
			IPL_DEPTH_8U, 1);
	IplImage *temp1 = cvCreateImage(cvSize(width, height),
			IPL_DEPTH_8U, 1);
	IplImage *temp2 = cvCreateImage(cvSize(width, height),
			IPL_DEPTH_8U, 1);

	/* Acquires your first image */
	acquire_grayscale_image(stream, gray);

	/* Allocates the model and initialize it with the first image */
	libvibeModelAllocInit_8u_C1R(model, gray->imageData, width, height,
			stride);

	/* Processes all the following frames of your stream:
		results are stored in "segmentation_map" */
	while(!acquire_grayscale_image(stream, gray)){
		/* Get FG image in temp1 */
		libvibeModelUpdate_8u_C1R(model, gray->imageData,
				temp1->imageData);
		/*
		 * Clean all small unnecessary FG objects. Get cleaned
		 * one in temp2
		 */
		cvErode(temp1, temp2, NULL, 2);
		/* Dilate it to get in proper shape */
		cvDilate(temp2, temp1, NULL, 1);
		/*
		 * Find out all moving contours , so basically segment
		 * it out. Create separte image for each moving object.
		 */
		 nc = cvFindContours(temp1, storage, &contours,
				sizeof(CvContour), CV_RETR_EXTERNAL,
				CV_CHAIN_APPROX_NONE, cvPoint(0, 0));
		for (c = contours; c != NULL; c = c->h_next) {
			float area = cvContourArea(c, CV_WHOLE_SEQ, 0);
			char name[30];

			/*
			 * choose only if moving object area is greater
			 * than MIN_AREA square pixel
			 */
			if (area > MIN_AREA) {
				cvZero(temp2);
				cvDrawContours(temp2, c, cvScalar(255, 255, 255, 0), 
						cvScalar(0, 0, 0, 0), -1,
						CV_FILLED, 8, cvPoint(0, 0));
				cvWaitKey(30);
				cvShowImage("TEST", temp2);
			}
		}
	}

	/* Cleanup allocated memory */
	libvibeModelFree(model);
	cvReleaseImage(&gray);
	cvReleaseImage(&temp1);
	cvReleaseImage(&temp2);
	cvReleaseMemStorage(&storage);

	return(0);
}
