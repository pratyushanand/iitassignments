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

static int hit_or_miss(IplImage *img, IplConvKernel *element1,
		IplConvKernel *element2)
{
	int ret = 0;
	IplImage *temp1 = cvCreateImage(cvSize(img->width, img->height), IPL_DEPTH_8U, 1);
	if (!temp1) {
		printf("Could not allocate memory for skel image\n");
		return -1;
	}
	IplImage *temp2 = cvCreateImage(cvSize(img->width, img->height), IPL_DEPTH_8U, 1);
	if (!temp2) {
		printf("Could not allocate memory for skel image\n");
		ret = -1;
		goto hmt_free;
	}
	cvNormalize(img, img, 0, 1, CV_MINMAX);
	cvErode(img, temp1, element1, 1);
	cvNot(img, img);
	cvErode(img, temp2, element2, 1);
	cvAnd(temp1, temp2, img, NULL);
hmt_free:
	if (temp1)
		cvReleaseImage(&temp1);
	if (temp2)
		cvReleaseImage(&temp2);
	return ret;
}

static int prune (IplImage *img)
{
	IplImage *temp, *temp1, *temp2, *temp3;
	int ret = 0;
	IplConvKernel *elementb[8], *elementc[8];
	IplConvKernel *elementh;
	int H[9] = {1, 1, 1, 1, 1, 1, 1, 1, 1};
	int B[8][9] = {
		{0, 0, 0,
		1, 1, 0,
		0, 0, 0},

		{0, 1, 0,
		0, 1, 0,
		0, 0, 0},

		{0, 0, 0,
		0, 1, 1,
		0, 0, 0},

		{0, 0, 0,
		0, 1, 0,
		0, 1, 0},

		{1, 0, 0,
		0, 1, 0,
		0, 0, 0},

		{0, 0, 1,
		0, 1, 0,
		0, 0, 0},

		{0, 0, 0,
		0, 1, 0,
		0, 0, 1},

		{0, 0, 0,
		0, 1, 0,
		1, 0, 0},
	};
	int C[8][9] = {
		{1, 1, 1,
		0, 0, 1,
		1, 1, 1},

		{1, 0, 1,
		1, 0, 1,
		1, 1, 1},

		{1, 1, 1,
		1, 0, 0,
		1, 1, 1},

		{1, 1, 1,
		1, 0, 1,
		1, 0, 1},

		{0, 1, 1,
		1, 0, 1,
		1, 1, 1},

		{1, 1, 0,
		1, 0, 1,
		1, 1, 1},

		{1, 1, 1,
		1, 0, 1,
		1, 1, 0},

		{1, 1, 1,
		1, 0, 1,
		0, 1, 1},
	};
	temp1 = cvCreateImage(cvSize(img->width, img->height), IPL_DEPTH_8U, 1);
	if (!temp1) {
		printf("Could not allocate memory for skel image\n");
		return -1;
	}
	temp2 = cvCreateImage(cvSize(img->width, img->height), IPL_DEPTH_8U, 1);
	if (!temp2) {
		printf("Could not allocate memory for skel image\n");
		ret = -1;
		goto prune_free;
	}
	temp3 = cvCreateImage(cvSize(img->width, img->height), IPL_DEPTH_8U, 1);
	if (!temp3) {
		printf("Could not allocate memory for skel image\n");
		ret = -1;
		goto prune_free;
	}
	temp = cvCreateImage(cvSize(img->width, img->height), IPL_DEPTH_8U, 1);
	if (!temp) {
		printf("Could not allocate memory for skel image\n");
		ret = -1;
		goto prune_free;
	}
	for (int i = 0; i < 8; i++) {
		elementb[i] = cvCreateStructuringElementEx(3, 3, 1, 1, CV_SHAPE_CUSTOM, B[0]);
		if (!elementb[0]) {
			printf("Could not create structuring element\n");
			ret = -1;
			goto prune_free;
		}
	}
	for (int i = 0; i < 8; i++) {
		elementc[i] = cvCreateStructuringElementEx(3, 3, 1, 1, CV_SHAPE_CUSTOM, C[0]);
		if (!elementc[0]) {
			printf("Could not create structuring element\n");
			ret = -1;
			goto prune_free;
		}
	}

	elementh = cvCreateStructuringElementEx(3, 3, 1, 1, CV_SHAPE_CUSTOM, H);
	if (!elementh) {
		printf("Could not create structuring element\n");
		ret = -1;
		goto prune_free;
	}
	cvCopy(img, temp1);
	hit_or_miss(temp1, elementb[0], elementc[0]);
	cvSub(img, temp1, temp1, NULL);
	/*now we have x1 in temp1 */
	for (int k = 1; k < 8; k++) {
		cvCopy(temp1, temp);
		hit_or_miss(temp, elementb[0], elementc[0]);
		cvOr(temp2, temp, temp2, NULL);
	}
	/*now we have x2 in temp2 */
	cvDilate(temp2, temp3, elementh, 1);
	cvAnd(temp3, img, temp3);
	/*now we have x3 in temp3 */
	cvOr(temp1, temp3, img);

prune_free:
	if (temp1)
		cvReleaseImage(&temp1);
	if (temp2)
		cvReleaseImage(&temp2);
	if (temp3)
		cvReleaseImage(&temp3);
	if (temp)
		cvReleaseImage(&temp);
	if (elementh)
		cvReleaseStructuringElement(&elementh);
	for (int i = 0; i < 8; i++)
		if (elementb[i])
			cvReleaseStructuringElement(&elementb[i]);
	for (int i = 0; i < 8; i++)
		if (elementc[i])
			cvReleaseStructuringElement(&elementc[i]);
	return ret;
}

static int skeltonize_line(IplImage *img)
{
	IplImage *skel, *eroded, *temp;
	IplConvKernel *element;
	int ret = 0;
	bool done;
	CvMemStorage* storage;
	CvSeq *line_seq;
	int H[9] = {1, 1, 1, 1, 1, 1, 1, 1, 1};
	int i, k;
//	cvNamedWindow("TEST", 1);
//	cvNamedWindow("TEST1", 1);
//	cvNamedWindow("TEST2", 1);

	skel = cvCreateImage(cvSize(img->width, img->height), IPL_DEPTH_8U, 1);
	if (!skel) {
		printf("Could not allocate memory for skel image\n");
		return -1;
	}
	cvZero(skel);

	eroded = cvCreateImage(cvSize(img->width, img->height), IPL_DEPTH_8U, 1);
	if (!eroded) {
		printf("Could not allocate memory for eroded image\n");
		ret = -1;
		goto skel_free;
	}
	temp = cvCreateImage(cvSize(img->width, img->height), IPL_DEPTH_8U, 1);
	if (!temp) {
		printf("Could not allocate memory for temp image\n");
		ret = -1;
		goto skel_free;
	}

	//element = cvCreateStructuringElementEx(3, 3, 1, 1, CV_SHAPE_CROSS, NULL);
	element = cvCreateStructuringElementEx(3, 3, 1, 1, CV_SHAPE_CUSTOM, H);
	if (!element) {
		printf("Could not create structuring element\n");
		ret = -1;
		goto skel_free;
	}

	storage = cvCreateMemStorage(0);

	if (!storage) {
		printf("Could not create storage area\n");
		ret = -1;
		goto skel_free;
	}

	cvThreshold(img, img, 127, 255, CV_THRESH_BINARY);

	do
	{
		cvErode(img, eroded, element, 1);
//		cvShowImage("TEST", eroded);
		cvDilate(eroded, temp, element, 1);
//		cvShowImage("TEST1", temp);
		cvSub(img, temp, temp, NULL);
		cvOr(skel, temp, skel, NULL);
		cvCopy(eroded, img);

		done = (cvCountNonZero(img) == 0);
//		cvShowImage("TEST2", skel);
//		cvWaitKey(0);
	} while (!done);
#if 0
		for(i = 0; i < skel->height;i++) {
			uchar *ptr = (uchar*)(skel->imageData + i * skel->widthStep);
			for(k = 0; k < skel->width;k++) {
				if (*(ptr+k))
				printf("%d ", *(ptr + k));
			}
			printf("\n");
		}

#endif
#if 1
	cvCopy (skel, img);
#else
	cvZero(img);
#if 0
	line_seq = cvHoughLines2( skel, storage, CV_HOUGH_STANDARD, 1, CV_PI/180, 500, 0, 0 );

		printf("\n");
	for(int i = 0; i < line_seq->total; i++ ) {
		float* line = (float*) cvGetSeqElem( line_seq , i );
	        float rho = line[0];
		float theta = line[1];
		CvPoint pt1, pt2;
		double a = cos(theta), b = sin(theta);
		double x0 = a*rho, y0 = b*rho;

//		printf("%f %f \n", rho, theta);
		pt1.x = cvRound(x0 + 1000*(-b));
		pt1.y = cvRound(y0 + 1000*(a));
		pt2.x = cvRound(x0 - 1000*(-b));
		pt2.y = cvRound(y0 - 1000*(a));

		cvLine(img, pt1, pt2, CV_RGB(255,255,255), 1, 8, 0 );
	}
#endif
	line_seq = cvHoughLines2(skel, storage, CV_HOUGH_PROBABILISTIC, 1, CV_PI/180, 100, 20, 1);
	for(int i = 0; i < line_seq->total; i++ ) {
		CvPoint* line = (CvPoint*)cvGetSeqElem(line_seq,i);
		cvLine(img, line[0], line[1], CV_RGB(255, 255, 255), 1, 8, 0 );
	}

	for(int i = 0; i < line_seq->total; i++ ) {
		float* line = (float*) cvGetSeqElem( line_seq , i );
	        float rho = line[0];
		float theta = line[1];
		CvPoint pt1, pt2;
		double a = cos(theta), b = sin(theta);
		double x0 = a*rho, y0 = b*rho;

//		printf("%f %f \n", rho, theta);
		pt1.x = cvRound(x0 + 1000*(-b));
		pt1.y = cvRound(y0 + 1000*(a));
		pt2.x = cvRound(x0 - 1000*(-b));
		pt2.y = cvRound(y0 - 1000*(a));

		cvLine(img, pt1, pt2, CV_RGB(255,255,255), 1, 8, 0 );
	}
#endif

skel_free:
	if (storage)
		cvReleaseMemStorage(&storage);
	if (element)
		cvReleaseStructuringElement(&element);
	if (eroded)
		cvReleaseImage(&eroded);
	if (temp)
		cvReleaseImage(&temp);
	if (skel)
		cvReleaseImage(&skel);
	return ret;
}

int find_endpoints(IplImage *img)
{
	int i, j, count;
	uchar *ptri, *ptrt;
	CvSeq *contours = NULL;
	CvSeq *c = NULL;
	CvPoint2D32f points[1000];
	CvPoint pt1, pt2;
	float line[4];
	float d, t;
	CvRect rect;
	CvMemStorage* storage = cvCreateMemStorage(0);
	IplImage *temp = cvCreateImage(cvSize(img->width, img->height),
			IPL_DEPTH_8U, 1);

	cvNamedWindow("TEST", 1);

	cvFindContours(img, storage, &contours,
			sizeof(CvContour), CV_RETR_TREE,
			CV_CHAIN_APPROX_SIMPLE, cvPoint(0, 0));
	cvZero(temp);
	for (c = contours; c != NULL; c = c->h_next) {
#if 1
		rect=cvBoundingRect (c, 0);
		count = 0;
		for (i = rect.x; i < rect.x + rect.width; i++) {
			for (j = rect.y; j < rect.y + rect.height; j++) {
				ptri = (uchar*) (img->imageData + j * img->widthStep);
				if (*(ptri + i)) {
					ptrt = (uchar*) (temp->imageData + j * temp->widthStep);
					//*(ptrt + i) = 255;
					points[count].x += i;
					points[count++].y += j;
					printf("%d\t%d\n", i, j);
				}
			}
		}
		printf("%d\n", count);
		cvFitLine2D(points, count, CV_DIST_L12, 0, 1, 1, line);
		d = sqrt((double)line[0]*line[0] + (double)line[1]*line[1]);
		line[0] /= d;
		line[1] /= d;
		t = (float)(img->width + img->height);
		pt1.x = cvRound(line[2] - line[0]*t);
		pt1.y = cvRound(line[3] - line[1]*t);
		pt2.x = cvRound(line[2] + line[0]*t);
		pt2.y = cvRound(line[3] + line[1]*t);
		cvLine(temp, pt1, pt2, cvScalar(255, 0, 0, 0), 1, CV_AA, 0);
#endif
#if 0

		if (c->total > 1000) {
			printf("Too many points\n");
			exit(-1);
		}
		for (i = 0; i < c->total; i++) {
			CvPoint* p = CV_GET_SEQ_ELEM(CvPoint, c,
					i);
			points[i].x += p->x;
			points[i].y += p->y;
			printf("%d %d \n", p->x, p->y);
		}
		pointMat = cvMat (1, c->total, CV_32SC2, points);
		cvFitLine2D(points, c->total, CV_DIST_L1, 0, 0.001, 0.001, line);
		d = sqrt((double)line[0]*line[0] + (double)line[1]*line[1]);
		line[0] /= d;
		line[1] /= d;
		t = (float)(img->width + img->height);
		pt1.x = cvRound(line[2] - line[0]*t);
		pt1.y = cvRound(line[3] - line[1]*t);
		pt2.x = cvRound(line[2] + line[0]*t);
		pt2.y = cvRound(line[3] + line[1]*t);
		cvLine(temp, pt1, pt2, cvScalar(255, 0, 0, 0), 1, CV_AA, 0);
#endif
#if 0
		rect=cvBoundingRect (c, 0);
		printf("%d\t%d\t%d\t%d\n", rect.x, rect.y, rect.width,
				rect.height);
		//if(cvArcLength(c, CV_WHOLE_SEQ, 0) <= 1) {
		cvRectangle(temp, cvPoint(rect.x, rect.y),
					cvPoint(rect.x + rect.width, rect.y + rect.height), cvScalar(255,0,0,0));
	//	}
#endif

		cvShowImage("TEST", temp);
		cvWaitKey(0);
	}
	cvCopy(temp, img);
find_ep_free:
	if (storage)
		cvReleaseMemStorage(&storage);
	if (temp)
		cvReleaseImage(&temp);
}
int main(int argc, char **argv)
{
	/* Your video stream */
	IplImage *img = cvLoadImage(argv[1], CV_LOAD_IMAGE_GRAYSCALE);
	skeltonize_line(img);
	cvSaveImage("skel.jpg", img, 0);
	//find_endpoints(img);

//	prune(img);
//	cvSaveImage("prune.jpg", img, 0);
//	cvReleaseImage(&img);
	return(0);
}
