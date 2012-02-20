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
#define pr_dbg(args...)
//#define pr_dbg(args...) fprintf(stdout, ##args)
#define pr_err(args...) fprintf(stdout, ##args)

/* Prints help for encoding program */
static void help_encoding (char *prog_name)
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
	CvCapture* capture = 0;
	IplImage *img, *imgp, *imgd, *imgt;
	const struct option long_options[] = {
		{ "help",     0, NULL, 'h' },
		{ "input",   1, NULL, 'i' },
		{ "output",   1, NULL, 'o' },
		{ NULL,       0, NULL, 0}
	};
	FILE *fp;
	CvMemStorage* storage = cvCreateMemStorage(0);
	CvSeq* comp = NULL;
	uchar *ptr;

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
			case -1:
				break;
			default:
				abort ();
		}
	} while (next_option != -1);

	capture = cvCaptureFromFile(input);

	if (!capture) {
		pr_err("Error with Video Imgae Capture\n");
		return -1;
	};

	width = cvGetCaptureProperty(capture, CV_CAP_PROP_FRAME_WIDTH);
	height = cvGetCaptureProperty(capture, CV_CAP_PROP_FRAME_HEIGHT);
	frame = cvGetCaptureProperty(capture, CV_CAP_PROP_FRAME_COUNT);
	pr_info("Original Raw size for each frame is %d x %d x %d\n", width, height, channel);

	img = cvQueryFrame(capture);
	fp = fopen(output, "wb");
	/* prepare header of compressed file */
	fwrite(&width, sizeof(int), 1, fp);
	offset += 4;
	fwrite(&height, sizeof(int), 1, fp);
	offset += 4;
	fwrite(&channel, sizeof(int), 1, fp);
	offset += 4;
	fwrite(&frame, sizeof(int), 1, fp);
	offset += 4;

	/* store first frame uncompressed */
	/* number of segment*/
	n_comp = 1;
	fwrite(&n_comp, sizeof(int), 1, fp);
	pr_dbg("%d %d \n", n_comp, offset);
	offset += 4;
	/* segment detail */
	x = 0;
	fwrite(&x, sizeof(int), 1, fp);
	offset += 4;
	y = 0;
	fwrite(&y, sizeof(int), 1, fp);
	offset += 4;
	fwrite(&width, sizeof(int), 1, fp);
	offset += 4;
	fwrite(&height, sizeof(int), 1, fp);
	offset += 4;
	/*segment data */
	for (j = y; j < y + height; j++) {
		ptr = (uchar*) (img->imageData + j * img->widthStep);
		fwrite(ptr + x * channel, sizeof(uchar), width * channel, fp);
		offset += width * channel;
	}
	imgp = cvCreateImage(cvSize(width, height), 8, channel);
	imgd = cvCreateImage(cvSize(width, height), 8, channel);
	imgt = cvCreateImage(cvSize(width, height), 8, channel);
	cvCopy(img, imgp);
	frame = 1;
	for(;;) {
		img = cvQueryFrame(capture);
		if (!img)
			break;
		frame++;
		cvAbsDiff(imgp, img, imgd);
		cvCopy(img, imgp);
		cvPyrSegmentation(imgd, imgt, storage, &comp, 4, 200, 10);
		/* number of segment*/
		n_comp = comp->total - 1;
		fwrite(&n_comp, sizeof(int), 1, fp);
		pr_dbg("%d %d \n", n_comp, offset);
		offset += 4;
		for (i = 1; i <= n_comp; i++) {
			CvConnectedComp* cc = (CvConnectedComp*) cvGetSeqElem(comp, i);
			/* segment detail */
			x = cc->rect.x;
			fwrite(&x, sizeof(int), 1, fp);
			offset += 4;
			y = cc->rect.y;
			fwrite(&y, sizeof(int), 1, fp);
			offset += 4;
			width = cc->rect.width;
			fwrite(&width, sizeof(int), 1, fp);
			offset += 4;
			height = cc->rect.height;
			fwrite(&height, sizeof(int), 1, fp);
			offset += 4;
			/*segment data */
			for (j = y; j < y + height; j++) {
				ptr = (uchar*) (img->imageData + j * img->widthStep);
				fwrite(ptr + x * channel, sizeof(uchar), width * channel, fp);
				offset += width * channel;
			}
		}
	}
	pr_info("Total number of frame is %d\n", frame);
	fclose(fp);
	cvReleaseImage(&imgt);
	cvReleaseImage(&imgd);
	cvReleaseImage(&imgp);
	cvReleaseCapture(&capture);
	cvReleaseMemStorage(&storage);
}
