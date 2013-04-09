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

static int skeltonize_line(IplImage *img)
{
	IplImage *skel, *eroded, *temp;
	IplConvKernel *element;
	int ret = 0;
	bool done;
	CvMemStorage* storage;
	CvSeq *line_seq;
	int H[9] = {1, 1, 1, 1, 1, 1, 1, 1, 1};
	int i, j, count;
	struct lines_final final[20];
	cvNamedWindow("TEST", 1);

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
#if 0
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
	line_seq = cvHoughLines2(skel, storage, CV_HOUGH_PROBABILISTIC, 1, CV_PI/180, 10, 1, 10);
	for(int i = 0; i < line_seq->total; i++) {
		CvPoint* line = (CvPoint*)cvGetSeqElem(line_seq, i);
		cvLine(img, line[0], line[1], cvScalar(255, 0, 0, 0), 1, 8, 0);
	}
	cvShowImage("TEST", img);
	cvWaitKey(0);
#if 0
	printf("%d\n", line_seq->total);
	count = 0;
	for(int i = 0; i < line_seq->total; i++ ) {
		CvPoint* line = (CvPoint*)cvGetSeqElem(line_seq, i);
		CvPoint pt1 = line[0];
		CvPoint pt2 = line[1];
		float m, c;

		if (pt1.x == pt2.x)
			pt1.x++;
		m = atan((double)(pt1.y - pt2.y) /
				(pt1.x - pt2.x)) * 180 / CV_PI;
		if (m < 0)
			m += 180;
		c = pt1.y - tan(m * CV_PI /180) * pt1.x;
		//printf("%f\t%f\n", m, c);
		for (j = 0; j < count; j++) {
			/*
			if (abs(m - final[j].m) < 2) {
				int x1, x2, x3, x4, y1, y2, y3, y4, mi, ma;
				x1 = line[0].x;
				y1 = line[0].y;
				x2 = line[1].x;
				y2 = line[1].y;
				x3 = final[j].pt1.x;
				y3 = final[j].pt1.y;
				x4 = final[j].pt2.x;
				y4 = final[j].pt2.y;
				mi = MIN(x1, x2);
				mi = MIN(mi, x3);
				mi = MIN(mi, x4);
				ma = MAX(x1, x2);
				ma = MAX(mi, x3);
				ma = MAX(mi, x4);
				if (mi == x1) {
					final[j].pt1.x = x1;
					final[j].pt1.y = y1;
				} else if (mi == x2) {
					final[j].pt1.x = x2;
					final[j].pt1.y = y2;
				} else if (mi == x4) {
					final[j].pt1.x = x4;
					final[j].pt1.y = y4;
				}
				if (ma == x1) {
					final[j].pt2.x = x1;
					final[j].pt2.y = y1;
				} else if (ma == x2) {
					final[j].pt2.x = x2;
					final[j].pt2.y = y2;
				} else if (ma == x3) {
					final[j].pt2.x = x3;
					final[j].pt2.y = y3;
				}
				final[j].m = atan((double)(final[j].pt1.y - final[j].pt2.y) /
						(final[j].pt1.x - final[j].pt2.x)) * 180 / CV_PI;
				final[j].c = final[j].pt1.y - tan(m * CV_PI /180) * final[j].pt1.x;
				break;
			} */
		}
		if (j == count) {
			final[j].pt1.x = line[0].x;
			final[j].pt1.y = line[0].y;
			final[j].pt2.x = line[1].x;
			final[j].pt2.y = line[1].y;
			final[j].m = m;
			final[j].c = c;
			count++;
		}
	}
	printf("%d\n", count);
	for(i = 0; i < count; i++) {
		printf("%f\t%f\n", final[i].m, final[i].c);
		cvLine(img, final[i].pt1, final[i].pt2, cvScalar(255, 0, 0, 0), 1, 8, 0 );
	}
	cvShowImage("TEST", img);
	cvWaitKey(0);
#endif
#if 0
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
	static int count = 0;
	int j = 0, i;
	int area, cx, cy;
	char name[30];

//	cvNamedWindow("TEST1", 1);
	IplImage *temp = cvCreateImage(cvSize(img->width, img->height),
			IPL_DEPTH_8U, 1);
	if (!temp) {
		printf("No space for temp image creation\n");
		return -1;
	}

#if 0
	IplImage *temp1 = cvCreateImage(cvSize(img->width, img->height),
			IPL_DEPTH_32F, 1);
	IplImage *temp2 = cvCreateImage(cvSize(img->width, img->height),
			IPL_DEPTH_8U, 1);
#endif
	cvFindContours(img, storage, &contours,
			sizeof(CvContour), CV_RETR_TREE,
			CV_CHAIN_APPROX_SIMPLE, cvPoint(0, 0));
	if (contours)
//		contours = cvApproxPoly(contours, sizeof(CvContour), storage, CV_POLY_APPROX_DP, 3, 1 );

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
#if 0
			cvDistTransform(temp, temp1, CV_DIST_L2, 5, NULL,
					NULL);
			cvNormalize(temp1, temp1, 0.0, 1.0, CV_MINMAX);
			cvShowImage("TEST1", temp1);
#endif
			skeltonize_line(temp);
			//cvConvertScale(temp1, temp2);
			//cvAdd(temp2, temp, temp, NULL);
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
			cvCircle(temp, cvPoint(cx, cy), 5, cvScalar(255,
						0, 0, 0), 1, 8);
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
