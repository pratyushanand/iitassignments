#include <cv.h>
#include <getopt.h>
#include <highgui.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_NAME_LEN	50
#define pr_info(args...) fprintf(stdout, ##args)
#define pr_err(args...) fprintf(stdout, ##args)

static void help_superposition (char *prog_name)
{
	pr_info ("Usage: %s options [ input ... ]\n", prog_name);
	pr_info (" -h	--help		Display this usage information.\n"
		" -i	--input1	Original Input Image Name.\n"
		" -j	--input2	Line art Input Image Name.\n"
		" -o	--output	Output Image Name.\n");
	return;
}

/*******************************************************************************/
/* Part 2.6: Superposition of Line Art with */
/*******************************************************************************/
static void superposition(IplImage *img1, IplImage *img2, IplImage *img3)
{
	int x, y;
	uchar *ptr1, *ptr2, *ptr3;

	for (y = 0; y < img1->height; y++) {
		ptr1 = (uchar*) (img1->imageData + y * img1->widthStep);
		ptr2 = (uchar*) (img2->imageData + y * img2->widthStep);
		ptr3 = (uchar*) (img3->imageData + y * img3->widthStep);
		for (x = 0; x < img1->width; x++) {
			/* If an edge point, then write it to final image */
			/* else write original image point to final image */
			if (*(ptr2 + x) == 0)
				*(ptr3 + x) = *(ptr2 + x);
			else
				*(ptr3 + x) = *(ptr1 + x);
		}
	}
}

/*******************************************************************************/
/* Main function */
/*******************************************************************************/
int main(int argc, char** argv)
{

	int next_option, err = 0;
	const char* const short_options = "hi:j:o:";
	char input1[MAX_NAME_LEN];
	char input2[MAX_NAME_LEN];
	char output[MAX_NAME_LEN];
	IplImage *img1, *img2, *img3;
	const struct option long_options[] = {
		{ "help",     0, NULL, 'h' },
		{ "input1",   1, NULL, 'i' },
		{ "input2",   1, NULL, 'j' },
		{ "output",   1, NULL, 'o' },
		{ NULL,       0, NULL, 0   }
	};
	/* get all user input in local variables */
	do {
		next_option = getopt_long (argc, argv, short_options,
				long_options, NULL);
		switch (next_option) {
			case 'h':
				help_superposition(argv[0]);
				return 0;
			case 'i':
				if (strlen(optarg) > MAX_NAME_LEN) {
					pr_err("Input Name is too large\n");
					abort();
				}
				strcpy(input1, optarg);
				break;
			case 'j':
				if (strlen(optarg) > MAX_NAME_LEN) {
					pr_err("Input Name is too large\n");
					abort();
				}
				strcpy(input2, optarg);
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
	img1 = cvLoadImage(input1, CV_LOAD_IMAGE_ANYCOLOR);
	if (!img1){
		pr_err("Error in reading input image %s\n", input1);
		return -1;
	}
	img2 = cvLoadImage(input2, CV_LOAD_IMAGE_ANYCOLOR);
	if (!img1){
		pr_err("Error in reading input image %s\n", input2);
		goto free_img1;
	}

	if (img1->nChannels != 1 || img1->depth != 8 || img2->nChannels != 1 || img2->depth != 8) {
		pr_err("Only Gray Scale Image with 8 bit depth is Supported\n");
		err = -2;
		goto free_img2;
	}
	/* Create memory for output image */
	img3 = cvCreateImage(cvGetSize(img1), IPL_DEPTH_8U, 1);
	if (!img3){
		pr_err("Error in creating gray scale image \n");
		err = -1;
		goto free_img2;
	}
	cvZero(img3);
	superposition(img1, img2, img3);
	cvSaveImage(output, img3, 0);

	cvReleaseImage(&img3);
free_img2:
	cvReleaseImage(&img2);
free_img1:
	cvReleaseImage(&img1);
     	return err;
}
