/*
 * this program does deformationing of image using moving least square.
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

/* all static/global variables for this program */
struct ctrl_pt {
	int end_selection;
	int ctrl_1count;
	int ctrl_2count;
	CvPoint2D32f ctrl_1[MAX_CTRL_POINT];
	CvPoint2D32f ctrl_2[MAX_CTRL_POINT];
};

/* Prints help for deformation program */
static void help_deformation (char *prog_name)
{
	FILE *stream = stdout;

	fprintf (stream, "Usage: %s options [ select... ]\n", prog_name);
	fprintf (stream,
		" -i	--initial	Initial Image Name.\n"
		" -o	--output	Output Image Name.\n"
		" -a	--alpha		Alpha value for weight calculation.\n");
	exit (0);
}

/*mouse callback function for selecting points on initial image*/

static void select_ctrl_point1(int event, int x, int y, int flags, void* param)
{
	struct ctrl_pt *pt = (struct ctrl_pt *)param;

	if(event == CV_EVENT_LBUTTONDOWN) {
		if (pt->ctrl_1count == pt->ctrl_2count) {
			pt->ctrl_2[pt->ctrl_2count].x = x;
			pt->ctrl_2[pt->ctrl_2count++].y = y;
			pr_dbg("cr2 %d x = %d, y = %d\n", pt->ctrl_1count, x, y);
		} else {
			pt->ctrl_1[pt->ctrl_1count].x = x;
			pt->ctrl_1[pt->ctrl_1count++].y = y;
			pr_dbg("cr2 %d x = %d, y = %d\n", pt->ctrl_2count, x, y);
		}
	} else if ((event == CV_EVENT_RBUTTONDOWN) &&
			(pt->ctrl_1count == pt->ctrl_2count)) {
		pr_dbg("Point selection end\n");
		pt->end_selection = 1;
	}

}

static void get_points(char *initial, struct ctrl_pt *pt)
{
	IplImage *img1;
	int i, j;

	/*create window for image display*/
	cvNamedWindow("IMAGE1", 1);

	img1 = cvLoadImage(initial, 1);

	cvShowImage("IMAGE1", img1);

	/*slect control points from mouse*/
	cvSetMouseCallback("IMAGE1", select_ctrl_point1, pt);
	pr_info("select control point(p1-q1-p2-q2...) with mouse left click\n");
	pr_info("Right click to end selection\n");

	/*select control points*/
	while (1) {
		/* wait till user selects point*/
		i = pt->ctrl_2count;
		while (i == pt->ctrl_2count && !pt->end_selection)
			cvWaitKey(300);
		if (pt->end_selection)
			break;
		for (j = -1;j < i;j++) {
			CvPoint	tmp;
			CvScalar s = cvScalar(0+rand()%255, 0+rand()%255,
					0+rand()%255, 0);
			tmp.x = pt->ctrl_2[j+1].x;
			tmp.y = pt->ctrl_2[j+1].y;
			cvCircle(img1, tmp, 2, s, 2, 8, 0);
		}
		cvShowImage("IMAGE1", img1);
		/* wait till user selects point*/
		i = pt->ctrl_1count;
		while (i == pt->ctrl_1count && !pt->end_selection)
			cvWaitKey(300);
		if (pt->end_selection)
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
	}

	cvDestroyWindow("IMAGE1");
	cvReleaseImage(&img1);
}

static void apply_deformation(char *initial, char *output, struct ctrl_pt *pt,
		int alpha)
{
	IplImage	*img, *img1;
	CvMat* transform;
	int w, h, i, j, k, count = pt->ctrl_1count, x, y;
	float *wi, *tw, sumwi, p00, p01, p10, p11, pi00, pi01, pi10, pi11, det, A;
	uchar *ptr1, *ptr;
	CvPoint2D32f sumwipi, sumwiqi, *pstar, *qstar, *tpstar, *tqstar, *pcap, *qcap, *tpcap, *tqcap, vpstar, vpstarmi, fav;

	img1 = cvLoadImage(initial, 1);
	w = img1->width;
	h = img1->height;
	img = cvCreateImage(cvSize(w, h), img1->depth,
			img1->nChannels);
	cvZero(img);

	/* count number of weight for each point in image */
	wi = (float *)malloc(count * w * h * sizeof(float));
	pstar = (CvPoint2D32f *)malloc(w * h * sizeof(CvPoint2D32f));
	qstar = (CvPoint2D32f *)malloc(w * h * sizeof(CvPoint2D32f));
	pcap = (CvPoint2D32f *)malloc(count * w * h * sizeof(CvPoint2D32f));
	qcap = (CvPoint2D32f *)malloc(count * w * h * sizeof(CvPoint2D32f));

	for (j = 0; j < h; j++) {
		ptr = (uchar*) (img->imageData + j * img->widthStep);
		tw = wi + j * count * w;
		tpstar = pstar + j * w;
		tqstar = qstar + j * w;
		tpcap = pcap + j * count * w;
		tqcap = qcap + j * count * w;
		for (i = 0; i < w; i++) {
			sumwipi.x = sumwipi.y = 0;
			sumwiqi.x = sumwiqi.y = 0;
			sumwi = 0.000000000001;
			/* calculate wi, pstar, qstar, pcap and qcap */
			for (k = 0; k < count; k++) {
				*(tw + k) = pow((pow((i - pt->ctrl_1[k].x), 2)
					+ pow((j - pt->ctrl_1[k].y), 2)), -alpha);
				sumwi += *(tw + k);
				sumwipi.x += (*(tw + k)) * pt->ctrl_1[k].x;
				sumwipi.y += (*(tw + k)) * pt->ctrl_1[k].y;
				sumwiqi.x += (*(tw + k)) * pt->ctrl_2[k].x;
				sumwiqi.y += (*(tw + k)) * pt->ctrl_2[k].y;
				pr_dbg("%f\t", pt->ctrl_1[k].x);
				pr_dbg("%f\t", pt->ctrl_1[k].y);
				pr_dbg("%f\t", pt->ctrl_2[k].x);
				pr_dbg("%f\n", pt->ctrl_2[k].y);
			}
			(*(tpstar + i)).x = sumwipi.x / sumwi;
			(*(tpstar + i)).y = sumwipi.y / sumwi;
			pr_dbg("tpstar %f\t", (*(tpstar + i)).x);
			pr_dbg("%f\n", (*(tpstar + i)).y);
			(*(tqstar + i)).x = sumwiqi.x / sumwi;
			(*(tqstar + i)).y = sumwiqi.y / sumwi;
			pr_dbg("tqstar %f\t", (*(tqstar + i)).x);
			pr_dbg("%f\n", (*(tqstar + i)).y);
			for (k = 0; k < count; k++) {
				(*(tpcap + k)).x = pt->ctrl_1[k].x - (*(tpstar + i)).x;
				(*(tpcap + k)).y = pt->ctrl_1[k].y - (*(tpstar + i)).y;
				pr_dbg("tpcap %f\t", (*(tpcap + k)).x);
				pr_dbg("%f\n", (*(tpcap + k)).y);
				(*(tqcap + k)).x = pt->ctrl_2[k].x - (*(tqstar + i)).x;
				(*(tqcap + k)).y = pt->ctrl_2[k].y - (*(tqstar + i)).y;
				pr_dbg("tqcap %f\t", (*(tqcap + k)).x);
				pr_dbg("%f\n", (*(tqcap + k)).y);
			}
			/* calculate affine matrix */
			p00 = p01 = p10 = p11 = 0;
			for (k = 0; k < count; k++) {
				p00 += (*(tw + k)) * (*(tpcap + k)).x * (*(tpcap + k)).x;
				p01 += (*(tw + k)) * (*(tpcap + k)).x * (*(tpcap + k)).y;
				p11 += (*(tw + k)) * (*(tpcap + k)).y * (*(tpcap + k)).y;
			}
			p10 = p01;
			det = p00 * p11 - p01 * p10;
			pi00 = p11 / det;
			pi01 = -p01 / det;
			pi10 = -p10 / det;
			pi11 = p00 / det;
			pr_dbg("p00 %f\t%f\t%f\t%f\n", p00, p01, p10, p11);
			pr_dbg("pi00 %f\t%f\t%f\t%f\n", pi00, pi01, pi10, pi11);
			vpstar.x = i - (*(tpstar + i)).x;
			vpstar.y = j - (*(tpstar + i)).y;
			pr_dbg("vpstar %f\t", vpstar.x);
			pr_dbg("%f\n", vpstar.y);
			vpstarmi.x = pi00 * vpstar.x + pi10 * vpstar.y;
			vpstarmi.y = pi01 * vpstar.x + pi11 * vpstar.y;
			pr_dbg("vpstarmi %f\t", vpstarmi.x);
			pr_dbg("%f\n", vpstarmi.y);
			fav.x = fav.y = 0;
			for (k = 0; k < count; k++) {
				A = (*(tw + k)) * (vpstarmi.x * (*(tpcap + k)).x + vpstarmi.y * (*(tpcap + k)).y);
				pr_dbg("%f\t", *(tw + k));
				pr_dbg("%f\t", vpstarmi.x);
				pr_dbg("%f\t", vpstarmi.y);
				pr_dbg("%f\t", (*(tpcap + k)).x);
				pr_dbg("%f\t", (*(tpcap + k)).y);
				pr_dbg("%f\n", A);
				fav.x += A * (*(tqcap + k)).x;
				fav.y += A * (*(tqcap + k)).y;
			}

			fav.x += (*(tqstar + i)).x;
			fav.y += (*(tqstar + i)).y;

			x = round(fav.x);
			y = round(fav.y);
			pr_dbg("%d\t%d\t%d\t%d\n\n", i, x, j, y);
			if (x >= 0 && x < img-> width
					&& y >= 0 && y < img->height) {
				ptr1 = (uchar*) (img1->imageData + y * img1->widthStep);
				*(ptr + img->nChannels * i) = *(ptr1 + img->nChannels * x);
				if (img->nChannels == 3) {
					*(ptr + img->nChannels * i + 1) = *(ptr1 + img->nChannels * x + 1);
					*(ptr + img->nChannels * i + 2) = *(ptr1 + img->nChannels * x + 2);
				}
			}
			tw += count;
			tpcap += count;
			tqcap += count;
		}
	}
	cvSaveImage(output, img);
	free(qcap);
	free(pcap);
	free(qstar);
	free(pstar);
	free(wi);
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
	const char* const short_options = "hi:o:a:";
	char initial[MAX_NAME_LEN];
	char output[MAX_NAME_LEN];
	int alpha;
	const struct option long_options[] = {
		{ "help",     0, NULL, 'h' },
		{ "initial",   1, NULL, 'i' },
		{ "output",   1, NULL, 'o' },
		{ "alpha",   1, NULL, 'a' },
		{ NULL,       0, NULL, 0}
	};

	/*save input arguments*/

	do {
		next_option = getopt_long (argc, argv, short_options,
				long_options, NULL);
		switch (next_option)
		{
			case 'h':
				help_deformation(argv[0]);
				return 0;
			case 'i':
				if (strlen(optarg) > MAX_NAME_LEN) {
					pr_err("Initial Name is too large\n");
					abort();
				}
				strcpy(initial, optarg);
				break;
			case 'o':
				if (strlen(optarg) > MAX_NAME_LEN) {
					pr_err("Output Name is too large\n");
					abort();
				}
				strcpy(output, optarg);
				break;
			case 'a':
				alpha = atoi(optarg);
				break;
			case -1:
				break;
			default:
				abort ();
		}
	} while (next_option != -1);

	pt.ctrl_1count = 0;
	pt.ctrl_2count = 0;
	pt.end_selection = 0;

	get_points(initial, &pt);
	apply_deformation(initial, output, &pt, alpha);
}
