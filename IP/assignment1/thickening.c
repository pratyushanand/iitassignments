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
		" -o	--output	Output Image Name.\n");
	return;
}


/*******************************************************************************/
/* Part 2.4: Edge thickeing, Self Implemented */
/*******************************************************************************/
static int nb[3][3] = { { 1, 1, 1},
	  		{  1, 0,  1},
	  		{  1, 1,  1}};

/* Check neighbouring point for and edge */
static int check_neighbour(uchar *ptr1, uchar *ptr2, uchar *ptr3)
{
	int i;

	for (i = 0; i < 3; i++)
		if (nb[i][0] && (*(ptr1 + i) == 0))
			return 1;
	for (i = 0; i < 3; i++)
		if (nb[i][1] && (*(ptr2 + i) == 0))
			return 1;
	for (i = 0; i < 3; i++)
		if (nb[i][2] && (*(ptr3 + i) == 0))
			return 1;

	return 0;
}

static void thickening(IplImage *img1, IplImage *img2)
{
	int x, y, neighbour;
	uchar *sptr, *pptr, *dptr;

	for (y = 1; y < img1->height - 1; y++) {
		sptr= (uchar*) (img1->imageData + y * img1->widthStep);
		dptr= (uchar*) (img2->imageData + y * img2->widthStep);
		for (x = 1; x < img1->width - 1; x++) {
			if (*(sptr + x) == 0) {
				/* its alreday an edge point, just copy it*/
				*(dptr + x) = *(sptr + x);
			} else {
				pptr = sptr + x - 1;
				neighbour = check_neighbour(pptr, pptr + img1->widthStep, pptr + 2 * img1->widthStep);
				/* If any of the 8 pixel is an edge point then, make current point as edge point also*/
				if (neighbour) {
					*(dptr + x) = 0;
				}
				else
					*(dptr + x) = 255;
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
	const char* const short_options = "hi:o:";
	char input[MAX_NAME_LEN];
	char output[MAX_NAME_LEN];
	IplImage *img1, *img2;
	const struct option long_options[] = {
		{ "help",     0, NULL, 'h' },
		{ "input",   1, NULL, 'i' },
		{ "output",   1, NULL, 'o' },
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
	thickening(img1, img2);
	cvSaveImage(output, img2, 0);

free_img2:
	cvReleaseImage(&img2);
free_img1:
	cvReleaseImage(&img1);
     	return err;
}
