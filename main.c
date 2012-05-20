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
	IplImage *gray, *temp1, *temp2, *map, *skel, *eroded,
		 *dilated;
	float area, di, di1, delta, delta1;
	int frame = 0, object = 0, i, cx, cy;
	CvMat* mat = cvCreateMat(1, width * height, CV_32FC1);
	CvMat* smat = cvCreateMat(1, width * height, CV_32FC1);

	cvNamedWindow("GRAY", 1);
	cvNamedWindow("FG", 1);
	cvMoveWindow("FG", width, 0);
	cvNamedWindow("CLEANED", 1);
	cvMoveWindow("CLEANED", 2 * width, 0);
	cvNamedWindow("SEPRATED_CONTOUR", 1);
	cvMoveWindow("SEPRATED_CONTOUR", 3 * width, 0);
#ifndef HIGH_BW
	cvNamedWindow("SKELTON", 1);
	cvMoveWindow("SKELTON", 0, 2 * height);
#endif
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

	/* Acquires your first image */
	gray = temp2;
	acquire_grayscale_image(stream, gray);

	/* Allocates the model and initialize it with the first image */
	libvibeModelAllocInit_8u_C1R(model, gray->imageData, width, height,
			stride);
	/* Processes all the following frames of your stream:
		results are stored in "segmentation_map" */
	while(!acquire_grayscale_image(stream, gray)){
		frame++;
		object = 0;
		cvShowImage("GRAY", gray);
		/* Get FG image in temp1 */
		map = temp1;
		libvibeModelUpdate_8u_C1R(model, gray->imageData,
				map->imageData);
		cvShowImage("FG", map);
		/*
		 * Clean all small unnecessary FG objects. Get cleaned
		 * one in temp2
		 */
		eroded = temp2;
		cvErode(map, eroded, NULL, 2);
		/* Dilate it to get in proper shape */
		dilated = temp1;
		cvDilate(eroded, dilated, NULL, 1);
		cvShowImage("CLEANED", dilated);
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
				cvWaitKey(30);
				/*
				 * If bandwidth is high then send data
				 * at this stage only. No need to
				 * skeltonize
				 */
				cvZero(temp1);
				for (i = 0; i < c->total; i++) {
					CvPoint* p = CV_GET_SEQ_ELEM(CvPoint, c,
							i);
					*((uchar*) (temp1->imageData +
					p->y * temp1->widthStep) + p->x) = 255;
				}
				cvShowImage("SEPRATED_CONTOUR", temp1);
				/* calculate centroid */
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
						sqrt((p->x - cx)^2 +
								(p->y - cy)^2);
				}
				/* Low pass filter it */
				cvSmooth(mat, smat, CV_BLUR, 3, 0, 0, 0);
				/* find extream points */
				skel = temp1;
				cvZero(skel);

				*((uchar*) (skel->imageData +
					cy * skel->widthStep) + cx) = 255;
				di = CV_MAT_ELEM(*mat, float, 0, 0);
				di1 = CV_MAT_ELEM(*mat, float, 0, 1);
				delta = di1 - di;
				for (i = 1; i < c->total; i++) {
					di = CV_MAT_ELEM(*mat, float, 0, i);
					di1 = CV_MAT_ELEM(*mat, float, 0,
							(i + 1) % c->total);
					delta1 = di1 - di;
					if (delta >= 0 && delta1 < 0) {
						CvPoint* p = CV_GET_SEQ_ELEM(
								CvPoint, c, i);
						cvLine(skel, *p, cvPoint(cx, cy),
							cvScalar(255, 255, 255, 0), 1, 8, 0);
					}
					delta = delta1;
				}
				cvShowImage("SKELTON", skel);
			}
		}
				gray = temp2;
	}
	/* Cleanup allocated memory */
	libvibeModelFree(model);
	cvReleaseImage(&temp1);
	cvReleaseImage(&temp2);
	cvReleaseMat(&mat);
	cvReleaseMat(&smat);
	cvReleaseMemStorage(&storage);

	return(0);
}
