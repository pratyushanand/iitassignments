//#include "vibe-background.h"
#include <cv.h>
#include <cxtypes.h>
#include <highgui.h>
#include <sys/queue.h>
#include <error.h>
#include "Timer.h"

#define MIN_AREA 300

//#define pr_debug(args...) fprintf(stdout, ##args)
#define pr_debug(args...)
#define pr_info(args...) fprintf(stdout, ##args)
#define pr_error(args...) fprintf(stdout, ##args)

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
	pr_error("Error in %s\n", __func__);
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

#define MAX_BOUNDARY_POINT 200
int main(int argc, char **argv)
{
	CTimer Timer;
	char msg[2048];
	CvCapture* stream;
	CvMemStorage* storage;
	int32_t width, height, stride;
	CvSeq* contours = NULL;
	CvSeq* c = NULL;
	IplImage *gray, *map, *eroded, *dilated;
	int frame = 0, ret;
	float area;
	vibeModel_t *model;
	int count;
	IplImage *temp;
	CvMat* mat, *smat;
	int cx, cy, i, j;
	float di, di1, delta, delta1;
	CvPoint skel_pt_all[MAX_BOUNDARY_POINT];

	stream = cvCaptureFromFile(argv[1]);
	if (!stream) {
		pr_error("Could not create capture stream\n");
		ret = -1;
		goto cleanup;
	}
	storage = cvCreateMemStorage(0);
	if (!storage) {
		pr_error("Could not create storage space\n");
		ret = -1;
		goto cleanup;
	}
	width = get_image_width(stream);
	height = get_image_height(stream);
	stride = get_image_stride(stream);

	temp = cvCreateImage(cvSize(width, height), IPL_DEPTH_8U, 1);
	if (!temp) {
		pr_error("Error with memory allocation for temp image\n");
		ret = -1;
		goto cleanup;
	}
	mat = cvCreateMat(1, width * height, CV_32FC1);
	if (!mat) {
		pr_error("Error with memory allocation for mat\n");
		ret = -1;
		goto cleanup;
	}
	smat = cvCreateMat(1, width * height, CV_32FC1);
	if (!smat) {
		pr_error("Error with memory allocation for smat\n");
		ret = -1;
		goto cleanup;
	}
	cvNamedWindow("GRAY", 1);
	cvNamedWindow("DI", 1);

	model = libvibeModelNew();
	if (!model) {
		pr_error("Vibe Model could not be created\n");
		ret = -1;
		goto cleanup;
	}

	/* Allocates memory to store the input images and the segmentation maps */
	gray = cvCreateImage(cvSize(width, height), IPL_DEPTH_8U, 1);
	if (!gray) {
		pr_error("Error with memory allocation for gray image\n");
		ret = -1;
		goto cleanup;
	}
	map = cvCreateImage(cvSize(width, height), IPL_DEPTH_8U, 1);
	if (!map) {
		pr_error("Error with memory allocation for map image\n");
		ret = -1;
		goto cleanup;
	}
	eroded = cvCreateImage(cvSize(width, height), IPL_DEPTH_8U, 1);
	if (!eroded) {
		pr_error("Error with memory allocation for eroded image\n");
		ret = -1;
		goto cleanup;
	}
	dilated = cvCreateImage(cvSize(width, height), IPL_DEPTH_8U, 1);
	if (!dilated) {
		pr_error("Error with memory allocation for dilated image\n");
		ret = -1;
		goto cleanup;
	}

	/* Acquires your first image */
	acquire_grayscale_image(stream, gray);
	libvibeModelAllocInit_8u_C1R(model, gray);

	/* Processes all the following frames of your stream:
		results are stored in "segmentation_map" */
	while(!acquire_grayscale_image(stream, gray)) {
		Timer.Start();
		frame++;
		if (frame < 0)
			continue;
		cvShowImage("GRAY", gray);
		libvibeModelUpdate_8u_C1R(model, gray, map);
		/*
		 * Clean all small unnecessary FG objects. Get cleaned
		 * one in temp2
		 */
		cvErode(map, eroded, NULL, 1);
		/* Dilate it to get in proper shape */
		cvDilate(eroded, dilated, NULL, 1);
		/*
		 * Find out all moving contours , so basically segment
		 * it out. Create separte image for each moving object.
		 */
		 cvFindContours(dilated, storage, &contours,
				sizeof(CvContour), CV_RETR_EXTERNAL,
				CV_CHAIN_APPROX_NONE, cvPoint(0, 0));
		for (c = contours; c != NULL; c = c->h_next) {
			area = cvContourArea(c, CV_WHOLE_SEQ, 0);
			/*
			 * choose only if moving object area is greater
			 * than MIN_AREA square pixel
			 */
			if (area > MIN_AREA) {
				cx = 0;
				cy = 0;
				for (i = 0; i < c->total; i++) {
					CvPoint* p = CV_GET_SEQ_ELEM(CvPoint, c,
							i);
					cx += p->x;
					cy += p->y;
				}
				cx /= c->total;
				cy /= c->total;

				/* calculate distance vector */
				cvZero(mat);
				for (i = 0; i < c->total; i++) {
					CvPoint* p = CV_GET_SEQ_ELEM(CvPoint, c,
							i);
					*((float*)CV_MAT_ELEM_PTR(*mat, 0, i)) =
						sqrt(pow((p->x - cx), 2) +
								pow((p->y - cy), 2));
				}
				/* Low pass filter it */
				cvSmooth(mat, smat, CV_GAUSSIAN, 39, 1, 0, 0);
				di = CV_MAT_ELEM(*smat, float, 0, 0);
				di1 = CV_MAT_ELEM(*smat, float, 0, 1);
				delta = di1 - di;
				j = 0;
				for (i = 1; i < c->total; i++) {
					di = CV_MAT_ELEM(*smat, float, 0, i);
					di1 = CV_MAT_ELEM(*smat, float, 0,
							(i + 1) % c->total);
					delta1 = di1 - di;
					if ((delta >= 0 && delta1 < 0) ||
							(delta <= 0 && delta1 > 0)) {
						CvPoint* p = CV_GET_SEQ_ELEM(
								CvPoint, c, i);
						skel_pt_all[j].x = p->x;
						skel_pt_all[j++].y = p->y;
						if (j == MAX_BOUNDARY_POINT) {
							pr_error("More than %d boundary points\n", j);
							goto cleanup;
						}
					}
					delta = delta1;
				}

				/*plot disatance vector */
				cvZero(temp);
				for (i = 0; i < j; i++) {
					cvLine(temp, cvPoint(cx, cy),
							cvPoint(skel_pt_all[i].x,
								skel_pt_all[i].y),
							cvScalar(255, 255, 0), 1, 8);
				}
				cvShowImage("DI", temp);
				cvWaitKey(0);
			}
		}
	}
cleanup:
	if (temp)
		cvReleaseImage(&temp);
	/* Cleanup allocated memory */
	if (dilated)
		cvReleaseImage(&dilated);
	if (eroded)
		cvReleaseImage(&eroded);
	if (map)
		cvReleaseImage(&map);
	if (gray)
		cvReleaseImage(&gray);
	if (model)
		libvibeModelFree(&model);
	if (storage)
		cvReleaseMemStorage(&storage);
	if (stream)
		cvReleaseCapture(&stream);
	if (mat)
		cvReleaseMat(&mat);
	if (smat)
		cvReleaseMat(&smat);
	cvDestroyWindow("GRAY");
	cvDestroyWindow("DI");
	return ret;
}
