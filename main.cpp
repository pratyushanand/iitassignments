#include "vibe-background.h"
#include <cv.h>
#include <cxtypes.h>
#include "cvaux.h"
#include "highgui.h"

#define MIN_AREA 1000
#define DEBUG_GRAY_IMAGE	1
#define DEBUG_FG_IMAGE		0
#define DEBUG_CLEANED_IMAGE	0
#define DEBUG_SEPARATED_IMAGE	0
#define DEBUG_OUTPUT_IMAGE	1

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
	CvCapture *stream = NULL;
	CvMemStorage *storage;
	IplImage *gray, *temp1, *temp2, *map, *eroded, *dilated;
	CvSeq *contours = NULL;
	CvSeq *c = NULL;
	int32_t width, height, stride, cx, cy, i;
	float area;
	CvMat *mat, *smat;
	CvFont font;
	double hScale=1.0, vScale=1.0;
	int    lineWidth=1;



	if (argv[1])
		stream = cvCaptureFromFile(argv[1]);

	if (!stream) {
		printf("No video stream found\n");
		return -1;
	}

	cvInitFont(&font,CV_FONT_HERSHEY_SIMPLEX|CV_FONT_ITALIC, hScale,vScale,0,lineWidth);
	storage = cvCreateMemStorage(0);
	width = get_image_width(stream);
	height = get_image_height(stream);
	stride = get_image_stride(stream);
	mat = cvCreateMat(1, width * height, CV_32FC1);
	smat = cvCreateMat(1, width * height, CV_32FC1);

    	cv::HOGDescriptor hog;
	hog.setSVMDetector(cv::HOGDescriptor::getDefaultPeopleDetector());
    	cv::vector<cv::Rect> found;

#if DEBUG_GRAY_IMAGE
	cvNamedWindow("GRAY", 1);
#endif
#if DEBUG_FG_IMAGE
	cvNamedWindow("FG", 1);
	cvMoveWindow("FG", width, 0);
#endif
#if DEBUG_CLEANED_IMAGE
	cvNamedWindow("CLEANED", 1);
	cvMoveWindow("CLEANED", 2 * width, 0);
#endif
#if DEBUG_SEPARATED_IMAGE
	cvNamedWindow("SEPRATED_CONTOUR", 1);
	cvMoveWindow("SEPRATED_CONTOUR", 3 * width, 0);
#endif
#if DEBUG_OUTPUT_IMAGE
	cvNamedWindow("OUTPUT", 1);
	cvMoveWindow("OUTPUT", 4 * width, 0);
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
	libvibeModelAllocInit_8u_C1R(model, (const uint8_t*)gray->imageData, width, height,
			stride);
	/* Processes all the following frames of your stream:
		results are stored in "segmentation_map" */
	while(!acquire_grayscale_image(stream, gray)){
#if DEBUG_GRAY_IMAGE
		cvShowImage("GRAY", gray);
#endif
		/* Get FG image in temp1 */
		map = temp1;
		libvibeModelUpdate_8u_C1R(model, (const uint8_t*)gray->imageData,
				(uint8_t*)map->imageData);
#if DEBUG_FG_IMAGE
		cvShowImage("FG", map);
#endif
		/*
		 * Clean all small unnecessary FG objects. Get cleaned
		 * one in temp2
		 */
		eroded = temp2;
		cvErode(map, eroded, NULL, 2);
		/* Dilate it to get in proper shape */
		dilated = temp1;
		cvDilate(eroded, dilated, NULL, 1);
#if DEBUG_CLEANED_IMAGE
		cvShowImage("CLEANED", dilated);
#endif
		/*
		 * Find out all moving contours , so basically segment
		 * it out. Create separte image for each moving object.
		 */
		 cvFindContours(dilated, storage, &contours,
				sizeof(CvContour), CV_RETR_EXTERNAL,
				CV_CHAIN_APPROX_NONE, cvPoint(0, 0));
		cvZero(temp2);
		for (c = contours; c != NULL; c = c->h_next) {
			area = cvContourArea(c, CV_WHOLE_SEQ, 0);
			cvZero(temp1);

			/*
			 * choose only if moving object area is greater
			 * than MIN_AREA square pixel
			 */
			if (area > MIN_AREA) {
				cvWaitKey(10);
				cvDrawContours(temp1, c, cvScalar(255, 255, 255, 0),
							cvScalar(0, 0, 0, 0),
							-1, CV_FILLED, 8,
							cvPoint(0, 0));
#if DEBUG_SEPARATED_IMAGE
				cvShowImage("SEPRATED_CONTOUR", temp1);
#endif
    				hog.detectMultiScale(temp1, found, 0, cv::Size(8,8), cv::Size(24,16), 1.05, 2);
				if (found.size()) {
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
					cvPutText (temp2,"H",cvPoint(cx, cy), &font, cvScalar(255,255,0));
				}
			}
		}
#if DEBUG_OUTPUT_IMAGE
		cvShowImage("OUTPUT", temp2);
#endif
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
