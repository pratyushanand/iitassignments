#include <cv.h>
#include <getopt.h>
#include <highgui.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_NAME_LEN	50
#define pr_info(args...) fprintf(stdout, ##args)
#define pr_err(args...) fprintf(stdout, ##args)


static void help_edge_link (char *prog_name)
{
	pr_info ("Usage: %s options [ input ... ]\n", prog_name);
	pr_info (" -h	--help		Display this usage information.\n"
		" -i	--input		Input Image Name.\n"
		" -o	--output	Output Image Name.\n"
		" -t	--thl to threshold number of points in a line for linking.\n");
	return;
}
/*******************************************************************************/
/* Part 2.3.2: Edge Linking, Self Implemented using Hough trasform */
/*******************************************************************************/
#define PI	3.1415926535

static void draw_line(IplImage *img, float rho, float theta)
{
	int x, y;
	float fx, fy;
	uchar *ptr;

	if (img->height > img->width) {
		for (y = 0; y < img->height; y++) {
			fy = y;
			if (theta == -90 || theta == 90)
				x = rho;
			else
				x = round((rho - fy * sin(theta * PI / 180.0)) / cos (theta * PI / 180.0));
			if (x < 0 || x > img->width)
				continue;
			ptr= (uchar*) (img->imageData + y * img->widthStep);
			*(ptr + x) = 0;
		}
	} else {
		for (x = 0; x < img->width; x++) {
			fx = x;
			if (theta == 0)
				y = rho;
			else
				y = round((rho - fx * cos(theta * PI / 180.0)) / sin (theta * PI / 180.0));
			if (y < 0 || y > img->height)
				continue;
			ptr= (uchar*) (img->imageData + y * img->widthStep);
			*(ptr + x) = 0;
		}
	}
}

static int edge_linking(IplImage *img1, IplImage *img2, int thl)
{
	int width, height, rhoi, thetai, x, y, rhomax, err = 0;
	int *acc, m, startx = 0, starty = 0, endx = 0, endy = 0, i, j, a;
	uchar *ptr;
	float fx, fy, rho, theta;
	

	width = img1->width;
	height = img1->height;
	rhomax = sqrt(width * width + height * height);

	/* -90 <= theta <= 90 */
	/* -rhomax <= rho <= rhomax */
	acc = calloc(181 * (2 * rhomax + 1), sizeof(int));
	if (!acc){
		pr_err("Error in creating Accumulator space\n");
		return -1;
	}
	
	/* Prepare Accumulator */
	for (y = 0; y < height; y++) {
		ptr= (uchar*) (img1->imageData + y * img1->widthStep);
		for (x = 0; x < width; x++) {
			if ((*(ptr + x)) == 0) {
				fx = x;
				fy = y;
				for (theta = -90; theta <= 90; theta++) {
					rhoi = round(fx  * cos (theta * PI / 180)
					+ fy  * sin (theta * PI / 180)); 
					thetai = theta;
					acc[181 * (rhomax + rhoi) + thetai + 90]++;
				}
			}
		}
	}
	/* First Copy input Image to the output area */
	cvCopy(img1, img2, 0);

	/* link now if threshold no of points are available */
	/* Now start looking for points to be linked */
	/* this program will only link points in a straight line */
	for (rhoi = -rhomax; rhoi <= rhomax; rhoi++) {
		for (thetai = -90; thetai <= 90; thetai++) {
			a = acc[181 * (rhomax + rhoi) + thetai + 90];
			if (a > thl)
				draw_line(img2, rhoi, thetai);
		}
	}
	free(acc);
	return err;
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
	int thl;
	const struct option long_options[] = {
		{ "help",     0, NULL, 'h' },
		{ "input",   1, NULL, 'i' },
		{ "output",   1, NULL, 'o' },
		{ "thl",   1, NULL, 't' },
		{ NULL,       0, NULL, 0   }
	};
	/* get all user input in local variables */
	do {
		next_option = getopt_long (argc, argv, short_options,
				long_options, NULL);
		switch (next_option) {
			case 'h':
				help_edge_link(argv[0]);
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
				thl = atoi(optarg);
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
		pr_err("Only gray scale image with depth 8 is Supported\n");
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
	edge_linking(img1, img2, thl);
	cvSaveImage(output, img2, 0);

free_img2:
	cvReleaseImage(&img2);
free_img1:
	cvReleaseImage(&img1);
     	return err;
}
