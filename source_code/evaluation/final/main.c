//#include "vibe-background.h"
#include <cv.h>
#include <highgui.h>
#include <sys/queue.h>
#include <error.h>
#include "Timer.h"

#define MIN_AREA 300
#define DEBUG_IMAGES
#define DEBUG_GRAY
//#define DEBUG_FG
//#define DEBUG_CLEANED
//#define DEBUG_SEPRATED_CONTOUR
//#define DEBUG_DI
//#define DEBUG_SKELTON
#define DEBUG_FINAL

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
#define MAX_OBJECT	50
#define MAX_BOUNDARY_POINT 200
#define MAX_TRACK_HISTORY 100
enum obj_type {
	TYPE_UNDEFINED = 0,
	TYPE_HUMAN = 1,
};

struct tracker{
	int theta[MAX_TRACK_HISTORY];
	int cx[MAX_TRACK_HISTORY];
	int cy[MAX_TRACK_HISTORY];
	int px;
	int py;
	int cur;
	enum obj_type type;
	LIST_ENTRY(tracker) list;
};
struct object_table {
	int obj_count;
	int cx[MAX_OBJECT];
	int cy[MAX_OBJECT];
	int theta[MAX_OBJECT];
	enum obj_type type[MAX_OBJECT];
	bool is_old[MAX_OBJECT];
};

static LIST_HEAD(trackerhead, tracker) tracker_head;

float compute_mean(int *data, int count)
{
	int i;
	float mean = 0;

	for (i = 0; i < count; i++)
		mean = mean + ((float)data[i] / (float)count);

	return mean;
}

float compute_variance(int *data, int count, float mean)
{
	int i;
	float var = 0;

	for (i = 0; i < count; i++)
		var = var + (pow(((float)data[i] - mean), 2) / (float)count);

	return var;
}

float compute_autocovariance(int *data, int count, int lag, int mean, int var)
{
	int i;
	float autocv = 0;

	for (i = 0; i < (count - lag); i++)
		autocv = autocv + (((float)data[i] - mean) * ((float)data[i+lag] - mean) / (float)count);

	return (autocv / var);
}

enum obj_type analyze_leg_motion(int *theta)
{
	int i, p[3], j = 0, m1, m2, n;
	int *t1, *t2;
	int numr, dnm1, dnm2;
	float corel;

	for (i = 0; i < MAX_TRACK_HISTORY; i++) {
		if (!theta[i]) {
			p[j++] = i;
			if (j == 3)
				break;
		}
	}
	if (j < 3)
		return TYPE_UNDEFINED;
	n = p[2] - p[1];
	t1 = &theta[p[1]];
	t2 = &theta[p[2]];
	m1 = compute_mean(t1, n);
	m2 = compute_mean(t2, n);
	numr = dnm1 = dnm2 =0;
	for (i = 0; i < n; i++)
		numr += (t1[i] - m1) * (t2[i] - m2);
	for (i = 0; i < n; i++)
		dnm1 += (t1[i] - m1) * (t1[i] - m1);
	for (i = 0; i < n; i++)
		dnm2 += (t2[i] - m2) * (t2[i] - m2);
	corel = numr / sqrt (dnm1 * dnm2);
	if (corel > 0.5)
		return TYPE_HUMAN;
	else
		return TYPE_UNDEFINED;
}

static void update_tracker(struct object_table *obj_table, int height,
		int width)
{
	struct tracker *node;
	int count, n_count, cx, cy, theta, dx, dy, x, y, dmin, dmin_prev, i;
	/* see if object was previously found.
	 * 1. Check if new centroid is nearby (at max distance D) to
	 * any centroid of previous frame. If not, then it might be a new
	 * object.
	 * 2. If there are more than one centroid of previous frame
	 * which is nearby to the current centroid, then release all
	 * nearby previous object and treat it as a new object.
	 */
	LIST_FOREACH(node, &tracker_head, list) {
		dmin_prev = pow(height, 2) + pow(width, 2);
		n_count = -1;
		x = node->px;
		y = node->py;
		for (count = 0; count < obj_table->obj_count; count++) {
			cx = obj_table->cx[count];
			cy = obj_table->cy[count];
			dmin = pow((x - cx), 2) + pow((y -cy), 2);
			if (dmin < dmin_prev)
				n_count = count;
			dmin_prev = dmin;
		}
		if (n_count == -1) {//TBD
			printf("\nBUG");
			while(1);
		}
		//pr_debug("Updating old object\n");
		cx = obj_table->cx[n_count];
		cy = obj_table->cy[n_count];
		theta = obj_table->theta[n_count];
		/* drift in x and y direction to predict next cx and cy
		 */
		dx = cx - node->cx[node->cur];
		dy = cy - node->cy[node->cur];
		if (!(!node->theta[node->cur]
					&& (!theta || !node->cur)))
			node->cur++;
		node->cur %= MAX_TRACK_HISTORY;
		node->theta[node->cur] = theta;
		node->cx[node->cur] = cx;
		node->cy[node->cur] = cy;
		node->px = cx + dx;
		node->py = cy + dy;
		if (node->type == TYPE_UNDEFINED)
			node->type = analyze_leg_motion(node->theta);
		obj_table->type[n_count] = node->type;
		obj_table->is_old[n_count] = true;
	}
	for (count = 0; count < obj_table->obj_count; count++) {
		if (!obj_table->is_old[count]) {
			pr_debug("Adding a new object\n");
			node = (struct tracker *)malloc(sizeof(*node));
			node->theta[0] = theta;
			for (i = 1; i < MAX_TRACK_HISTORY; i++)
				node->theta[i] = 180;
			node->px = node->cx[0] = obj_table->cx[count];
			node->py = node->cy[0] = obj_table->cy[count];
			node->cur = 0;
			node->type = TYPE_UNDEFINED;
			LIST_INSERT_HEAD(&tracker_head, node, list);
		}
	}
}

static void update_object_table(CvSeq *c, int width, int height,
		struct object_table *obj_table)
{
	int found = 0;
	CvMat* mat, *smat;
	CvPoint skel_pt_final[3];
	CvPoint skel_pt_all[MAX_BOUNDARY_POINT];
	double theta;
	float di, di1, delta, delta1;
	int i, j, cx, cy, x, y, hy, wx, dmin, dmin_prev, ret;
	CvRect rect;

#ifdef DEBUG_IMAGES
	IplImage *temp = cvCreateImage(cvSize(width, height), IPL_DEPTH_8U, 1);
	if (!temp) {
		pr_error("Error with memory allocation for temp image\n");
		ret = -1;
		goto cleanup;
	}
#endif

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

#ifdef DEBUG_SEPRATED_CONTOUR
	cvNamedWindow("SEPRATED_CONTOUR", 1);
	cvMoveWindow("SEPRATED_CONTOUR", 0, 2 * height);
#endif
#ifdef DEBUG_DI
	cvNamedWindow("DI", 1);
	cvMoveWindow("DI", 2 * width, 2 * height);
#endif
#ifdef DEBUG_SKELTON
	cvNamedWindow("SKELETON", 1);
	cvMoveWindow("SKELETON", 4 * width, 2 * height);
#endif
#ifdef DEBUG_SEPRATED_CONTOUR
	cvZero(temp);
	for (i = 0; i < c->total; i++) {
		CvPoint* p = CV_GET_SEQ_ELEM(CvPoint, c,
				i);
		*((uchar*) (temp->imageData +
					p->y * temp->widthStep) + p->x) = 255;
	}
	cvShowImage("SEPRATED_CONTOUR", temp);
#endif
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
			sqrt(pow((p->x - cx), 2) +
					pow((p->y - cy), 2));
	}
	/* Low pass filter it */
	cvSmooth(mat, smat, CV_GAUSSIAN, 39, 1, 0, 0);
#ifdef DEBUG_DI
	/*plot disatance vector */
	cvZero(temp);
	for (i = 0; i < c->total; i++) {
		int x = CV_MAT_ELEM(*smat, float, 0, i);
		*((uchar*) (temp->imageData +
					(height - x) * temp->widthStep) + i) = 255;
	}
	cvShowImage("DI", temp);
#endif
	/* find extream points */

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
	rect=cvBoundingRect (c, 0);
	hy = rect.height;
	wx = rect.width;
	x = rect.x;
	y = rect.y;
	skel_pt_final[0].x = cx;
	skel_pt_final[0].y = cy;
	/* skeleton point nearest to bottom right corner */
	//dmin_prev = sqrt(pow(hy, 2) + pow(wx, 2));
	dmin_prev = pow(hy, 2) + pow(wx, 2);
	for (i = 0; i < j; i++) {
		dmin = pow((x + wx - skel_pt_all[i].x), 2) +
				pow((y + hy - skel_pt_all[i].y), 2);
		if (dmin < dmin_prev) {
			skel_pt_final[1].x =
				skel_pt_all[i].x;
			skel_pt_final[1].y =
				skel_pt_all[i].y;
			dmin_prev = dmin;
		}
	}
	/* skeleton point nearest to bottom left corner */
	//dmin_prev = sqrt(pow(hy, 2) + pow(wx, 2));
	dmin_prev = pow(hy, 2) + pow(wx, 2);
	for (i = 0; i < j; i++) {
		dmin = pow((x - skel_pt_all[i].x), 2) +
				pow((y + hy - skel_pt_all[i].y), 2);
		if (dmin < dmin_prev) {
			skel_pt_final[2].x =
				skel_pt_all[i].x;
			skel_pt_final[2].y =
				skel_pt_all[i].y;
			dmin_prev = dmin;
		}
	}
#ifdef DEBUG_SKELTON
	cvZero(temp);
	cvLine(temp, skel_pt_final[0], skel_pt_final[1],
			cvScalar(255, 0, 0, 0), 1, CV_AA, 0);
	cvLine(temp, skel_pt_final[0], skel_pt_final[2],
			cvScalar(255, 0, 0, 0), 1, CV_AA, 0);
	cvShowImage("SKELETON", temp);
//	cvWaitKey(0);
#endif
	/* fine angle between legs */
	theta = atan2(skel_pt_final[2].y - skel_pt_final[0].y, skel_pt_final[2].x - skel_pt_final[0].x) * 180.0 / CV_PI -
		atan2(skel_pt_final[1].y - skel_pt_final[0].y, skel_pt_final[1].x - skel_pt_final[0].x) * 180.0 / CV_PI;
	i = obj_table->obj_count;
	obj_table->cx[i] = cx;
	obj_table->cy[i] = cy;
	obj_table->theta[i] = theta;

cleanup:
	cvReleaseMat(&mat);
	cvReleaseMat(&smat);
#ifdef DEBUG_IMAGES
	if (temp)
		cvReleaseImage(&temp);
#endif
#ifdef DEBUG_SEPRATED_CONTOUR
	cvDestroyWindow("SEPRATED_CONTOUR");
#endif
#ifdef DEBUG_DI
	cvDestroyWindow("DI");
#endif
#ifdef DEBUG_SKELTON
	cvDestroyWindow("SKELETON");
#endif
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
			//printf("count %d index %d\n", count, index);
			/* classify pixel and update model */
			if (count >= psimin) {
				*(map_data + y * height + x) = bg;
				/* update current pixel model */
				rand = getRandomNumber(rng, phi - 1);
			//	printf("rand %d\n", rand);
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
	struct object_table obj_table;
	int count;
#ifdef DEBUG_IMAGES
	IplImage *temp;
#endif
#ifdef DEBUG_FINAL
	CvFont font;

	cvInitFont(&font, CV_FONT_HERSHEY_SIMPLEX|CV_FONT_ITALIC, 1, 1, 1);
#endif

	LIST_INIT(&tracker_head);
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

#ifdef DEBUG_IMAGES
	temp = cvCreateImage(cvSize(width, height), IPL_DEPTH_8U, 1);
	if (!temp) {
		pr_error("Error with memory allocation for temp image\n");
		ret = -1;
		goto cleanup;
	}
#endif

#ifdef DEBUG_GRAY
	cvNamedWindow("GRAY", 1);
#endif
#ifdef DEBUG_FG
	cvNamedWindow("FG", 1);
	cvMoveWindow("FG", 2*width, 0);
#endif
#ifdef DEBUG_CLEANED
	cvNamedWindow("CLEANED", 1);
	cvMoveWindow("CLEANED", 4 * width, 0);
#endif
#ifdef DEBUG_FINAL
	cvNamedWindow("FINAL", 1);
#endif

#if 0
	/* Get a model data structure */
	/*
	 * Library for background detection from following paper.
	 * O. Barnich and M. Van Droogenbroeck.
	 * ViBe: A universal background subtraction algorithm for video sequences.
	 * In IEEE Transactions on Image Processing, 20(6):1709-1724, June 2011.
	*/
#endif
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
#if 0
	/* Allocates the model and initialize it with the first image */
	libvibeModelAllocInit_8u_C1R(model, (const uint8_t *)gray->imageData,
		width, height, stride);
#endif
	libvibeModelAllocInit_8u_C1R(model, gray);

	/* Processes all the following frames of your stream:
		results are stored in "segmentation_map" */
	while(!acquire_grayscale_image(stream, gray)) {
		Timer.Start();
		frame++;
		if (frame < 0)
			continue;
#ifdef DEBUG_GRAY
		cvShowImage("GRAY", gray);
#endif
#if 0
		/* Get FG image in temp */
		libvibeModelUpdate_8u_C1R(model, (const uint8_t *)gray->imageData,
				(uint8_t *)map->imageData);
#endif
		libvibeModelUpdate_8u_C1R(model, gray, map);
#ifdef DEBUG_FG
		cvShowImage("FG", map);
#endif
		/*
		 * Clean all small unnecessary FG objects. Get cleaned
		 * one in temp2
		 */
		cvErode(map, eroded, NULL, 1);
		/* Dilate it to get in proper shape */
		cvDilate(eroded, dilated, NULL, 1);
#ifdef DEBUG_CLEANED
		cvShowImage("CLEANED", dilated);
#endif
		/*
		 * Find out all moving contours , so basically segment
		 * it out. Create separte image for each moving object.
		 */
		 cvFindContours(dilated, storage, &contours,
				sizeof(CvContour), CV_RETR_EXTERNAL,
				CV_CHAIN_APPROX_NONE, cvPoint(0, 0));
		 memset(&obj_table, 0 , sizeof(obj_table));
		for (c = contours; c != NULL; c = c->h_next) {
			area = cvContourArea(c, CV_WHOLE_SEQ, 0);
			/*
			 * choose only if moving object area is greater
			 * than MIN_AREA square pixel
			 */
			if (area > MIN_AREA) {
				update_object_table(c, width, height,
						&obj_table);
		 		obj_table.obj_count++;
			}
		}
		if(obj_table.obj_count)
			update_tracker(&obj_table, height, width);
		Timer.Stop();
		Timer.PrintElapsedTimeMsg(msg);
		printf("%s\n", msg);
#ifdef DEBUG_IMAGES
		cvZero(temp);
#endif
#ifdef DEBUG_FINAL
		for (count = 0; count < obj_table.obj_count; count++) {
			if (obj_table.type[count] == TYPE_HUMAN) {
				cvPutText(temp, "H", cvPoint(obj_table.cx[count], obj_table.cy[count]),
						&font, cvScalar(255, 255, 0));
			}
		}
		cvShowImage("FINAL", temp);
		cvWaitKey(5);
#endif
	}
cleanup:
#ifdef DEBUG_IMAGES
	if (temp)
		cvReleaseImage(&temp);
#endif
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
#ifdef DEBUG_GRAY
	cvDestroyWindow("GRAY");
#endif
#ifdef DEBUG_FG
	cvDestroyWindow("FG");
#endif
#ifdef DEBUG_CLEANED
	cvDestroyWindow("CLEANED");
#endif
#ifdef DEBUG_CLEANED
	cvDestroyWindow("FINAL");
#endif
	return ret;
}
