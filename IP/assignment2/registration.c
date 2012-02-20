/*
 * this program does registrationing of one images into another using
 * affine and projective transformation.
 */

/*-----------All includes of file-----------------*/
#include <cv.h>
#include <highgui.h>
#include "stdio.h"
#include "string.h"
#include "getopt.h"
#include "fcntl.h"
#include "unistd.h"

/* all defines for this program */
#define MAX_NAME_LEN	50
#define MAX_CTRL_POINT	20
#define pr_info(args...) fprintf(stdout, ##args)
#define pr_dbg(args...)
#define pr_err(args...) fprintf(stdout, ##args)

struct ctrl_pt {
	int select_end;
	int ctrl_point_num;
	int ctrl_1count;
	int ctrl_2count;
	CvPoint2D32f ctrl_1[MAX_CTRL_POINT];
	CvPoint2D32f ctrl_2[MAX_CTRL_POINT];
};

/* Prints help for registration program */
static void help_registration (char *prog_name)
{
	FILE *stream = stdout;

	fprintf (stream, "Usage: %s options [ select... ]\n", prog_name);
	fprintf (stream,
		" -i	--initial	Initial Image Name.\n"
		" -f	--final		Final Image Name.\n"
		" -o	--output	Output Image Name.\n"
		" -m	--method	Method can be affine or projective.\n"
		" -e	--error		Calculate error between two images initial and final.\n");
	exit (0);
}

/*mouse callback function for selecting points on initial image*/

static void select_ctrl_point1(int event, int x, int y, int flags, void* param)
{
	struct ctrl_pt *pt = (struct ctrl_pt *)param;

	if(event == CV_EVENT_LBUTTONDOWN){

		if (pt->ctrl_1count < pt->ctrl_point_num) {
			pt->ctrl_1[pt->ctrl_1count].x = x;
			pt->ctrl_1[pt->ctrl_1count++].y = y;
			pr_dbg("x = %d, y = %d\n", x, y);
		}
	} else if(event == CV_EVENT_RBUTTONDOWN && pt->ctrl_1count == pt->ctrl_2count)
		pt->select_end = 1;
}

/*mouse callback function for selecting points on final image*/

static void select_ctrl_point2(int event, int x, int y, int flags, void* param)
{
	struct ctrl_pt *pt = (struct ctrl_pt *)param;

	if (event == CV_EVENT_LBUTTONDOWN){

		if (pt->ctrl_2count < pt->ctrl_point_num){
			pt->ctrl_2[pt->ctrl_2count].x = x;
			pt->ctrl_2[pt->ctrl_2count++].y = y;
			pr_dbg("x = %d, y = %d\n", x, y);
		}
	} else if(event == CV_EVENT_RBUTTONDOWN && pt->ctrl_1count == pt->ctrl_2count)
		pt->select_end = 1;
}

static void get_points(char *initial, char *final, struct ctrl_pt *pt)
{
	IplImage *img1, *img2;
	int i, j;

	/*create window for image display*/
	cvNamedWindow("IMAGE1", 1);
	cvNamedWindow("IMAGE2", 1);

	img1 = cvLoadImage(initial, 1);
	img2 = cvLoadImage(final, 1);

	cvShowImage("IMAGE1", img1);
	cvShowImage("IMAGE2", img2);

	/*slect control points from mouse*/
	cvSetMouseCallback("IMAGE1", select_ctrl_point1, pt);
	cvSetMouseCallback("IMAGE2", select_ctrl_point2, pt);
	pr_info("select %d control point(1-2-1-2...) with mouse left click\n", pt->ctrl_point_num);

	/*select control points*/
	for (i = 0;i < pt->ctrl_point_num;i++) {
		/* wait till user selects point*/
		while (i >= pt->ctrl_1count)
			cvWaitKey(300);
		for (j = -1;j < i;j++) {
			CvPoint	tmp;
			CvScalar s = cvScalar(0+rand()%255, 0+rand()%255,
					0+rand()%255, 0);
			tmp.x = pt->ctrl_1[j+1].x;
			tmp.y = pt->ctrl_1[j+1].y;
			cvCircle(img1, tmp, 2, s, 2, 8, 0);
		}
		cvShowImage("IMAGE1", img1);
		/* wait till user selects point*/
		while (i >= pt->ctrl_2count)
			cvWaitKey(300);
		for (j = -1;j < i;j++) {
			CvPoint	tmp;
			CvScalar s = cvScalar(0+rand()%255, 0+rand()%255,
					0+rand()%255, 0);
			tmp.x = pt->ctrl_2[j+1].x;
			tmp.y = pt->ctrl_2[j+1].y;
			cvCircle(img2, tmp, 2, s, 2, 8, 0);
		}
		cvShowImage("IMAGE2", img2);
	}

	cvDestroyWindow("IMAGE1");
	cvDestroyWindow("IMAGE2");
	cvReleaseImage(&img1);
	cvReleaseImage(&img2);
}

void calculate_error(char *initial, char *output, struct ctrl_pt *pt)
{
	IplImage *img1, *img2;
	int i, j;

	/*create window for image display*/
	cvNamedWindow("IMAGE1", 1);
	cvNamedWindow("IMAGE2", 1);

	img1 = cvLoadImage(initial, 1);
	img2 = cvLoadImage(output, 1);

	cvShowImage("IMAGE1", img1);
	cvShowImage("IMAGE2", img2);

	/*slect control points from mouse*/
	cvSetMouseCallback("IMAGE1", select_ctrl_point1, pt);
	cvSetMouseCallback("IMAGE2", select_ctrl_point2, pt);
	pr_info("select point(1-2-1-2...) with mouse left click\n");
	pr_info("right click to end\n");

	while(1) {
		/* wait till user selects point*/
		i = pt->ctrl_1count;
		while (i == pt->ctrl_1count && !pt->select_end)
			cvWaitKey(300);
		if (pt->select_end)
			break;
		for (j = -1;j < i;j++) {
			CvPoint	tmp;
			CvScalar s = cvScalar(0+rand()%255, 0+rand()%255,
					0+rand()%255, 0);
			tmp.x = pt->ctrl_1[j+1].x;
			tmp.y = pt->ctrl_1[j+1].y;
			cvCircle(img1, tmp, 2, s, 2, 8, 0);
		}
		cvShowImage("IMAGE1", img1);
		/* wait till user selects point*/
		i = pt->ctrl_2count;
		while (i == pt->ctrl_2count && !pt->select_end)
			cvWaitKey(300);
		if (pt->select_end)
			break;
		for (j = -1;j < i;j++) {
			CvPoint	tmp;
			CvScalar s = cvScalar(0+rand()%255, 0+rand()%255,
					0+rand()%255, 0);
			tmp.x = pt->ctrl_2[j+1].x;
			tmp.y = pt->ctrl_2[j+1].y;
			cvCircle(img2, tmp, 2, s, 2, 8, 0);
		}
		cvShowImage("IMAGE2", img2);
		pr_info("Delta_X = %d, Delta_Y = %d\n", (int)(pt->ctrl_2[i].x - pt->ctrl_1[i].x),
				(int)(pt->ctrl_2[i].y - pt->ctrl_1[i].y));
	}
	cvDestroyWindow("IMAGE1");
	cvDestroyWindow("IMAGE2");
	cvReleaseImage(&img1);
	cvReleaseImage(&img2);

}
/*
 * Ref for Eqn: http://alumni.media.mit.edu/~cwren/interpolator/
 *
 * src : src control point
 * dst : dst control point
 * transform is 3 x 3 affine matrix
 * |a b c|
 * |d e f|
 * |g h 1|
 *
 *      a*x + b*y + c
 * u = ---------------
 *      g*x + h*y + 1
 *
 *      d*x + e*y + f
 * v = ---------------
 *      g*x + h*y + 1
 * | x0 y0  1  0  0  0 -x0*u0 -y0*u0 | |a| |u0|
 * | x1 y1  1  0  0  0 -x1*u1 -y1*u1 | |b| |u1|
 * | x2 y2  1  0  0  0 -x2*u2 -y2*u2 | |c| |u2|
 * | x3 y3  1  0  0  0 -x3*u3 -y3*u3 |.|d|=|u3|
 * |  0  0  0 x0 y0  1 -x0*v0 -y0*v0 | |e| |v0|
 * |  0  0  0 x1 y1  1 -x1*v1 -y1*v1 | |f| |v1|
 * |  0  0  0 x2 y2  1 -x2*v2 -y2*v2 | |g| |v2|
 * |  0  0  0 x3 y3  1 -x3*v3 -y3*v3 | |h| |v3|
 * p * v = q : solve for v
 */
static void calculate_projective_transform(CvPoint2D32f *src,
		CvPoint2D32f *dst, CvMat *transform)
{
	int i;
	CvMat* p = cvCreateMat(8, 8, CV_32FC1);
	CvMat* q = cvCreateMat(8, 1, CV_32FC1);

	for (i = 0; i < 4; i++) {
		*((float *)CV_MAT_ELEM_PTR(*p, i, 0)) = (src + i)->x;
		*((float *)CV_MAT_ELEM_PTR(*p, i + 4, 3)) = (src + i)->x;
		*((float *)CV_MAT_ELEM_PTR(*p, i, 1)) = (src + i)->y;
		*((float *)CV_MAT_ELEM_PTR(*p, i + 4, 4)) = (src + i)->y;
		*((float *)CV_MAT_ELEM_PTR(*p, i, 2)) = 1;
		*((float *)CV_MAT_ELEM_PTR(*p, i + 4, 5)) = 1;
		*((float *)CV_MAT_ELEM_PTR(*p, i, 3)) = 0;
		*((float *)CV_MAT_ELEM_PTR(*p, i, 4)) = 0;
		*((float *)CV_MAT_ELEM_PTR(*p, i, 5)) = 0;
		*((float *)CV_MAT_ELEM_PTR(*p, i + 4, 0)) = 0;
		*((float *)CV_MAT_ELEM_PTR(*p, i + 4, 1)) = 0;
		*((float *)CV_MAT_ELEM_PTR(*p, i + 4, 2)) = 0;

		*((float *)CV_MAT_ELEM_PTR(*p, i, 6)) = -((src + i)->x
				* (dst + i)->x);
		*((float *)CV_MAT_ELEM_PTR(*p, i, 7)) = -((src + i)->y
				* (dst + i)->x);
		*((float *)CV_MAT_ELEM_PTR(*p, i + 4, 6)) = -((src + i)->x
				* (dst + i)->y);
		*((float *)CV_MAT_ELEM_PTR(*p, i + 4, 7)) = -((src + i)->y
				* (dst + i)->y);
		*((float *)CV_MAT_ELEM_PTR(*q, i, 0)) = (dst + i)->x;
		*((float *)CV_MAT_ELEM_PTR(*q, i + 4, 0)) = (dst + i)->y;
	}
#ifdef DEBUG
	for(int row=0; row<p->rows; row++) {
		const float* ptr = (const float*)(p->data.ptr + row * p->step);
		for(int col=0; col<p->cols; col++) {
			pr_dbg("%f\t", *ptr++);
		}
		pr_dbg("\n");
	}
#endif
	cvSolve(p, q, transform);
}
/*
 *      t0*x + t1*y + t2
 * u = -----------------
 *      t6*x + t7*y + 1
 *
 *      t3*x + t4*y + t5
 * v = -----------------
 *      t6*x + t7*y + 1
 */
static void warp_projective(IplImage *img1, IplImage *img, CvMat *transform)
{
	int i, x, y, u, v, ch = img1->nChannels;
	uchar *ptr1, *ptr;
	float t[6], fx, fy;

	for (i = 0; i < 8; i++)
		t[i] = CV_MAT_ELEM(*transform, float, i, 0);

	for (y = 0; y < img->height; y++) {
		ptr = (uchar*) (img->imageData + y * img->widthStep);
		for (x = 0; x < img->width; x++) {
			fx = x; fy = y;
			u = round((t[0] * fx + t[1] * fy + t[2])
					/ (t[6] * fx + t[7] * fy + 1));
			v = round((t[3] * fx + t[4] * fy + t[5])
					/ (t[6] * fx + t[7] * fy + 1));
			pr_dbg("%d\t%d\t%d\t%d\n", x, y, u, v);
			if (u >= 0 && u < img1-> width
					&& v >= 0 && v < img1->height) {
				ptr1 = (uchar*) (img1->imageData +
						v * img1->widthStep);
				*(ptr + ch * x) = *(ptr1 + ch * u);
				if (ch == 3) {
					*(ptr + ch * x + 1) =
						*(ptr1 + ch * u + 1);
					*(ptr + ch * x + 2) =
						*(ptr1 + ch * u + 2);
				}
			}
		}
	}
}

static void register_projective(char *initial, char *output,
		struct ctrl_pt *pt)
{
	IplImage	*img, *img1;
	CvMat* transform;

	img1 = cvLoadImage(initial, 1);
	img = cvCreateImage(cvSize(img1->width, img1->height), 8, 3);
	cvZero(img);

	/* space for transformation matrix */
	transform = cvCreateMat(8, 1, CV_32FC1);
	/* get transformation matrix between two images */
	calculate_projective_transform(pt->ctrl_2, pt->ctrl_1, transform);
	/* do projective transformation from image1 to image 2 */
	warp_projective(img1, img, transform);
	cvSaveImage(output, img);

	/* free all memories */
	cvReleaseMat(&transform);
	cvReleaseImage(&img1);
	cvReleaseImage(&img);

}

/*
 * src : src control point
 * dst : dst control point
 * transform is 2 x 3 affine matrix
 * |a1 a2 b1|
 * |a3 a4 b2|
 * |u1| = |a1 a2| |x1| + |b1|
 * |v1|   |a3 a4| |y1| + |b2|
 * a1*x1 + a2*y1 + b1 = u1
 * a3*x1 + a4*y1 + b2 = v1
 * |x1 y1 1  0  0  0| |a1| = |u1|
 * |x2 y2 1  0  0  0| |a2| = |u2|
 * |x3 y3 1  0  0  0| |b1| = |u3|
 * |0  0  0 x1 y1 1 | |a3| = |v1|
 * |0  0  0 x2 y2 1 | |a4| = |v2|
 * |0  0  0 x3 y3 1 | |b2| = |v3|
 * p * v = q : solve for v
 */
static void calculate_affine_transform(CvPoint2D32f *src,
		CvPoint2D32f *dst, CvMat *transform)
{
	int i;
	CvMat* p = cvCreateMat(6, 6, CV_32FC1);
	CvMat* q = cvCreateMat(6, 1, CV_32FC1);

	for (i = 0; i < 3; i++) {
		*((float *)CV_MAT_ELEM_PTR(*p, i, 0)) = (src + i)->x;
		*((float *)CV_MAT_ELEM_PTR(*p, i + 3, 3)) = (src + i)->x;
		*((float *)CV_MAT_ELEM_PTR(*p, i, 1)) = (src + i)->y;
		*((float *)CV_MAT_ELEM_PTR(*p, i + 3, 4)) = (src + i)->y;
		*((float *)CV_MAT_ELEM_PTR(*p, i, 2)) = 1;
		*((float *)CV_MAT_ELEM_PTR(*p, i + 3, 5)) = 1;
		*((float *)CV_MAT_ELEM_PTR(*p, i, 3)) = 0;
		*((float *)CV_MAT_ELEM_PTR(*p, i, 4)) = 0;
		*((float *)CV_MAT_ELEM_PTR(*p, i, 5)) = 0;
		*((float *)CV_MAT_ELEM_PTR(*p, i + 3, 0)) = 0;
		*((float *)CV_MAT_ELEM_PTR(*p, i + 3, 1)) = 0;
		*((float *)CV_MAT_ELEM_PTR(*p, i + 3, 2)) = 0;
		*((float *)CV_MAT_ELEM_PTR(*q, i, 0)) = (dst + i)->x;
		*((float *)CV_MAT_ELEM_PTR(*q, i + 3, 0)) = (dst + i)->y;
	}
#ifdef DEBUG
	for(int row=0; row<q->rows; row++ ) {
		const float* ptr = (const float*)(q->data.ptr + row * q->step);
		for(int col=0; col<q->cols; col++ ) {
			pr_dbg("%f\t", *ptr++);
		}
		pr_dbg("\n");
	}
#endif
	cvSolve(p, q, transform);
}

/*
 * u = t0*x + t1*y + t2
 * v = t3*x + t4*y + t5
 */
static void warp_affine(IplImage *img1, IplImage *img, CvMat *transform)
{
	int i, x, y, u, v, ch = img1->nChannels;
	uchar *ptr1, *ptr;
	float t[6], fx, fy;

	for (i = 0; i < 6; i++)
		t[i] = CV_MAT_ELEM(*transform, float, i, 0);

	for (y = 0; y < img->height; y++) {
		ptr = (uchar*) (img->imageData + y * img->widthStep);
		for (x = 0; x < img->width; x++) {
			fx = x; fy = y;
			u = round(t[0] * fx + t[1] * fy + t[2]);
			v = round(t[3] * fx + t[4] * fy + t[5]);
			if (u >= 0 && u < img1-> width
					&& v >= 0 && v < img1->height) {
				ptr1 = (uchar*) (img1->imageData +
						v * img1->widthStep);
				*(ptr + ch * x) = *(ptr1 + ch * u);
				if (ch == 3) {
					*(ptr + ch * x + 1) =
						*(ptr1 + ch * u + 1);
					*(ptr + ch * x + 2) =
						*(ptr1 + ch * u + 2);
				}
			}
		}
	}
}

/*
 * initial : name of first image to be registered
 * output : name of registered image
 * ctrl_pt : control points for inital and final image
 * will create an image with name output
 */
static void register_affine(char *initial, char *output, struct ctrl_pt *pt)
{
	IplImage	*img, *img1;
	CvMat* transform;

	img1 = cvLoadImage(initial, 1);
	img = cvCreateImage(cvSize(img1->width, img1->height), img1->depth,
			img1->nChannels);
	cvZero(img);

	/*space for affine transformation matrix*/
	transform = cvCreateMat(6, 1, CV_32FC1);
	/*get affine matrix between two images*/
	calculate_affine_transform(pt->ctrl_2, pt->ctrl_1, transform);
	/*do Affine transformation from image1 to image 2*/
	warp_affine(img1, img, transform);
	cvSaveImage(output, img);

	cvReleaseMat(&transform);
	cvReleaseImage(&img1);
	cvReleaseImage(&img);
}

/*
 * main routine of this program. takes varibale arg.
 * details can be seen in help
 */
int main(int argc, char **argv)
{
	struct ctrl_pt pt;
	int next_option;
	const char* const short_options = "hi:f:o:m:e";
	char initial[MAX_NAME_LEN];
	char final[MAX_NAME_LEN];
	char output[MAX_NAME_LEN];
	char method[MAX_NAME_LEN];
	unsigned char error_calc = 0;
	const struct option long_options[] = {
		{ "help",     0, NULL, 'h' },
		{ "initial",   1, NULL, 'i' },
		{ "final",   1, NULL, 'f' },
		{ "output",   1, NULL, 'o' },
		{ "error",   0, NULL, 'e' },
		{ NULL,       0, NULL, 0}
	};

	/*save input arguments*/

	do {
		next_option = getopt_long (argc, argv, short_options,
				long_options, NULL);
		switch (next_option)
		{
			case 'h':
				help_registration(argv[0]);
				return 0;
			case 'i':
				if (strlen(optarg) > MAX_NAME_LEN) {
					pr_err("Initial Name is too large\n");
					abort();
				}
				strcpy(initial, optarg);
				break;
			case 'f':
				if (strlen(optarg) > MAX_NAME_LEN) {
					pr_err("final Name is too large\n");
					abort();
				}
				strcpy(final, optarg);
				break;
			case 'o':
				if (strlen(optarg) > MAX_NAME_LEN) {
					pr_err("Output Name is too large\n");
					abort();
				}
				strcpy(output, optarg);
				break;
			case 'm':
				if (strlen(optarg) > MAX_NAME_LEN) {
					pr_err("Method Name is too large\n");
					abort();
				}
				strcpy(method, optarg);
				break;
			case 'e':
				error_calc = 1;
			case -1:
				break;
			default:
				abort ();
		}
	} while (next_option != -1);

	if (error_calc) {
		pt.select_end = 0;
		pt.ctrl_1count = 0;
		pt.ctrl_2count = 0;
		pt.ctrl_point_num = MAX_CTRL_POINT;
		calculate_error(initial, output, &pt);
	} else {
		if (!strcmp(method, "projective"))
			pt.ctrl_point_num = 4;
		else if (!strcmp(method, "affine"))
			pt.ctrl_point_num = 3;
		pt.ctrl_1count = 0;
		pt.ctrl_2count = 0;

		get_points(initial, final, &pt);

		if (!strcmp(method, "projective"))
			register_projective(initial, output, &pt);
		else if (!strcmp(method, "affine"))
			register_affine(initial, output, &pt);
	}
}
