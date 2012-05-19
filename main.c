#include "vibe-background.h"
#include <cv.h>
#include <cxtypes.h>
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
	CvCapture* stream = cvCaptureFromFile("../test_video/test.avi");
	CvMemStorage* storage = cvCreateMemStorage(0);
	CvSeq* contours = NULL;
	CvSeq* c = NULL;
	int count = 0;
	int nc = 0;
	int i;

	cvNamedWindow("ORIGINAL", 1);
//	cvNamedWindow("FG", 1);
	/* Get a model data structure */
	vibeModel_t *model = libvibeModelNew();

	/* Get the dimensions of the images of your stream 
nb: stride is te number of bytes from the start of one row of the image  
to the start of the next row. */
	int32_t width = get_image_width(stream);
	int32_t height = get_image_height(stream);
	int32_t stride = get_image_stride(stream);

	/* Allocates memory to store the input images and the segmentation maps */
	IplImage *gray = cvCreateImage(cvSize(width, height),
			IPL_DEPTH_8U, 1);
	IplImage *map = cvCreateImage(cvSize(width, height),
			IPL_DEPTH_8U, 1);
	IplImage *dilate = cvCreateImage(cvSize(width, height),
			IPL_DEPTH_8U, 1);
	IplImage *erode = cvCreateImage(cvSize(width, height),
			IPL_DEPTH_8U, 1);
	IplImage *edge = cvCreateImage(cvSize(width, height),
			IPL_DEPTH_8U, 1);
	IplImage *seg = cvCreateImage(cvSize(width, height),
			IPL_DEPTH_8U, 1);
	IplImage *skelton = cvCreateImage(cvSize(width, height),
			IPL_DEPTH_8U, 1);

	/* Acquires your first image */
	acquire_grayscale_image(stream, gray);

	/* Allocates the model and initialize it with the first image */
	libvibeModelAllocInit_8u_C1R(model, gray->imageData, width, height,
			stride);

	/* Processes all the following frames of your stream:
		results are stored in "segmentation_map" */
	while(!acquire_grayscale_image(stream, gray)){
		libvibeModelUpdate_8u_C1R(model, gray->imageData,
				map->imageData);
		cvErode(map, erode, NULL, 2);
		cvDilate(erode, dilate, NULL, 1);
		nc = cvFindContours(dilate, storage, &contours,
				sizeof(CvContour), CV_RETR_EXTERNAL,
				CV_CHAIN_APPROX_NONE, cvPoint(0, 0));
		for (c = contours; c != NULL; c = c->h_next) {
			float area = cvContourArea(c, CV_WHOLE_SEQ, 0);
			char name[30];

			if (area > 1000) {
				cvZero(skelton);
				cvDrawContours(skelton, c, cvScalar(255, 255, 255, 0), 
						cvScalar(0, 0, 0, 0), -1,
						CV_FILLED, 8, cvPoint(0, 0));
//				sprintf(name, "out%d.jpg", count++);
//				cvSaveImage(name, skelton, 0);
				cvWaitKey(30);
				cvShowImage("ORIGINAL", skelton);
			}
		}
#if 0
		while (nc--) {
			for (i = 0; i < contours->total; i++) {
				CvPoint* p = CV_GET_SEQ_ELEM(CvPoint, c,
						i);
				*((uchar*) (skelton->imageData +
				 p->y * skelton->widthStep) + p->x) =
					255;
				printf("%d %d %d\n", p->x, p->y, nc);
			}
			c = c->h_next;
			if (!c)
				break;
			cvShowImage("ORIGINAL", skelton);
		}
		printf("\n");
#endif
//		cvShowImage("FG", edge);
		cvWaitKey(30);
	}

	/* Cleanup allocated memory */
	libvibeModelFree(model);
	cvReleaseImage(&gray);
	cvReleaseImage(&map);
	cvReleaseImage(&erode);
	cvReleaseImage(&dilate);
	cvReleaseImage(&seg);
	cvReleaseImage(&skelton);
	cvReleaseMemStorage(&storage);

	return(0);
}
