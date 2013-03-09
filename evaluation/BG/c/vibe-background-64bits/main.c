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

int main(int argc, char **argv)
{
	/* Your video stream */
	CvCapture* stream = cvCaptureFromFile(argv[1]);
	int count = 0;
	char name[30];
	CTimer Timer;
	char msg[2048];

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
		Timer.Start();
		libvibeModelUpdate_8u_C1R(model, (const uint8_t*)gray->imageData, (uint8_t*)map->imageData);
		Timer.Stop();
		Timer.PrintElapsedTimeMsg(msg);
		printf("%s\n", msg);
		sprintf(name, "out/out%d.jpg", count++);
		cvSaveImage(name, map, 0);
	}

	/* Cleanup allocated memory */
	libvibeModelFree(model);
	cvReleaseImage(&gray);
	cvReleaseImage(&map);

	return(0);
}
