/* 
   January 2010
   Olivier Barnich <o.barnich@ulg.ac.be> and
   Marc Van Droogenbroeck <m.vandroogenbroeck@ulg.ac.be>
   */

/*
   This file contains an example of a main functions that uses the ViBe algorithm
   implemented in vibe-background.{o, h}. You should read vibe-background.h for
   more information.

   vibe-background.o was compiled by gcc 4.0.3 using the following command
   $> gcc -ansi -Wall -Werror -pedantic -c vibe-background.c

   This file can be compiled using the following command
   $> gcc -o main -std=c99 your-main-file.c vibe-background.o

*/

#include "vibe-background.h"
#include "Timer.h"
#include <cv.h>
#include <highgui.h>

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

int clean_fg(IplImage *img)
{
	IplImage *temp = cvCreateImage(cvSize(img->width, img->height),
			IPL_DEPTH_8U, 1);

	if (!temp) {
		printf("No space for temp image creation\n");
		return -1;
	}
	/* Clean all small unnecessary FG objects. */
	cvErode(img, temp, NULL, 2);
	/* Dilate it to get in proper shape */
	cvDilate(temp, img, NULL, 1);

	cvReleaseImage(&temp);
}

struct lines_final {
	CvPoint pt1;
	CvPoint pt2;
	float m;
	float c;
};

int skeltonize(IplImage *img, char **argv)
{
	CvSeq *contours = NULL;
	CvSeq *c = NULL;
	CvMemStorage *storage = cvCreateMemStorage(0);
	static int count = 0;
	int j = 0, i;
	int area, cx, cy;
	char name[30];

	cvNamedWindow("TEST1", 1);
	IplImage *temp = cvCreateImage(cvSize(img->width, img->height),
			IPL_DEPTH_8U, 1);
	if (!temp) {
		printf("No space for temp image creation\n");
		return -1;
	}

	IplImage *temp1 = cvCreateImage(cvSize(img->width, img->height),
			IPL_DEPTH_32F, 1);
	cvFindContours(img, storage, &contours,
			sizeof(CvContour), CV_RETR_TREE,
			CV_CHAIN_APPROX_SIMPLE, cvPoint(0, 0));

	for (c = contours; c != NULL; c = c->h_next) {
		area = cvContourArea(c, CV_WHOLE_SEQ, 0);
		/*
		 * choose only if moving object area is greater
		 * than MIN_AREA square pixel
		 */
		if (area > atoi(argv[2])) {
			cvZero(temp);
			cvDrawContours(temp, c,
					cvScalar(255, 255, 255, 0),
					cvScalar(0, 0, 0, 0),
					-1, CV_FILLED, 8,
					cvPoint(0, 0));
			cvDistTransform(temp, temp1, CV_DIST_L2, 5, NULL,
					NULL);
			cvNormalize(temp1, temp1, 0.0, 1.0, CV_MINMAX);
			cvShowImage("TEST1", temp1);
			sprintf(name, "out/out%d_%d.jpg", count, j);
			cvSaveImage(name, temp1, 0);
			cvWaitKey(5);
		}
		j++;
	}
	cvReleaseImage(&temp);
	cvReleaseMemStorage(&storage);
	count++;
}

int main(int argc, char **argv)
{
	unsigned long long frame_count = 0;
	//cvNamedWindow("TEST2", 1);
	/* Your video stream */
	CvCapture* stream = cvCaptureFromFile(argv[1]);

	/* Get a model data structure */
	vibeModel_t *model = libvibeModelNew();

	/* Get the dimensions of the images of your stream 
nb: stride is te number of bytes from the start of one row of the image  
to the start of the next row. */
	int32_t width = get_image_width(stream);
	int32_t height = get_image_height(stream);
	int32_t stride = get_image_stride(stream);

	/* Allocates memory to store the input images and the segmentation maps */
	IplImage *map = cvCreateImage(cvSize(width, height),
			IPL_DEPTH_8U, 1);
	IplImage *gray = cvCreateImage(cvSize(width, height),
			IPL_DEPTH_8U, 1);

	/* Acquires your first image */
	acquire_grayscale_image(stream, gray);

	/* Allocates the model and initialize it with the first image */
	libvibeModelAllocInit_8u_C1R(model, (const uint8_t*)gray->imageData, width, height, stride);

	/* Processes all the following frames of your stream:
	 * results are stored in "segmentation_map"
	 */
	while(!acquire_grayscale_image(stream, gray)){
		libvibeModelUpdate_8u_C1R(model, (const uint8_t*)gray->imageData, (uint8_t*)map->imageData);
		if (frame_count++ < 200)
			continue;
		//cvShowImage("TEST2", gray);
		clean_fg(map);
		skeltonize(map, argv);
	}
	/* Cleanup allocated memory */
	libvibeModelFree(model);
	cvReleaseImage(&gray);
	cvReleaseImage(&map);
	return(0);
}