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
		cvDilate(eroded, temp, element, 1);
		cvSub(img, temp, temp, NULL);
		cvOr(skel, temp, skel, NULL);
		cvCopy(eroded, img);

		done = (cvCountNonZero(img) == 0);
	} while (!done);
		for(i = 0; i < skel->height;i++) {
			uchar *ptr = (uchar*)(skel->imageData + i * skel->widthStep);
			for(k = 0; k < skel->width;k++) {
				if (*(ptr+k))
				printf("%d ", *(ptr + k));
			}
			printf("\n");
		}


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


int skeltonize(IplImage *img, char **argv)
{
	CvSeq *contours = NULL;
	CvSeq *c = NULL;
	CvMemStorage *storage = cvCreateMemStorage(0);
	IplImage *temp = cvCreateImage(cvSize(img->width, img->height),
			IPL_DEPTH_8U, 1);
	static int count = 0;
	int j = 0, i;
	int area, cx, cy;
	char name[30];

	if (!temp) {
		printf("No space for temp image creation\n");
		return -1;
	}

	cvNamedWindow("CONTOUR", 1);
	cvFindContours(img, storage, &contours,
			sizeof(CvContour), CV_RETR_TREE,
			CV_CHAIN_APPROX_SIMPLE, cvPoint(0, 0));
	if (contours)
		contours = cvApproxPoly(contours, sizeof(CvContour), storage, CV_POLY_APPROX_DP, 3, 1 );

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
					-1, 1, 8,
					cvPoint(0, 0));
			cvShowImage("CONTOUR", temp);
			cvWaitKey(10);
			//			skeltonize_line(temp);
#if 0
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
			printf("%d\n", c->total);
#endif
			//			CvRect rect=cvBoundingRect (c, 0);
			//			cvEllipseBox(temp, rect, cvScalar(255, 0, 0, 0), 2, 8, 0);
			//			cvRectangle(temp, cvPoint(rect.x, rect.y),
			//					cvPoint(rect.x + rect.width, rect.y + rect.height), cvScalar(255,0,0,0));
			sprintf(name, "out/out%d_%d.jpg", count, j);
			cvSaveImage(name, temp, 0);
		}
		j++;
	}
	cvReleaseImage(&temp);
	cvReleaseMemStorage(&storage);
	count++;
}

int main(int argc, char **argv)
{
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
		clean_fg(map);
		skeltonize(map, argv);
	}
	/* Cleanup allocated memory */
	libvibeModelFree(model);
	cvReleaseImage(&gray);
	cvReleaseImage(&map);
	return(0);
}
