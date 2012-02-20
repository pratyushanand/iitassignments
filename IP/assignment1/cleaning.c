#include <cv.h>
#include <getopt.h>
#include <highgui.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_NAME_LEN	50
#define pr_info(args...) fprintf(stdout, ##args)
#define pr_err(args...) fprintf(stdout, ##args)


static void help_detect (char *prog_name)
{
	pr_info ("Usage: %s options [ inputfile ... ]\n", prog_name);
	pr_info (" -h	--help		Display this usage information.\n"
		" -i	--input		Input Image Name.\n"
		" -o	--output	Output Image Name.\n"
		" -x	--wx	neighbourhood window horizontal size\n"
		" -y	--wy	neighbourhood window vertical size\n"
		" -t	--thd	threshold number of points in window\n");

	return;
}

/*******************************************************************************/
/* Part 2.5: Edge cleaning, Self Implemented */
/*******************************************************************************/
static int get_edge_pt(uchar *ptr, int width, int wx, int wy)
{
	int i, j, pt = 0;
	for (j = 0; j < wy; j++) {
		for (i = 0; i < wx; i++) {
			if (*(ptr + width * j + i) == 0)
				pt++;
		}
	}
	return pt;
}

static void clean_edge_pt(uchar *ptr, int width, int wx, int wy)
{
	int i, j, pt = 0;
	for (j = 0; j < wy; j++)
		for (i = 0; i < wx; i++)
			*(ptr + width * j + i) = 255;
}

static void copy_edge_pt(uchar *sptr, uchar *dptr, int width, int wx, int wy)
{
	int i, j, pt = 0;
	for (j = 0; j < wy; j++)
		for (i = 0; i < wx; i++)
			*(dptr + width * j + i) = *(sptr + width * j + i);
}

static void cleaning(IplImage *img1, IplImage *img2, int wx, int wy, int thd)
{
	int x, y, j;
	uchar *sptr, *pptr, *dptr;

	for (y = 0; y < img1->height; y = y + wy) {
		sptr= (uchar*) (img1->imageData + y * img1->widthStep);
		dptr= (uchar*) (img2->imageData + y * img2->widthStep);
		for (x = 0; x < img1->width ; x = x + wx) {
			if (get_edge_pt(sptr + x, img1->widthStep, wx, wy) < thd)
				clean_edge_pt(dptr + x, img1->widthStep, wx, wy);
			else
				copy_edge_pt(sptr + x, dptr + x, img1->widthStep, wx, wy);
			if ((x + 2 * wx) > img1->width) {
				x += wx;
				while ( x < img1->width) {
					for (j = 0; j < wy; j++)
						*(dptr + img1->widthStep * j + x) = *(sptr + img1->widthStep * j + x);
					x++;
				}
			}
		}
		if ((y + 2 * wy) > img1->height) {
			y += wy;
			while ( y < img1->height) {
				memcpy(dptr, sptr, img1->widthStep);
				y++;
			}
		}
	}
}
/*******************************************************************************/
/* Main function */
/*******************************************************************************/
int main(int argc, char** argv)
{

	int next_option, err = 0;
	const char* const short_options = "hi:o:x:y:t:";
	char input[MAX_NAME_LEN];
	char output[MAX_NAME_LEN];
	IplImage *img1, *img2;
	int wx, wy, thd;
	const struct option long_options[] = {
		{ "help",     0, NULL, 'h' },
		{ "input",   1, NULL, 'i' },
		{ "output",   1, NULL, 'o' },
		{ "wx",   1, NULL, 'x' },
		{ "wy",   1, NULL, 'y' },
		{ "thd",   1, NULL, 't' },
		{ NULL,       0, NULL, 0   }
	};
	/* get all user input in local variables */
	do {
		next_option = getopt_long (argc, argv, short_options,
				long_options, NULL);
		switch (next_option) {
			case 'h':
				help_detect(argv[0]);
				return 0;
			case 'i':
				if (strlen(optarg) > MAX_NAME_LEN) {
					pr_err("Input Name is too large\n");
					abort();
				}
				strcpy(input, optarg);
				break;
			case 'o':
				if (strlen(optarg) > MAX_NAME_LEN) {
					pr_err("Output Name is too large\n");
					abort();
				}
				strcpy(output, optarg);
				break;
			case 'x':
				wx = atoi(optarg);
				break;
			case 'y':
				wy = atoi(optarg);
				break;
			case 't':
				thd = atoi(optarg);
				break;
			case -1:
				break;
			default:
				pr_err("Wrong options\n");
				abort ();
		}
	} while (next_option != -1);
	/* Load input image */
	img1 = cvLoadImage(input, CV_LOAD_IMAGE_ANYCOLOR);
	if (!img1){
		pr_err("Error in reading input image %s\n", input);
		return -1;
	}
	if (img1->nChannels != 1 || img1->depth != 8) {
		pr_err("Only gary scale Image of depth 8 is Supported\n");
		err = -2;
		goto free_img1;
	}
	/* Create memory for ouput image */
	img2 = cvCreateImage(cvGetSize(img1), IPL_DEPTH_8U, 1);
	if (!img2){
		pr_err("Error in creating gray scale image \n");
		err = -1;
		goto free_img1;
	}
	cvZero(img2);
	cleaning(img1, img2, wx, wy, thd);
	cvSaveImage(output, img2, 0);

free_img2:
	cvReleaseImage(&img2);
free_img1:
	cvReleaseImage(&img1);
     	return err;
}
