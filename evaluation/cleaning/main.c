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
	CTimer Timer;
	char msg[2048];

	if (!temp) {
		printf("No space for temp image creation\n");
		return -1;
	}
	Timer.Start();
	/* Clean all small unnecessary FG objects. */
	cvErode(img, temp, NULL, 2);
	/* Dilate it to get in proper shape */
	cvDilate(temp, img, NULL, 1);
	Timer.Stop();
	Timer.PrintElapsedTimeMsg(msg);
	printf("%s\n", msg);

	cvReleaseImage(&temp);
}

#define MAX_SAMPLE	20
typedef struct vibeModel {
	int N;
	int R;
	int psimin;
	int phi;
	int width;
	int height;
	IplImage *sample[MAX_SAMPLE];
	int background;
	int foreground;
} vibeModel_t;

void libvibeModelFree(vibeModel_t **model)
{
	int i;

	if (!*model)
		return;
	for (i = 0; i < MAX_SAMPLE; i++) {
		if ((*model)->sample[i])
			cvReleaseImage(&(*model)->sample[i]);
	}
	free(*model);
	*model = NULL;
}

vibeModel_t * libvibeModelNew()
{
	vibeModel_t *model;

	model = (vibeModel_t *)malloc(sizeof(*model));
	if (!model)
		return NULL;
	return model;
}

int libvibeModelAllocInit_8u_C1R(vibeModel_t *model, IplImage *gray)
{
	int i;

	model->N = MAX_SAMPLE;
	model->R = 20;
	model->psimin = 2;
	model->phi = 16;
	model->width = gray->width;
	model->height = gray->height;
	model->background = 0;
	model->foreground = 255;
	for (i = 0; i < MAX_SAMPLE; i++) {
		model->sample[i] = cvCreateImage(cvSize(model->width, model->height), IPL_DEPTH_8U, 1);
		if (!model->sample[i])
			goto error_lib_init;
		cvCopy(gray, model->sample[i]);
	}
	return 0;
error_lib_init:
	printf("Error in %s\n", __func__);
	for (i = 0; i < MAX_SAMPLE; i++) {
		if (model->sample[i])
			cvReleaseImage(&model->sample[i]);
	}
	return -1;
}

int getRandomNumber(CvRNG rng, int seed)
{
	return (cvRandInt(&rng) % seed);
}

int getRandomNeighbrXCoordinate(CvRNG rng, int x, int width)
{
	int ng = getRandomNumber(rng, 8);
	int xng;

	switch (ng) {
	case 0:
	case 7:
	case 6:
		xng = x--;
		if (xng < 0)
			xng = 0;
		break;
	case 1:
	case 5:
		xng = x;
		break;
	case 2:
	case 3:
	case 4:
		xng = x++;
		if (xng >= width)
			xng = width - 1;
		break;
	}

	return xng;
}

int getRandomNeighbrYCoordinate(CvRNG rng, int y, int height)
{
	int ng = getRandomNumber(rng, 8);
	int yng;

	switch (ng) {
	case 0:
	case 1:
	case 2:
		yng = y--;
		if (yng < 0)
			yng = 0;
		break;
	case 3:
	case 7:
		yng = y;
		break;
	case 4:
	case 5:
	case 6:
		yng = y++;
		if (yng >= height)
			yng = height - 1;
		break;
	}

	return yng;
}

int libvibeModelUpdate_8u_C1R(vibeModel_t *model, IplImage *gray,
		IplImage *map)
{
	int width = model->width;
	int height = model->height;
	int psimin = model->psimin;
	int N = model->N;
	int R = model->R;
	int bg = model->background;
	int fg = model->foreground;
	int phi = model->phi;
	CvRNG rng = cvRNG(-1);
	uint8_t *gray_data, *sample_data, *map_data;
	int rand, xng, yng;

	map_data = (uint8_t *)map->imageData;
	gray_data = (uint8_t *)gray->imageData;

	cvZero(map);
	for (int x = 0 ; x < width ; x++) {
		for (int y = 0 ; y < height ; y++) {
			int count = 0, index = 0, dist = 0;
			/* compare pixel to the BG model */
			while ((count < psimin) && (index < N)) {
				sample_data = (uint8_t *)model->sample[index]->imageData;
				dist = abs(*(gray_data + y * height + x) - *(sample_data + y * height + x));
				if (dist < R)
					count++;
				index++;
			}
			/* classify pixel and update model */
			if (count >= psimin) {
				*(map_data + y * height + x) = bg;
				/* update current pixel model */
				rand = getRandomNumber(rng, phi - 1);
				if (rand == 0) {
					rand = getRandomNumber(rng, N - 1);
					sample_data = (uint8_t *)model->sample[rand]->imageData;
					*(sample_data + y * height + x) = *(gray_data + y * height + x);
				}
				/* update neighbouring pixel model */
				rand = getRandomNumber(rng, phi - 1);
				if (rand == 0) {
					xng = getRandomNeighbrXCoordinate(rng, x, width);
					yng = getRandomNeighbrYCoordinate(rng, y, height);
					rand = getRandomNumber(rng, N - 1);
					sample_data = (uint8_t *)model->sample[rand]->imageData;
					*(sample_data + yng * height + xng) = *(gray_data + y * height + x);
				}
			} else {
				*(map_data + y * height + x) = fg;
			}
		}
	}
}

int main(int argc, char **argv)
{
	/* Your video stream */
	CvCapture* stream = cvCaptureFromFile(argv[1]);
	int count = 0;
	char name[30];

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
	libvibeModelAllocInit_8u_C1R(model, gray);

	/* Processes all the following frames of your stream:
	 * results are stored in "segmentation_map"
	 */
	while(!acquire_grayscale_image(stream, gray)){
		libvibeModelUpdate_8u_C1R(model, gray, map);
		clean_fg(map);
		sprintf(name, "out/out%d.jpg", count++);
		cvSaveImage(name, map, 0);
	}
	/* Cleanup allocated memory */
	libvibeModelFree(&model);
	cvReleaseImage(&gray);
	cvReleaseImage(&map);
	return(0);
}
