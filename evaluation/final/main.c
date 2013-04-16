#include "vibe-background.h"
#include <cv.h>
#include <cxtypes.h>
#include <highgui.h>
#include <sys/queue.h>

#define MIN_AREA 300

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
#if 0
filter()
{
	double a[9]={   1.0/9.0,1.0/9.0,1.0/9.0,
		1.0/9.0,1.0/9.0,1.0/9.0,
		1.0/9.0,1.0/9.0,1.0/9.0};
	CvMat k;
	cvInitMatHeader(&k, 3, 3, CV_64FC1, a);

	cvFilter2D(img ,dst,
			&k,cvPoint(-1,-1));
}
#endif
#define MAX_OBJECT	20
#define MAX_BOUNDARY_POINT 200
#define MAX_TRACK_HISTORY 100
struct tracker{
	int theta1[MAX_TRACK_HISTORY];
	int theta2[MAX_TRACK_HISTORY];
	int di[MAX_TRACK_HISTORY];
	int hy[MAX_TRACK_HISTORY];
	int cx[MAX_TRACK_HISTORY];
	int cy[MAX_TRACK_HISTORY];
	int cur;
	LIST_ENTRY(tracker) list;
};

static LIST_HEAD(trackerhead, tracker) tracker_head;

void update_tracker(int theta1, int theta2, int di, int hy, int
		cx, int cy)
{
	int found = 0;
	struct tracker *node;

//	printf("%d %d %d %d %d %d\n", theta1, theta2, di, hy, cx, cy);
	LIST_FOREACH(node, &tracker_head, list) {
//		printf("Matching object\n");
		if (abs(node->cx[node->cur] - cx) < 10 && abs(node->cy[node->cur] -cy) < 10) {
			node->cur++;
			node->theta1[node->cur] = theta1;
			node->theta2[node->cur] = theta2;
			node->di[node->cur] = di;
			node->hy[node->cur] = hy;
			node->cx[node->cur] = cx;
			node->cy[node->cur] = cy;
			found = 1;
			printf("object matched\n");
		}
	}
	if (!found) {
		printf("object not matched\n");
		/* this is a new object */
		node = (struct tracker *)malloc(sizeof(*node));
		node->theta1[0] = theta1;
		node->theta2[0] = theta2;
		node->di[0] = di;
		node->hy[0] = hy;
		node->cx[0] = cx;
		node->cy[0] = cy;
		node->cur = 0;
		LIST_INSERT_HEAD(&tracker_head, node, list);
	}
}

int main(int argc, char **argv)
{
	CvCapture* stream = cvCaptureFromFile(argv[1]);
	CvMemStorage* storage = cvCreateMemStorage(0);
	int32_t width = get_image_width(stream);
	int32_t height = get_image_height(stream);
	int32_t stride = get_image_stride(stream);
	CvSeq* contours = NULL;
	CvSeq* c = NULL;
	IplImage *gray, *temp1, *temp2, *map, *skel, *eroded,
		 *dilated, *dii;
	float area, di, di1, delta, delta1;
	int frame = 0, object = 0, i, j, cx, cy, hy, wx, dmin, dmin_prev;
	CvMat* mat = cvCreateMat(1, width * height, CV_32FC1);
	CvMat* smat = cvCreateMat(1, width * height, CV_32FC1);
	CvRect rect;
	IplConvKernel *element;
	CvPoint skel_pt_all[MAX_BOUNDARY_POINT];
	CvPoint skel_pt_final[5];
	double theta1, theta2;

	LIST_INIT(&tracker_head);
	cvNamedWindow("GRAY", 1);
	cvNamedWindow("FG", 1);
	cvMoveWindow("FG", width, 0);
	cvNamedWindow("CLEANED", 1);
	cvMoveWindow("CLEANED", 2 * width, 0);
	cvNamedWindow("SEPRATED_CONTOUR", 1);
	cvMoveWindow("SEPRATED_CONTOUR", 3 * width, 0);
	cvNamedWindow("DI", 1);
	cvMoveWindow("DI", width, 2 * height);
	cvNamedWindow("SKELTON", 1);
	cvMoveWindow("SKELTON", 0, 2 * height);
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
	libvibeModelAllocInit_8u_C1R(model, (const uint8_t *)gray->imageData, width, height,
			stride);
	element = cvCreateStructuringElementEx(5, 5, 2, 2, CV_SHAPE_RECT, NULL);

	/* Processes all the following frames of your stream:
		results are stored in "segmentation_map" */
	while(!acquire_grayscale_image(stream, gray)){
		frame++;
		if (frame < 200)
			continue;
		//printf("%d\n", frame);
		object = 0;
		cvShowImage("GRAY", gray);
		/* Get FG image in temp1 */
		map = temp1;
		libvibeModelUpdate_8u_C1R(model, (const uint8_t *)gray->imageData,
				(uint8_t *)map->imageData);
		cvShowImage("FG", map);
		/*
		 * Clean all small unnecessary FG objects. Get cleaned
		 * one in temp2
		 */
		eroded = temp2;
		cvErode(map, eroded, NULL, 1);
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
			//printf("test%d\n", frame);

			/*
			 * choose only if moving object area is greater
			 * than MIN_AREA square pixel
			 */
			if (area > MIN_AREA) {
				cvWaitKey(0);
//				if (frame == 551)
//					cvWaitKey(0);
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

#if 1
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
#if 0
				/*plot disatance vector */
				dii = temp1;
				cvZero(dii);
				for (i = 0; i < c->total; i++) {
					int x = CV_MAT_ELEM(*smat, float, 0, i);
					*((uchar*) (dii->imageData +
					(height - x) * dii->widthStep) + i) = 255;
				}
				cvShowImage("DI", dii);
#endif
				/* find extream points */
				skel = temp1;
				cvZero(skel);

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
							printf("Error with parameters\n");
							goto exit;
						}
//						cvLine(skel, *p, cvPoint(cx, cy),
//							cvScalar(255, 255, 255, 0), 1, 8, 0);
					}
					delta = delta1;
				}
				rect=cvBoundingRect (c, 0);
				hy = rect.height;
				wx = rect.width;
				/* Centroid of final skeleton */
				skel_pt_final[0].x = cx;
				skel_pt_final[0].y = cy;
				/* top left point of skeleton */
				dmin_prev = sqrt(pow(rect.height, 2) +
						pow(rect.width, 2));
				for (i = 0; i < j; i++) {
					dmin = sqrt(pow((rect.x - skel_pt_all[i].x), 2) +
								pow((rect.y - skel_pt_all[i].y), 2));
					if (dmin < dmin_prev) {
						skel_pt_final[1].x =
							skel_pt_all[i].x;
						skel_pt_final[1].y =
							skel_pt_all[i].y;
						dmin_prev = dmin;
					}
				}
				/* top right point of skeleton */
				dmin_prev = sqrt(pow(rect.height, 2) +
						pow(rect.width, 2));
				for (i = 0; i < j; i++) {
					dmin = sqrt(pow((rect.x + rect.width - skel_pt_all[i].x), 2) +
								pow((rect.y - skel_pt_all[i].y), 2));
					if (dmin < dmin_prev) {
						skel_pt_final[2].x =
							skel_pt_all[i].x;
						skel_pt_final[2].y =
							skel_pt_all[i].y;
						dmin_prev = dmin;
					}
				}
				/* bottom right point of skeleton */
				dmin_prev = sqrt(pow(rect.height, 2) +
						pow(rect.width, 2));
				for (i = 0; i < j; i++) {
					dmin = sqrt(pow((rect.x + rect.width - skel_pt_all[i].x), 2) +
								pow((rect.y + rect.height - skel_pt_all[i].y), 2));
					if (dmin < dmin_prev) {
						skel_pt_final[3].x =
							skel_pt_all[i].x;
						skel_pt_final[3].y =
							skel_pt_all[i].y;
						dmin_prev = dmin;
					}
				}
				/* bottom left point of skeleton */
				dmin_prev = sqrt(pow(rect.height, 2) +
						pow(rect.width, 2));
				for (i = 0; i < j; i++) {
					dmin = sqrt(pow((rect.x - skel_pt_all[i].x), 2) +
								pow((rect.y + rect.height - skel_pt_all[i].y), 2));
					if (dmin < dmin_prev) {
						skel_pt_final[4].x =
							skel_pt_all[i].x;
						skel_pt_final[4].y =
							skel_pt_all[i].y;
						dmin_prev = dmin;
					}
				}
#if 0
				for (i = 0; i < 5; i++) {
					*((uchar*) (skel->imageData +
						skel_pt_final[i].y * skel->widthStep) + skel_pt_final[i].x) = 255;
				}
#endif
				cvLine(skel, skel_pt_final[0], skel_pt_final[1],
						cvScalar(255, 0, 0, 0), 1, CV_AA,0);
				cvLine(skel, skel_pt_final[0], skel_pt_final[2],
						cvScalar(255, 0, 0, 0), 1, CV_AA,0);
				cvLine(skel, skel_pt_final[0], skel_pt_final[3],
						cvScalar(255, 0, 0, 0), 1, CV_AA,0);
				cvLine(skel, skel_pt_final[0], skel_pt_final[4],
						cvScalar(255, 0, 0, 0), 1, CV_AA,0);
				cvShowImage("SKELTON", skel);
				theta1 = atan2(skel_pt_final[2].y - skel_pt_final[0].y, skel_pt_final[2].x - skel_pt_final[0].x) * 180.0 / CV_PI -
					atan2(skel_pt_final[1].y - skel_pt_final[0].y, skel_pt_final[1].x - skel_pt_final[0].x) * 180.0 / CV_PI;
				theta2 = atan2(skel_pt_final[4].y - skel_pt_final[0].y, skel_pt_final[4].x - skel_pt_final[0].x) * 180.0 / CV_PI -
					atan2(skel_pt_final[3].y - skel_pt_final[0].y, skel_pt_final[3].x - skel_pt_final[0].x) * 180.0 / CV_PI;
				di = sqrt(pow(((skel_pt_final[3].x + skel_pt_final[4].x)/2 - cx), 2) +
								pow(((skel_pt_final[3].y + skel_pt_final[4].y)/2 - cy), 2));
				update_tracker((int)theta1, (int)theta2, (int)di, hy, cx, cy);
#endif
			}
		}
				gray = temp2;
	}
exit:
	/* Cleanup allocated memory */
	libvibeModelFree(model);
	cvReleaseImage(&temp1);
	cvReleaseImage(&temp2);
	cvReleaseMat(&mat);
	cvReleaseMat(&smat);
	cvReleaseMemStorage(&storage);

	return(0);
}
