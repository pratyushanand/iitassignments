#include <cv.h>
#include <errno.h>
#include <getopt.h>
#include <highgui.h>
#include <sys/queue.h>

#define MAX_OBJECT	50
#define MAX_BOUNDARY_POINT 200
#define MAX_TRACK_HISTORY 100
#define MAX_SAMPLE	20
#define VIDEO_FILE_PATH_LENGTH 100

#define pr_debug(args...)
#define pr_info(args...) fprintf(stdout, ##args)
#define pr_error(args...) fprintf(stdout, ##args)

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

static LIST_HEAD(trackerhead, tracker) tracker_head;
static int min_contour_area = 300;
static bool debug_images = false;
static bool wait_debug = false;

/* Helper function for user */
static void help_main (char *prog_name)
{
	pr_info ("Usage: %s options [ inputfile ... ]\n", prog_name);
	pr_info (" -h	--help		Display this usage information.\n"
			" -i	--input		Input Video file. If this option is not selected then system will try attached camera.\n"
			" -a	--area		Minimum contour area which can be treated as valid contour.\n"
			" -d	--debug		To display debug pipeline images.\n"
			" -w	--wait		To wait for key press after each frame processing.\n");
	return;
}

static int32_t get_image_width(CvCapture *stream)
{
	return cvGetCaptureProperty(stream, CV_CAP_PROP_FRAME_WIDTH);
}

static int32_t get_image_height(CvCapture *stream)
{
	return cvGetCaptureProperty(stream, CV_CAP_PROP_FRAME_HEIGHT);
}

static int32_t acquire_grayscale_image(CvCapture *stream, IplImage *gray)
{
	IplImage *img = cvQueryFrame(stream);

	if (!img)
		return -1;

	cvCvtColor(img, gray, CV_BGR2GRAY);
	return 0;
}

static float compute_mean(int *data, int count)
{
	int i;
	float mean = 0;

	for (i = 0; i < count; i++)
		mean = mean + ((float)data[i] / (float)count);

	return mean;
}

/*
 * This function analyzes leg motion and tells that whether it is a
 * human or not.
 */
static enum obj_type analyze_leg_motion(int *theta)
{
	int i, p[3], j = 0, m1, m2, n;
	int *t1, *t2;
	int numr, dnm1, dnm2;
	float corel;

	/* track value of theta until it goes to ’0’ three times */
	for (i = 0; i < MAX_TRACK_HISTORY; i++) {
		if (!theta[i]) {
			p[j++] = i;
			if (j == 3)
				break;
		}
	}
	if (j < 3)
		return TYPE_UNDEFINED;

	/*
	 * Now consider values of theta between first and second ’0’ as vector t1
	 * and values  between second and third ’0’ as vector t2.
	 */
	n = p[2] - p[1];
	t1 = &theta[p[1]];
	t2 = &theta[p[2]];

	/* Find mean m1 and m2 of vector t1 and t2 respectively */
	m1 = compute_mean(t1, n);
	m2 = compute_mean(t2, n);

	/* calculate correlation value between these two vectors */
	numr = dnm1 = dnm2 =0;
	for (i = 0; i < n; i++)
		numr += (t1[i] - m1) * (t2[i] - m2);
	for (i = 0; i < n; i++)
		dnm1 += (t1[i] - m1) * (t1[i] - m1);
	for (i = 0; i < n; i++)
		dnm2 += (t2[i] - m2) * (t2[i] - m2);
	corel = numr / sqrt (dnm1 * dnm2);

	/*
	 * Correlation value is greater than a threshold value TH1 then
	 * we say that it is a human.
	 */
	if (corel > 0.5)
		return TYPE_HUMAN;
	else
		return TYPE_UNDEFINED;
}

/* update_tracker function detects human and update table with result */
static void update_tracker(struct object_table *obj_table, int height,
		int width)
{
	struct tracker *node;
	int count, n_count, cx, cy, theta, dx, dy, x, y, dmin, n_dmin, dmin_prev, i;

	/*
	 * For all the previously found object (which are already in the
	 * list), update their new position.
	 */
	LIST_FOREACH(node, &tracker_head, list) {
		/*
		 * For each object in the list, check which object is
		 * closest to it's predicted position.
		 */
		dmin_prev = pow(height, 2) + pow(width, 2);
		n_count = -1;
		x = node->px;
		y = node->py;
		for (count = 0; count < obj_table->obj_count; count++) {
			cx = obj_table->cx[count];
			cy = obj_table->cy[count];
			dmin = pow((x - cx), 2) + pow((y -cy), 2);
			if (dmin < dmin_prev) {
				n_count = count;
				n_dmin = dmin;
			}
			dmin_prev = dmin;
		}
		/*
		 * if distance between new position of object and
		 * predicted position of object is less than a threshold
		 * value then take this new value as current position.
		 * else, object is either occluded or moved out of FOV.
		 * Until, we handle occlusion delete node for else case.
		 * Occlusion handling (TBD)
		 */
		/* Allow drift of +-15 pixel in each direction */
		if (n_dmin > 1800) {
			LIST_REMOVE(node, list);
			continue;
		}
		cx = obj_table->cx[n_count];
		cy = obj_table->cy[n_count];
		theta = obj_table->theta[n_count];
		/*
		 * drift in x and y direction to predict next cx and cy.
		 * Currently we are predicting for only next frame,
		 * however a prediction for n number of future frames,
		 * and then it's update from feedback would give us
		 * more closer prediction result.(TBD)
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
		/*
		 * if object type is still not found then try to detect
		 * it on the basis of leg motion.
		 */
		if (node->type == TYPE_UNDEFINED)
			node->type = analyze_leg_motion(node->theta);
		obj_table->type[n_count] = node->type;
		/*
		 * just mark object as older one, ie already found in
		 * list and no need to be added.
		 */
		obj_table->is_old[n_count] = true;
	}

	/*
	 * Check for each object, if it was already in list. If not in
	 * list, then initialize it's structure and add them in list.
	 */
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

/*
 * update_object_table calculates detection feature , which is angle
 * between legs in our case. Object table is updated with calculated
 * feature.
 */
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
	IplImage *temp;

	/* create memory for debug image */
	if (debug_images) {
		temp = cvCreateImage(cvSize(width, height), IPL_DEPTH_8U, 1);
		if (!temp) {
			pr_error("Error with memory allocation for temp image\n");
			ret = -1;
			goto cleanup;
		}
	}

	/*
	 * memory for storing distance of contour boundary points from
	 * it's centroid.
	 */
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

	if (debug_images) {
		cvNamedWindow("SEPRATED_CONTOUR", 1);
		cvMoveWindow("SEPRATED_CONTOUR", 0, 2 * height);
		cvNamedWindow("DI", 1);
		cvMoveWindow("DI", 2 * width, 2 * height);
		cvNamedWindow("SKELETON", 1);
		cvMoveWindow("SKELETON", 4 * width, 2 * height);
		cvZero(temp);
		for (i = 0; i < c->total; i++) {
			CvPoint* p = CV_GET_SEQ_ELEM(CvPoint, c,
					i);
			*((uchar*) (temp->imageData +
						p->y * temp->widthStep) + p->x) = 255;
		}
		cvShowImage("SEPRATED_CONTOUR", temp);
	}
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

	/* calculate distance from centroid to each boundary point*/
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
	if (debug_images) {
		/*plot disatance vector */
		cvZero(temp);
		for (i = 0; i < c->total; i++) {
			int x = CV_MAT_ELEM(*smat, float, 0, i);
			*((uchar*) (temp->imageData +
						(height - x) * temp->widthStep) + i) = 255;
		}
		cvShowImage("DI", temp);
	}
	/* find extream points, 0 crossing of distance points */

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
	/*
	 * To calculate angle between legs we need centroid and a point
	 * on line segment corresponding to each leg. We have already
	 * calculated centroid. We have some skeleton points which are
	 * at local maximum distance from centroid. Endpoints corresponding
	 * to each leg will be those which are neatest to left and right
	 * bottom corner of bounding rectangle respectively.
	 */
	rect=cvBoundingRect (c, 0);
	hy = rect.height;
	wx = rect.width;
	x = rect.x;
	y = rect.y;
	skel_pt_final[0].x = cx;
	skel_pt_final[0].y = cy;
	/* skeleton point nearest to bottom right corner */
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
	if (debug_images) {
		cvZero(temp);
		cvLine(temp, skel_pt_final[0], skel_pt_final[1],
				cvScalar(255, 0, 0, 0), 1, CV_AA, 0);
		cvLine(temp, skel_pt_final[0], skel_pt_final[2],
				cvScalar(255, 0, 0, 0), 1, CV_AA, 0);
		cvShowImage("SKELETON", temp);
		if (wait_debug)
			cvWaitKey(0);
		else
			cvWaitKey(5);
	}
	/* find angle between legs */
	theta = atan2(skel_pt_final[2].y - skel_pt_final[0].y, skel_pt_final[2].x - skel_pt_final[0].x) * 180.0 / CV_PI -
		atan2(skel_pt_final[1].y - skel_pt_final[0].y, skel_pt_final[1].x - skel_pt_final[0].x) * 180.0 / CV_PI;
	/* update object table */
	i = obj_table->obj_count;
	obj_table->cx[i] = cx;
	obj_table->cy[i] = cy;
	obj_table->theta[i] = theta;

cleanup:
	cvReleaseMat(&mat);
	cvReleaseMat(&smat);
	if (debug_images) {
		if (temp)
			cvReleaseImage(&temp);
		cvDestroyWindow("SEPRATED_CONTOUR");
		cvDestroyWindow("DI");
		cvDestroyWindow("SKELETON");
	}
}

static void libvibeModelFree(vibeModel_t **model)
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

static vibeModel_t * libvibeModelNew()
{
	vibeModel_t *model;

	model = (vibeModel_t *)malloc(sizeof(*model));
	if (!model)
		return NULL;
	return model;
}

static int libvibeModelAllocInit_8u_C1R(vibeModel_t *model, IplImage *gray)
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

static int getRandomNumber(CvRNG rng, int seed)
{
	return (cvRandInt(&rng) % seed);
}

static int getRandomNeighbrXCoordinate(CvRNG rng, int x, int width)
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

static int getRandomNeighbrYCoordinate(CvRNG rng, int y, int height)
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

static int libvibeModelUpdate_8u_C1R(vibeModel_t *model, IplImage *gray,
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
	CvCapture* stream;
	CvMemStorage* storage;
	int32_t width, height;
	CvSeq* contours = NULL;
	CvSeq* c = NULL;
	IplImage *gray, *map, *eroded, *dilated, *out_img;
	int ret, count;
	float area;
	vibeModel_t *model;
	struct object_table obj_table;
	char video_path[VIDEO_FILE_PATH_LENGTH];
	CvFont font;
	int next_option;
	const char* const short_options = "hi:a:dw";
	const struct option long_options[] = {
		{ "help",     0, NULL, 'h' },
		{ "input",   1, NULL, 'i' },
		{ "area",   1, NULL, 'a' },
		{ "debug",   1, NULL, 'd' },
		{ "wait",   1, NULL, 'w' },
	};

	video_path[0] = '\0';
	/* get all user input in variables */
	do {
		next_option = getopt_long (argc, argv, short_options,
				long_options, NULL);
		switch (next_option) {
		case 'h':
			help_main(argv[0]);
			return 0;
		case 'i':
			if (strlen(optarg) > VIDEO_FILE_PATH_LENGTH) {
				pr_error("Too long input file path name\n");
				return -EINVAL;
			}
			strcpy(video_path, optarg);
			break;
		case 'a':
			min_contour_area = atoi(optarg);
			if (!min_contour_area) {
				pr_error("Incorrect min contour area\n");
				return -EINVAL;
			}
			break;
		case 'd':
			debug_images = true;
			break;
		case 'w':
			wait_debug = true;
			break;
		case -1:
			break;
		default:
			abort();
		}
	} while (next_option != -1);

	/* Initialize font to be used to write text in output image window */
	cvInitFont(&font, CV_FONT_HERSHEY_SIMPLEX|CV_FONT_ITALIC, 1, 1, 1);

	/* Initialize tracker list */
	LIST_INIT(&tracker_head);

	/*
	 * Check if user has provided a test video input file. If test
	 * input file has been provided then process frames from there,
	 * else process frames from live camera
	 */
	if (video_path[0] != '\0')
		stream = cvCaptureFromFile(video_path);
	else
		stream = cvCaptureFromCAM(0);
	if (!stream) {
		pr_error("Could not create capture stream\n");
		ret = -EINVAL;
		goto cleanup;
	}

	/* Memory storage for internal uses by few CV functions */
	storage = cvCreateMemStorage(0);
	if (!storage) {
		pr_error("Could not create storage space\n");
		ret = -ENOMEM;
		goto cleanup;
	}

	width = get_image_width(stream);
	height = get_image_height(stream);

	out_img = cvCreateImage(cvSize(width, height), IPL_DEPTH_8U, 1);
	if (!out_img) {
		pr_error("Error with memory allocation for out_img image\n");
		ret = -ENOMEM;
		goto cleanup;
	}

	cvNamedWindow("GRAY", 1);
	if (debug_images) {
		cvNamedWindow("FG", 1);
		cvMoveWindow("FG", 2*width, 0);
		cvNamedWindow("CLEANED", 1);
		cvMoveWindow("CLEANED", 4 * width, 0);
	}
	cvNamedWindow("FINAL", 1);
	cvMoveWindow("FINAL", 2*width, 0);

	model = libvibeModelNew();
	if (!model) {
		pr_error("Vibe Model could not be created\n");
		ret = -ENOMEM;
		goto cleanup;
	}

	/* Allocates memory to store the intermediate images */
	gray = cvCreateImage(cvSize(width, height), IPL_DEPTH_8U, 1);
	if (!gray) {
		pr_error("Error with memory allocation for gray image\n");
		ret = -ENOMEM;
		goto cleanup;
	}
	map = cvCreateImage(cvSize(width, height), IPL_DEPTH_8U, 1);
	if (!map) {
		pr_error("Error with memory allocation for map image\n");
		ret = -ENOMEM;
		goto cleanup;
	}
	eroded = cvCreateImage(cvSize(width, height), IPL_DEPTH_8U, 1);
	if (!eroded) {
		pr_error("Error with memory allocation for eroded image\n");
		ret = -ENOMEM;
		goto cleanup;
	}
	dilated = cvCreateImage(cvSize(width, height), IPL_DEPTH_8U, 1);
	if (!dilated) {
		pr_error("Error with memory allocation for dilated image\n");
		ret = -ENOMEM;
		goto cleanup;
	}

	/* Acquires first image */
	acquire_grayscale_image(stream, gray);
	/* Initialize Vibe model with first image parameters */
	libvibeModelAllocInit_8u_C1R(model, gray);

	/* Processes all the following frames of stream results are stored in map */
	while(!acquire_grayscale_image(stream, gray)) {
		cvShowImage("GRAY", gray);

		/* get foreground image in map */
		libvibeModelUpdate_8u_C1R(model, gray, map);

		if (debug_images)
			cvShowImage("FG", map);
		/* Clean all small unnecessary FG objects. */
		cvErode(map, eroded, NULL, 1);
		/* Dilate it to get in proper shape */
		cvDilate(eroded, dilated, NULL, 1);
		if (debug_images)
			cvShowImage("CLEANED", dilated);
		/*
		 * Find out all moving contours , so basically segment
		 * out all moving objects, so that they are treated
		 * separately.
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
			if (area > min_contour_area) {
				update_object_table(c, width, height,
						&obj_table);
				obj_table.obj_count++;
			}
		}
		if(obj_table.obj_count)
			update_tracker(&obj_table, height, width);

		/* Display detected object */
		cvZero(out_img);

		for (count = 0; count < obj_table.obj_count; count++) {
			if (obj_table.type[count] == TYPE_HUMAN) {
				cvPutText(out_img, "H", cvPoint(obj_table.cx[count], obj_table.cy[count]),
						&font, cvScalar(255, 255, 0));
			}
		}
		cvShowImage("FINAL", out_img);
		cvWaitKey(0);
	}
cleanup:
	/* Cleanup allocated memory */
	if (out_img)
		cvReleaseImage(&out_img);
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
	cvDestroyWindow("GRAY");
	if (debug_images) {
		cvDestroyWindow("FG");
		cvDestroyWindow("CLEANED");
	}
	cvDestroyWindow("FINAL");

	return ret;
}
