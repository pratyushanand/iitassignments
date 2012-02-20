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
		" -t	--thd 		to threshold soble differential.\n");
	return;
}


/*******************************************************************************/
/* Part 2.3.1: Edge detection, Self Implemented using Sobel operator */
/*******************************************************************************/
static int dx[3][3] = { { -1, -2, -1},
	  		{  0, 0,  0},
	  		{  1, 2,  1}};

static int dy[3][3] = { { -1, 0,  1},
	  		{ -2, 0,  2},
	  		{ -1, 0,  1}};
/* Convolution with Soble operator */
static int convolution(uchar *ptr1, uchar *ptr2, uchar *ptr3)
{
	int xconv = 0, yconv = 0, conv, i;

	for (i = 0; i < 3; i++)
		xconv += dx[i][0] * (*(ptr1 + i));	
	for (i = 0; i < 3; i++)
		xconv += dx[i][1] * (*(ptr2 + i));	
	for (i = 0; i < 3; i++)
		xconv += dx[i][2] * (*(ptr3 + i));	

	for (i = 0; i < 3; i++)
		yconv += dy[i][0] * (*(ptr1 + i));	
	for (i = 0; i < 3; i++)
		yconv += dy[i][1] * (*(ptr2 + i));	
	for (i = 0; i < 3; i++)
		yconv += dy[i][2] * (*(ptr3 + i));	

	conv = sqrt(xconv * xconv + yconv * yconv);

	return conv;
}

static void edge_detection(IplImage *img1, IplImage *img2, int th)
{
	int x, y, conv;
	uchar *sptr, *pptr, *dptr;

	for (y = 1; y < img1->height - 1; y++) {
		sptr= (uchar*) (img1->imageData + y * img1->widthStep);
		dptr= (uchar*) (img2->imageData + y * img2->widthStep);
		for (x = 1; x < img1->width - 1; x++) {
			pptr = sptr + x - 1;
			conv = convolution(pptr, pptr + img1->widthStep, pptr + 2 * img1->widthStep);
			/* threshold on the basis of covolution value */
			if (conv > th) {
				*(dptr + x) = 0;
			}
			else
				*(dptr + x) = 255;
		}
	}
}
/*******************************************************************************/
/* Main function */
/*******************************************************************************/
int main(int argc, char** argv)
{

	int next_option, err = 0;
	const char* const short_options = "hi:o:t:";
	char input[MAX_NAME_LEN];
	char output[MAX_NAME_LEN];
	IplImage *img1, *img2;
	int thd;
	const struct option long_options[] = {
		{ "help",     0, NULL, 'h' },
		{ "input",   1, NULL, 'i' },
		{ "output",   1, NULL, 'o' },
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
	/* Create memory for intermediate images */
	img2 = cvCreateImage(cvGetSize(img1), IPL_DEPTH_8U, 1);
	if (!img2){
		pr_err("Error in creating gray scale image \n");
		err = -1;
		goto free_img1;
	}
	cvZero(img2);
	edge_detection(img1, img2, thd);
	cvSaveImage(output, img2, 0);

free_img2:
	cvReleaseImage(&img2);
free_img1:
	cvReleaseImage(&img1);
     	return err;
}
