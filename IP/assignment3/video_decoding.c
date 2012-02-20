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
#define pr_dbg(args...)
//#define pr_dbg(args...) fprintf(stdout, ##args)
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

/*
 * main routine of this program. takes varibale arg.
 * details can be seen in help
 */
int main(int argc, char **argv)
{
	int next_option;
	const char* const short_options = "hi:o:";
	char input[MAX_NAME_LEN];
	char output[MAX_NAME_LEN];
	int offset = 0, n_comp, i, j, x, y, width, height, frame, channel = 3;
	IplImage *img, *imgp, *imgd;
	const struct option long_options[] = {
		{ "help",     0, NULL, 'h' },
		{ "input",   1, NULL, 'i' },
		{ "output",   1, NULL, 'o' },
		{ NULL,       0, NULL, 0}
	};
	FILE *fp;
	uchar *ptr, *data;
	CvVideoWriter *writer = 0;
	long long size;
	CvRect rect;
	int count = 0;

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

	fp = fopen(input, "rb");
	if (!fp) {
		pr_err("Error in opening input file\n");
		return -1;
	}
	fseek(fp, 0, SEEK_END);
	size = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	data = (uchar *)malloc(size);
	fread(data, sizeof(uchar), size, fp);
	fclose(fp);
	offset = 0;
	/* extract header of compressed file */
	width = *((int *)&data[offset]);
	offset += 4;
	height = *((int *)&data[offset]);
	offset += 4;
	channel = *((int *)&data[offset]);
	offset += 4;
	frame = *((int *)&data[offset]);
	offset += 4;

	writer = cvCreateVideoWriter(output, CV_FOURCC('P', 'I', 'M', '1'), 30,
			cvSize(width, height), 1);
	img = cvCreateImage(cvSize(width, height), IPL_DEPTH_8U, channel);

	/* store first frame uncompressed */
	/* number of segment*/
	n_comp = *((int *)&data[offset]);
	pr_dbg("%d %d \n", n_comp, offset);
	offset += 4;
	/* segment detail */
	x = *((int *)&data[offset]);
	offset += 4;
	y = *((int *)&data[offset]);
	offset += 4;
	width = *((int *)&data[offset]);
	offset += 4;
	height = *((int *)&data[offset]);
	offset += 4;
	/*segment data */
	for (j = y; j < y + height; j++) {
		ptr = (uchar*) (img->imageData + j * img->widthStep);
		for (i = x; i < x + width; i++) {
			*(ptr + 3 * i) = data[offset];
			offset++;
			*(ptr + 3 * i + 1) = data[offset];
			offset++;
			*(ptr + 3 * i + 2) = data[offset];
			offset++;
		}
	}
	cvWriteFrame(writer, img);
	cvNamedWindow( "Motion", 1 );
	cvShowImage( "Motion", img );
			cvWaitKey(30);
	imgp = cvCreateImage(cvSize(width, height), 8, channel);
	imgd = cvCreateImage(cvSize(width, height), 8, channel);
	cvCopy(img, imgp);

	while (offset < size) {
		/* number of segment*/
		n_comp = *((int *)&data[offset]);
		pr_dbg("%d %d \n", n_comp, offset);
		offset += 4;
		while (n_comp--) {
			/* segment detail */
			rect.x = *((int *)&data[offset]);
			offset += 4;
			rect.y = *((int *)&data[offset]);
			offset += 4;
			rect.width = *((int *)&data[offset]);
			offset += 4;
			rect.height = *((int *)&data[offset]);
			offset += 4;
			cvSetImageROI(imgp, rect);
			cvZero(imgp);
			cvResetImageROI(imgp);
			cvZero(imgd);
			/*segment data */
			for (j = rect.y; j < rect.y + rect.height; j++) {
				ptr = (uchar*) (imgd->imageData + j * imgd->widthStep);
				for (i = rect.x; i < rect.x + rect.width; i++) {
					*(ptr + 3 * i) = data[offset];
					offset++;
					*(ptr + 3 * i + 1) = data[offset];
					offset++;
					*(ptr + 3 * i + 2) = data[offset];
					offset++;
				}
			}
			cvAdd(imgp, imgd, img);
			cvCopy(img, imgp);
		}
		cvWriteFrame(writer, img);
	if(count++ == 10)
		cvSaveImage("test.tif", img);
	cvShowImage( "Motion", img );
			if( cvWaitKey(30) >= 0 )
				break;
		cvCopy(img, imgp);
	}


	free(data);
	cvReleaseImage(&img);
	cvReleaseImage(&imgd);
	cvReleaseImage(&imgp);
	cvReleaseVideoWriter(&writer);
}
