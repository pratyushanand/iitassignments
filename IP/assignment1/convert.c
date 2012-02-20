#include <cv.h>
#include <getopt.h>
#include <highgui.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_NAME_LEN	50
#define pr_info(args...) fprintf(stdout, ##args)
#define pr_err(args...) fprintf(stdout, ##args)


static void help_sketch (char *prog_name)
{
	pr_info ("Usage: %s options [ inputfile ... ]\n", prog_name);
	pr_info (" -h	--help		Display this usage information.\n"
		" -i	--input		Input Image Name.\n"
		" -o	--output	Output Image Name.\n");
	return;
}

/*******************************************************************************/
/* Part 2.1: Convert RGB to Gray Scale using standrd function*/
/*******************************************************************************/

static void convert(IplImage *img1, IplImage *img2)
{
	/* If Image is RGBthen , Convert Image from RGB to GrayScale */

	if (img1->nChannels == 3)
		cvCvtColor(img1, img2, CV_RGB2GRAY);
	else 
		cvCopyImage(img1, img2);
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
				help_sketch(argv[0]);
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
	if ((img1->nChannels != 1 && img1->nChannels != 3) || img1->depth != 8) {
		pr_err("Image not Supported\n");
		err = -2;
		goto free_img1;
	}
	/* Create memory for output image */
	img2 = cvCreateImage(cvGetSize(img1), IPL_DEPTH_8U, 1);
	if (!img2){
		pr_err("Error in creating gray scale image \n");
		err = -1;
		goto free_img1;
	}

	convert(img1, img2);

	cvSaveImage(output, img2, 0);

free_img2:
	cvReleaseImage(&img2);
free_img1:
	cvReleaseImage(&img1);
     	return err;
}
