#include <cv.h>
#include <getopt.h>
#include <highgui.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_NAME_LEN	50
#define pr_info(args...) fprintf(stdout, ##args)
#define pr_err(args...) fprintf(stdout, ##args)


static void help_halftone (char *prog_name)
{
	pr_info ("Usage: %s options [ input ... ]\n", prog_name);
	pr_info (" -h	--help		Display this usage information.\n"
		" -i	--input		Input Image Name.\n"
		" -o	--output	Output Image Name.\n");
	return;
}
/*******************************************************************************/
/*
* Part 1: Halftoning , Area based approach with Floyd Steinberg error correction 
* Method. Expect grayscale image as input.
*/
/*******************************************************************************/
static int halftone(IplImage *img1, IplImage *img2)
{
	int x, y, av, height, width, val, delta;	
	uchar *ptr1, *ptr2;

	height = img1->height;
	width = img1->width;

	for (y = 0; y < height - 1; y++) {
		ptr1= (uchar*) (img1->imageData + y * width);
		ptr2= (uchar*) (img2->imageData + y * width);
		for (x = 0; x < width - 1; x++) {
			/* threshold 00 */
			if (*(ptr1 + x) > 127)
				*(ptr2 + x) = 255;
			else
				*(ptr2 + x) = 0;

			/* calculate error due to 00 */
			delta = *(ptr1 + x) - *(ptr2 + x);

			/* Diffuse the error as per Floyd Steinberg Algorithm */
			/* 01 -> 01 + 3 / 8* delta */
			*(ptr1 + x + 1) += 3 * delta / 8;
			/* 10 -> 10 + 3 / 8* delta */
			*(ptr1 + width + x) += 3 * delta / 8;
			/* 11 -> 11 + 1 / 4* delta */
			*(ptr1 + width + x + 1) += delta / 4;
		}
		/* for the last pixel of the row only threshold*/
		if (*(ptr1 + x) > 127)
			*(ptr2 + x) = 255;
		else
			*(ptr2 + x) = 0;
	}
	/* for the last row only threshold*/
	ptr1= (uchar*) (img1->imageData + y * width);
	ptr2= (uchar*) (img2->imageData + y * width);
	for (x = 0; x < width; x++) {
		if (*(ptr1 + x) > 127)
			*(ptr2 + width) = 255;
		else
			*(ptr2 + x) = 0;
	}
	return 0;
}

/*******************************************************************************/
/* Main function*/
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
				help_halftone(argv[0]);
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
	/* Load input image*/
	img1 = cvLoadImage(input, CV_LOAD_IMAGE_ANYCOLOR);
	if (!img1){
		pr_err("Error in reading input image %s\n", input);
		return -1;
	}
	if (img1->nChannels != 1 || img1->depth != 8) {
		pr_err("Only Gray scale image of 8 bit depth is supported\n");
		err = -2;
		goto free_img1;
	}
	/* Create memory for ouput image */
	img2 = cvCreateImage(cvGetSize(img1), IPL_DEPTH_8U, 1);
	if (!img2){
		pr_err("Error in creating output image \n");
		err = -1;
		goto free_img1;
	}
	cvZero(img2);
	halftone(img1, img2);
	cvSaveImage(output, img2, 0);

free_img2:
	cvReleaseImage(&img2);
free_img1:
	cvReleaseImage(&img1);
     	return err;
}
