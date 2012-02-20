/*
 * this program does image decoding using Image Pyramids
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

/* Prints help for decoding program */
static void help_decoding (char *prog_name)
{
	FILE *stream = stdout;

	fprintf (stream, "Usage: %s options [ select... ]\n", prog_name);
	fprintf (stream,
		" -i	--input		Input Image Name.\n"
		" -o	--output	Output Image Name.\n");
	exit (0);
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

static void decode(char *input, char *output)
{
	IplImage *imgd, *imgu = NULL;
	FILE *fp;
	uchar *data, *imgdp, *imgup;
	int width, height, channel, level, i, j, l, offset, size, c;
	int loff[9];
	uchar byte;

	fp = fopen(input, "rb");
	if (!fp) {
		pr_err("Error in opening input file\n");
		return;
	}
	fseek(fp, 0, SEEK_END);
	size = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	data = (uchar *)malloc(size);
	fread(data, sizeof(uchar), size, fp);
	fclose(fp);

	/* extract header of compressed file */
	width = *((int *)&data[0]);
	pr_dbg("%d ", width);
	height = *((int *)&data[4]);
	pr_dbg("%d ", height);
	channel = *((int *)&data[8]);
	pr_dbg("%d ", channel);
	level = *((int *)&data[12]);
	pr_dbg("%d ", level);

	for (i = 16; i < size; i++)
		pr_dbg("%d ", data[i]);
	offset = 16;
	for (l = 0; l < level; l++) {
		loff[l] = offset;
		offset += (width * height * channel / 4);
		if (width * height * channel % 4)
			offset++;
		width = width / 2;
		if (width % 2)
			width++;
		height = height / 2;
		if (height % 2)
			height++;
	}
	loff[l] = offset;
	imgd = cvCreateImage(cvSize(width, height), 8, channel);
	for (j = 0; j < height; j++) {
		imgdp = (uchar*) (imgd->imageData + j * imgd->widthStep);
		for (i = 0; i < width; i++) {
			*(imgdp + 3 * i) = data[offset];
			offset++;
			if (channel == 3) {
				*(imgdp + 3 * i + 1) = data[offset];
				offset++;
				*(imgdp + 3 * i + 2) = data[offset];
				offset++;
			}
		}
	}
	for (l = 0; l < level; l++) {
		if (l == level - 1) {
			width = *((int *)&data[0]);
			height = *((int *)&data[4]);
		} else {
			width *= 2;
			height *= 2;
		}
		imgu = cvCreateImage(cvSize(width , height), 8, channel);
		/* expand gausian */
		upsample(imgd, imgu);
		/* add laplacian */
		offset = loff[level - l - 1];
		c = 6;
		for (j = 0; j < height; j++) {
			imgup = (uchar*) (imgu->imageData + j * imgu->widthStep);
			for (i = 0; i < width; i++) {
				byte = data[offset];
				byte >>= c;
				byte &= 0x3;
				byte <<= 2;
				*(imgup + 3 * i) += byte;
				if (c == 0) {
					c = 6;
					offset++;
				} else
					c -= 2;
				if (channel == 3) {
					byte = data[offset];
					byte >>= c;
					byte &= 0x3;
					byte <<= 2;
					*(imgup + 3 * i + 1) += byte;
					if (c == 0) {
						c = 6;
						offset++;
					} else
						c -= 2;
					byte = data[offset];
					byte >>= c;
					byte &= 0x3;
					byte <<= 2;
					*(imgup + 3 * i + 2) += byte;
					if (c == 0) {
						c = 6;
						offset++;
					} else
						c -= 2;

				}
			}
		}
		/* save upsampled for next iteration */
		cvReleaseImage(&imgd);
		imgd = cvCreateImage(cvSize(width, height), 8, channel);
		cvCopy(imgu, imgd);
	}
	cvSaveImage(output, imgd);
	free(data);
	cvReleaseImage(&imgd);
	cvReleaseImage(&imgu);
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
				help_decoding(argv[0]);
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
			case -1:
				break;
			default:
				abort ();
		}
	} while (next_option != -1);

	decode(input, output);
}
