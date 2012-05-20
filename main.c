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
	IplImage *gray, *temp1, *temp2, *temp3, *temp4, *map, *skel, *eroded,
		 *dilated, *seprated, *edged;
	IplConvKernel* str_ele = cvCreateStructuringElementEx(3, 3, 2,
			2, CV_SHAPE_ELLIPSE, NULL);
	float area;

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
	temp1 = cvCreateImage(cvSize(width, height), IPL_DEPTH_8U, 1);
	temp2 = cvCreateImage(cvSize(width, height), IPL_DEPTH_8U, 1);
	temp3 = cvCreateImage(cvSize(width, height), IPL_DEPTH_8U, 1);
	temp4 = cvCreateImage(cvSize(width, height), IPL_DEPTH_8U, 1);

	/* Acquires your first image */
	gray = temp2;
	acquire_grayscale_image(stream, gray);

	/* Allocates the model and initialize it with the first image */
	libvibeModelAllocInit_8u_C1R(model, gray->imageData, width, height,
			stride);
	/* Processes all the following frames of your stream:
		results are stored in "segmentation_map" */
	while(!acquire_grayscale_image(stream, gray)){
		/* Get FG image in temp1 */
		map = temp1;
		libvibeModelUpdate_8u_C1R(model, gray->imageData,
				map->imageData);
		/*
		 * Clean all small unnecessary FG objects. Get cleaned
		 * one in temp2
		 */
		eroded = temp2;
		cvErode(map, eroded, NULL, 2);
		/* Dilate it to get in proper shape */
		dilated = temp1;
		cvDilate(eroded, dilated, NULL, 1);
		/*
		 * Find out all moving contours , so basically segment
		 * it out. Create separte image for each moving object.
		 */
		 nc = cvFindContours(dilated, storage, &contours,
				sizeof(CvContour), CV_RETR_EXTERNAL,
				CV_CHAIN_APPROX_NONE, cvPoint(0, 0));
		for (c = contours; c != NULL; c = c->h_next) {
			area = cvContourArea(c, CV_WHOLE_SEQ, 0);

			/*
			 * choose only if moving object area is greater
			 * than MIN_AREA square pixel
			 */
			if (area > MIN_AREA) {
				seprated = temp2;
				cvZero(seprated);
				cvDrawContours(seprated, c, cvScalar(255, 255, 255, 0), 
						cvScalar(0, 0, 0, 0), -1,
						CV_FILLED, 8, cvPoint(0, 0));
#if 1
				/*
				 * So , now we have completely seprated
				 * moving objects in temp2
				 * Now, try to skeltonize it
				 */
				eroded = temp1;
				dilated = temp3;
				edged = temp3;
				skel = temp4;
				cvZero(skel);
				do {
					cvErode(seprated, eroded, str_ele, 1);
					cvDilate(eroded, dilated, str_ele, 1);
					cvSub(seprated, dilated, edged, NULL);
					cvOr(skel, edged, skel, NULL);
					cvCopy(seprated, eroded, NULL);
					printf("T");
				} while (!cvNorm(seprated, NULL, CV_L2, NULL));
#endif
				printf("\n");
				cvWaitKey(30);
				cvShowImage("TEST", skel);
			}
		}
				gray = temp2;
	}
	/* Cleanup allocated memory */
	libvibeModelFree(model);
	cvReleaseImage(&temp1);
	cvReleaseImage(&temp2);
	cvReleaseImage(&temp3);
	cvReleaseImage(&temp4);
	cvReleaseMemStorage(&storage);
	cvReleaseStructuringElement(&str_ele);

	return(0);
}
