#include <cv.h>
#include <getopt.h>
#include <highgui.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_NAME_LEN	50
#define pr_info(args...) fprintf(stdout, ##args)
#define pr_err(args...) fprintf(stdout, ##args)


void help_scale (char *prog_name)
{
	pr_info ("Usage: %s options [ inputfile ... ]\n", prog_name);
	pr_info (" -h	--help		Display this usage information.\n"
		" -i	--input		Input Image Name.\n"
		" -o	--output	Output Image Name.\n"
		" -s	--sf		Scaling Factor.\n"
		" -m	--method	Method can be replicate or interpolate.\n");
	return;
}

void scale_replicate(IplImage *img1, IplImage *img2, float sf)
{
	int w2, h2, w1 = 0, h1 = 0, ch = img1->nChannels;

	for (h2 = 0; h2 < img2->height; h2++) {
		uchar *ptr1, *ptr2, *ptr;
		ptr2= (uchar*) (img2->imageData + h2 * img2->widthStep);
		
		if ((h2 != ceil (h1 * sf)) && sf >= 1)
			memcpy(ptr2, ptr, img2->widthStep);
		else {
			ptr1= (uchar*) (img1->imageData + h1 * img1->widthStep);
			w1 = 0;
			for (w2 = 0; w2 < img2->width; w2++) {
				ptr2[ch * w2] = ptr1[ch * w1];
				if (ch == 3) {
					ptr2[ch * w2 + 1] = ptr1[ch * w1 + 1];
					ptr2[ch * w2 + 2] = ptr1[ch * w1 + 2];
				}
				/* for sf >= 1 */
				if ((w2 + 1) >= ceil((w1 + 1) * sf)) 
					w1++;
				/* for sf < 1 */
				while ((w2 + 1) > ceil((w1 + 1) * sf)) 
					w1++;
			}
			ptr = ptr2;
		}
		/* for sf >= 1 */
		if ((h2 + 1) >= ceil((h1 + 1) * sf)) 
			h1++;
		/* for sf < 1 */
		while ((h2 + 1) > ceil((h1 + 1) * sf)) 
			h1++;
	}
}

void scale_interpolate(IplImage *img1, IplImage *img2, float sf)
{
	int w1, h1, r11, g11, b11, r21, g21, b21, r12, g12, b12, r22, g22, b22, x, y, x1, y1, x2, y2, ch = img1->nChannels;

	for (h1 = 0; h1 < img1->height - 1; h1++) {
		uchar *ptr11, *ptr12, *ptr2;
		y1 = ceil(h1 * sf);
		y2 = ceil((h1 + 1) * sf) - 1;
		ptr11 = (uchar*) (img1->imageData + h1 * img1->widthStep);
		ptr12 = (uchar*) (img1->imageData + (h1 + 1) * img1->widthStep);
		ptr2 = (uchar*) (img2->imageData + y1 * img2->widthStep);
		for (w1 = 0; w1 < img1->width -1 ; w1++) {
			x1 = ceil(w1 * sf);
			x2 = ceil((w1 + 1) * sf) - 1;
			r11 = ptr11[ch * w1];
			g11 = ptr11[ch * w1 + 1];
			b11 = ptr11[ch * w1 + 2];
			r21 = ptr11[ch * w1 + 3];
			g21 = ptr11[ch * w1 + 4];
			b21 = ptr11[ch * w1 + 5];
			r12 = ptr12[ch * w1];
			g12 = ptr12[ch * w1 + 1];
			b12 = ptr12[ch * w1 + 2];
			r22 = ptr12[ch * w1 + 3];
			g22 = ptr12[ch * w1 + 4];
			b22 = ptr12[ch * w1 + 5];
			for (x = x1; x <= x2; x++) {
				for (y = y1; y <= y2; y++) {
					if ((x1 != x2) && (y1 != y2)) {
						ptr2[((y - y1) * img2->widthStep) + (ch * x)] = (r11 * (x2 -x) * (y2 - y) +
								r21 * (x -x1) * (y2 - y) + r12 * (x2 -x) * (y - y1) +
								r22 * (x -x1) * (y - y1)) / (x2 - x1) / (y2 -y1);
						if (ch == 3) {
							ptr2[((y - y1) * img2->widthStep) + (ch * x) + 1] = (g11 * (x2 -x) * (y2 - y) +
									g21 * (x -x1) * (y2 - y) + g12 * (x2 -x) * (y - y1) +
									g22 * (x -x1) * (y - y1)) / (x2 - x1) / (y2 -y1);
							ptr2[((y - y1) * img2->widthStep) + (ch * x) + 2] = (b11 * (x2 -x) * (y2 - y) +
									b21 * (x -x1) * (y2 - y) + b12 * (x2 -x) * (y - y1) +
									b22 * (x -x1) * (y - y1)) / (x2 - x1) / (y2 -y1);
						}
					} else if ((x1 == x2) && (y1 != y2)) {
						ptr2[((y - y1) * img2->widthStep) + (ch * x)] = (r11 * (y2 -y) +
								r12 * (y - y1)) / (y2 - y1);
						if (ch == 3) {
							ptr2[((y - y1) * img2->widthStep) + (ch * x) + 1] = (g11 * (y2 -y) +
									r12 * (y - y1)) / (y2 - y1);
							ptr2[((y - y1) * img2->widthStep) + (ch * x) + 2] = (b11 * (y2 -y) +
									r12 * (y - y1)) / (y2 - y1);
						}
					} else if ((x1 != x2) && (y1 == y2)) {
						ptr2[((y - y1) * img2->widthStep) + (ch * x)] = (r11 * (x2 -x) +
								r21 * (x - x1)) / (x2 - x1);
						if (ch == 3) {
							ptr2[((y - y1) * img2->widthStep) + (ch * x) + 1] = (g11 * (x2 -x) +
									r21 * (x - x1)) / (x2 - x1);
							ptr2[((y - y1) * img2->widthStep) + (ch * x) + 2] = (b11 * (x2 -x) +
									r21 * (x - x1)) / (x2 - x1);
						}
					} else {
						ptr2[((y - y1) * img2->widthStep) + (ch * x)] = r11;
						if (ch == 3) {
							ptr2[((y - y1) * img2->widthStep) + (ch * x) + 1] = g11;
							ptr2[((y - y1) * img2->widthStep) + (ch * x) + 2] = b11;
						}
					}
				}
			}
		}
	}
}

int main(int argc, char** argv)
{

	int next_option, err = 0;
	float sf = 1.0;
	const char* const short_options = "hi:o:s:m:";
	char input[MAX_NAME_LEN];
	char output[MAX_NAME_LEN];
	char method[12];
	IplImage *img1, *img2;
	CvSize osize;
	const struct option long_options[] = {
		{ "help",     0, NULL, 'h' },
		{ "input",   1, NULL, 'i' },
		{ "output",   1, NULL, 'o' },
		{ "sf",   1, NULL, 's' },
		{ "method",   1, NULL, 'm' },
		{ NULL,       0, NULL, 0   }
	};

	/* get all user input in local variables */
	do {
		next_option = getopt_long (argc, argv, short_options,
				long_options, NULL);
		switch (next_option) {
			case 'h':
				help_scale(argv[0]);
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
				sf = atof(optarg);
				break;
			case 'm':
				if (strlen(optarg) > 12 ||
					(strcmp(optarg, "replicate") &&
					strcmp(optarg, "interpolate"))) {
					pr_err("Method not supported\n");
					abort();
				}
				strcpy(method, optarg);
				break;
			case -1:
				break;
			default:
				abort ();
		}
	} while (next_option != -1);

	img1 = cvLoadImage(input, CV_LOAD_IMAGE_ANYCOLOR);
	if (!img1){
		pr_err("Error in reading input image %s\n", input);
		return -1;
	}
	if (img1->nChannels != 1 && img1->nChannels != 3 && img1->depth != 8) {
		pr_err("Image not Supported\n");
		err = -2;
		goto free_img1;
	}
		
	osize.height = ceil(img1->height * sf);
	osize.width = ceil(img1->width * sf);
	img2 = cvCreateImage(osize, img1->depth, img1->nChannels);
	if (!img2){
		pr_err("Error in creating output image \n");
		err = -1;
		goto free_img1;
	}
	img2->origin = img1->origin;
	cvZero(img2);
	if (!strcmp(method, "replicate"))
		scale_replicate(img1, img2, sf);
	else
		scale_interpolate(img1, img2, sf);
	cvSaveImage(output, img2, 0);
	cvReleaseImage(&img2);
free_img1:
	cvReleaseImage(&img1);
     	return err;
}
