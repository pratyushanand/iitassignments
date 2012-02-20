#include <cv.h>
#include <getopt.h>
#include <highgui.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_NAME_LEN	50
#define pr_info(args...) fprintf(stdout, ##args)
#define pr_err(args...) fprintf(stdout, ##args)

enum options {
SMOOTH,
HIST_EQUALIZE,
};

static void help_enhancement (char *prog_name)
{
	pr_info ("Usage: %s options [ input ... ]\n", prog_name);
	pr_info (" -h	--help		Display this usage information.\n"
		" -i	--input		Input Image Name.\n"
		" -o	--output	Output Image Name.\n"
		" -s	--smooth	to smooth the image\n"
		" -t	--smoothtype	smooth type, can be mean, median, gaussian or bilateral \n"
		" -x	--wx	local neighbourhood window horizontal size\n"
		" -y	--wy	local neighbourhood window vertical size\n"
		" -e	--hist_equalize	to equalize the histogram.\n");
	return;
}

/*******************************************************************************/
/* Part 2.2: Preprocessing using standrd functions */
/*******************************************************************************/
static void smooth(IplImage *img1, IplImage *img2, int type, int x, int y)
{
	cvSmooth(img1, img2, type, x, y, 0 , 0);	
}

static void hist_equalize(IplImage *img1, IplImage *img2)
{
	cvEqualizeHist(img1, img2);	
}

/*******************************************************************************/
/* Main function */
/*******************************************************************************/
int main(int argc, char** argv)
{

	int next_option, err = 0;
	const char* const short_options = "hesi:o:t:x:y:";
	char input[MAX_NAME_LEN];
	char output[MAX_NAME_LEN];
	IplImage *img1, *img2;
	enum options option;
	int smoothtype, wx, wy, thd, thl, thg;
	const struct option long_options[] = {
		{ "help",     0, NULL, 'h' },
		{ "input",   1, NULL, 'i' },
		{ "output",   1, NULL, 'o' },
		{ "smoothtype",   1, NULL, 't' },
		{ "wx",   1, NULL, 'x' },
		{ "wy",   1, NULL, 'y' },
		{ "hist_equalize",   0, NULL, 'e' },
		{ "smooth",   0, NULL, 's' },
		{ NULL,       0, NULL, 0   }
	};
	/* get all user input in local variables */
	do {
		next_option = getopt_long (argc, argv, short_options,
				long_options, NULL);
		switch (next_option) {
			case 'h':
				help_enhancement(argv[0]);
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
			case 's':
				option = SMOOTH;
				break;
			case 't':
				if (strcmp(optarg, "mean") == 0)
					smoothtype = CV_BLUR;
				else if (strcmp(optarg, "median") == 0)
					smoothtype = CV_MEDIAN;
				else if (strcmp(optarg, "gaussian") == 0)
					smoothtype = CV_GAUSSIAN;
				else if (strcmp(optarg, "bilateral") == 0)
					smoothtype = CV_BILATERAL;
				break;
			case 'x':
				wx = atoi(optarg);
				break;
			case 'y':
				wy = atoi(optarg);
				break;
			case 'e':
				option = HIST_EQUALIZE;
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
		pr_err("Only Gray Scale Image with 8 bit depth is Supported\n");
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
	switch (option) {
		case SMOOTH:
			smooth(img1, img2, smoothtype, wx, wy);
		case HIST_EQUALIZE:
			hist_equalize(img1, img2);
			break;
		default:
			pr_err("Wrong test case\n");
			abort();
	}

	cvSaveImage(output, img2, 0);

free_img2:
	cvReleaseImage(&img2);
free_img1:
	cvReleaseImage(&img1);
     	return err;
}
