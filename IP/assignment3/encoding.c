/*
 * this program does image encoding using Image Pyramids
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
#define MAX_NAME_LEN		30
#define pr_info(args...) fprintf(stdout, ##args)
//#define pr_dbg(args...) fprintf(stdout, ##args)
#define pr_dbg(args...)
#define pr_err(args...) fprintf(stdout, ##args)

/* Prints help for encoding program */
static void help_encoding (char *prog_name)
{
	FILE *stream = stdout;

	fprintf (stream, "Usage: %s options [ select... ]\n", prog_name);
	fprintf (stream,
		" -i	--input		Input Image Name.\n"
		" -o	--output	Output Image Name.\n"
		" -l	--level		Pyramid Levels.\n");
	exit (0);
}
static int kernel[5][5] = {
	{1, 4, 6, 4, 1},
	{4, 16, 24, 16, 4},
	{6, 24, 36, 24, 6},
	{4, 16, 24, 16, 4},
	{1, 4, 6, 4, 1},
};

static void convolution(IplImage *in, IplImage *out)
{
	int height = in->height;
	int width = in->width;
	int ch = in->nChannels;
	int i, j, sum, m , n, mm, nn, ii, jj;
	uchar *inp, *outp;

	for(j = 0; j < height; ++j) {
		outp = (uchar*) (out->imageData + j * out->widthStep);
		for(i = 0; i < width; ++i) {
			sum = 0;
			for(m = 0; m < 5; ++m) {
				mm = 4 - m;
				for(n = 0; n < 5; ++n) {
					nn = 4 - n;
					ii = i + m - 3;
					jj = j + n - 3;
					if (ii >= 0 && ii < width
							&& jj >= 0 && jj < height) {
						inp = (uchar*) (in->imageData + jj * in->widthStep);
						*(outp + ch * i) += ((*(inp + ch * ii)) * kernel[mm][nn]) / 256;
						if (ch == 3) {
							*(outp + ch * i + 1) += ((*(inp + ch * ii + 1)) * kernel[mm][nn]) / 256;
							*(outp + ch * i + 2) += ((*(inp + ch * ii + 2)) * kernel[mm][nn]) / 256;
						}

					}
				}
			}
		}
	}

}

static void upsample(IplImage *imgd, IplImage *imgu)
{
	int width = imgu->width;
	int height = imgu->height;
	int ch = imgu->nChannels;
	uchar *imgup, *imgdp;
	int i, j;

	for (j = 0; j < height; j++) {
		imgup = (uchar*) (imgu->imageData + j * imgu->widthStep);
		imgdp = (uchar*) (imgd->imageData + (j / 2) * imgd->widthStep);
		for (i = 0; i < width; i++) {
			*(imgup + 3 * i) = *(imgdp + 3 * (i / 2));
			if (ch == 3) {
				*(imgup + 3 * i + 1) = *(imgdp + 3 * (i / 2) + 1);
				*(imgup + 3 * i + 2) = *(imgdp + 3 * (i / 2) + 2);
			}
		}
	}
}

static void downsample(IplImage *imgg, IplImage *imgd)
{
	int width = imgd->width;
	int height = imgd->height;
	int ch = imgd->nChannels;
	uchar *imggp, *imgdp;
	int i, j;

	for (j = 0; j < height * 2; j = j + 2) {
		imggp = (uchar*) (imgg->imageData + j * imgg->widthStep);
		imgdp = (uchar*) (imgd->imageData + (j / 2) * imgd->widthStep);
		for (i = 0; i < width * 2; i = i + 2) {
			*(imgdp + 3 * (i / 2)) = *(imggp + 3 * i);
			if (ch == 3) {
				*(imgdp + 3 * (i / 2) + 1) = *(imggp + 3 * i + 1);
				*(imgdp + 3 * (i / 2) + 2) = *(imggp + 3 * i + 2);
			}
		}
	}
}
static void encode(char *input, char *output, int level)
{
	IplImage *imgi, *imgo, *imgg, *imgl = NULL, *imgd = NULL, *imgu = NULL;
	int width, height, channel, i, j;
	uchar *imglp, *imgdp, *imggp;
	FILE *fp;
	uchar data, byte, c;
	int offset;

	imgi = cvLoadImage(input, 1);
	if (!imgi) {
		pr_err("Error in opening input file\n");
		return;
	}
	fp = fopen(output, "wb");
	width = imgi->width;
	height = imgi->height;
	channel = imgi->nChannels;
	pr_info("Input Size is %d x %d x %d\n", width, height, channel);
	/* prepare header of compressed file */
	fwrite(&width, sizeof(int), 1, fp);
	pr_dbg("%d ", width);
	fwrite(&height, sizeof(int), 1, fp);
	pr_dbg("%d ", height);
	fwrite(&channel, sizeof(int), 1, fp);
	pr_dbg("%d ", channel);
	fwrite(&level, sizeof(int), 1, fp);
	pr_dbg("%d ", level);
	offset = 16;
	imgg = cvCreateImage(cvSize(width, height), 8, channel);
	cvZero(imgg);
	convolution(imgi, imgg);
	while(level--) {
		if (imgu)
			cvReleaseImage(&imgu);
		imgu = cvCreateImage(cvSize(width, height), 8, channel);
		if (imgl)
			cvReleaseImage(&imgl);
		imgl = cvCreateImage(cvSize(width, height), 8, channel);
		width = width / 2;
		if (width % 2)
			width++;
		height = height / 2;
		if (height % 2)
			height++;
		if (imgd)
			cvReleaseImage(&imgd);
		imgd = cvCreateImage(cvSize(width, height), 8, channel);
		/* downsample: prepare g2*/
		downsample(imgg, imgd);
		/* upsample: prepare expand[g2]*/
		upsample(imgd, imgu);
		/* lplacian: g1 - expand[g2] */
		cvAbsDiff(imgg, imgu, imgl);
		cvReleaseImage(&imgg);
		imgg = cvCreateImage(cvSize(width, height), 8, channel);
		/* Save gausian for next interation */
		cvCopy(imgd, imgg);
		/* encode lapalcian */
		c = 6;
		byte = 0;
		for (j = 0; j < imgl->height; j++) {
			imglp = (uchar*) (imgl->imageData + j * imgl->widthStep);
			for (i = 0; i < imgl->width; i++) {
				data = *(imglp + 3 * i);
				data &= 0x0C;
				data >>= 2;
				data <<= c;
				byte |= data;
				if (c == 0) {
					fwrite(&byte, sizeof(uchar), 1, fp);
					offset++;
					pr_dbg("%d ", byte);
					c = 6;
					byte = 0;
				} else
					c -= 2;
				if (channel == 3) {
					data = *(imglp + 3 * i + 1);
					data &= 0x0C;
					data >>= 2;
					data <<= c;
					byte |= data;
					if (c == 0) {
						fwrite(&byte, sizeof(uchar), 1, fp);
						offset++;
						pr_dbg("%d ", byte);
						c = 6;
						byte = 0;
					} else
						c -= 2;
					data = *(imglp + 3 * i + 2);
					data &= 0x0C;
					data >>= 2;
					data <<= c;
					byte |= data;
					if (c == 0) {
						fwrite(&byte, sizeof(uchar), 1, fp);
						offset++;
						pr_dbg("%d ", byte);
						c = 6;
						byte = 0;
					} else
						c -= 2;
				}
			}
		}
		if (c != 6) {
			fwrite(&byte, sizeof(uchar), 1, fp);
			offset++;
			pr_dbg("%d ", byte);
		}
	}
	/* encode gausian */
	cvSaveImage("test1.tif", imgg);
	for (j = 0; j < height; j++) {
		imggp = (uchar*) (imgg->imageData + j * imgg->widthStep);
		for (i = 0; i < width; i++) {
			data = *(imggp + 3 * i);
			fwrite(&data, sizeof(uchar), 1, fp);
			offset++;
			pr_dbg("%d ", data);
			if (channel == 3) {
				data = *(imggp + 3 * i + 1);
				fwrite(&data, sizeof(uchar), 1, fp);
				offset++;
				pr_dbg("%d ", data);
				data = *(imggp + 3 * i + 2);
				fwrite(&data, sizeof(uchar), 1, fp);
				offset++;
				pr_dbg("%d ", data);
			}
		}
	}
	fclose(fp);

	cvReleaseImage(&imgu);
	cvReleaseImage(&imgl);
	cvReleaseImage(&imgd);
	cvReleaseImage(&imgg);
	cvReleaseImage(&imgi);
}
/*
 * main routine of this program. takes varibale arg.
 * details can be seen in help
 */
int main(int argc, char **argv)
{
	int next_option;
	const char* const short_options = "hi:o:l:";
	char input[MAX_NAME_LEN];
	char output[MAX_NAME_LEN];
	int level;
	const struct option long_options[] = {
		{ "help",     0, NULL, 'h' },
		{ "input",   1, NULL, 'i' },
		{ "output",   1, NULL, 'o' },
		{ "level",   1, NULL, 'l' },
		{ NULL,       0, NULL, 0}
	};

	/*save input arguments*/

	do {
		next_option = getopt_long (argc, argv, short_options,
				long_options, NULL);
		switch (next_option)
		{
			case 'h':
				help_encoding(argv[0]);
				return 0;
			case 'i':
				if (strlen(optarg) > MAX_NAME_LEN) {
					pr_err("input Name is too large\n");
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
			case 'l':
				level = atoi(optarg);
				if (level > 8) {
					pr_err("Level not supported\n");
					return - 1;
				}
				break;
			case -1:
				break;
			default:
				abort ();
		}
	} while (next_option != -1);
	encode(input, output, level);
}
