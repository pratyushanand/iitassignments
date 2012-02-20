#define _GNU_SOURCE
#include <cv.h>
#include <getopt.h>
#include <highgui.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#define MAX_NAME_LEN	50
#define MAX_CPU_NUM	8
#define pr_info(args...) fprintf(stdout, ##args)
#define pr_err(args...) fprintf(stdout, ##args)


static void help_detect (char *prog_name)
{
	pr_info ("Usage: %s options [ inputfile ... ]\n", prog_name);
	pr_info (" -h	--help		Display this usage information.\n"
		" -i	--input		Input Image Name.\n"
		" -o	--output	Output Image Name.\n"
		" -t	--thd 		to threshold soble differential.\n"
		" -n	--num 		number of processes.\n"
		" -c	--cpu 		number of CPUs.\n");
	return;
}


/*******************************************************************************/
/* Part 2.3.1: Edge detection, Self Implemented using Sobel operator */
/*******************************************************************************/
static int dx[3][3] = { { -1, -2, -1},
	  		{  0, 0,  0},
	  		{  1, 2,  1}};

static int dy[3][3] = { { -1, 0,  1},
	  		{ -2, 0,  2},
	  		{ -1, 0,  1}};

struct shared_data {
	IplImage *img1;
	IplImage *img2;
	IplImage *img3;
	int th;
	sem_t start;
	sem_t end;
};
/* Convolution with Soble operator */
static int convolution(uchar *ptr1, uchar *ptr2, uchar *ptr3)
{
	int xconv = 0, yconv = 0, conv, i;

	for (i = 0; i < 3; i++)
		xconv += dx[i][0] * (*(ptr1 + i));
	for (i = 0; i < 3; i++)
		xconv += dx[i][1] * (*(ptr2 + i));
	for (i = 0; i < 3; i++)
		xconv += dx[i][2] * (*(ptr3 + i));

	for (i = 0; i < 3; i++)
		yconv += dy[i][0] * (*(ptr1 + i));
	for (i = 0; i < 3; i++)
		yconv += dy[i][1] * (*(ptr2 + i));
	for (i = 0; i < 3; i++)
		yconv += dy[i][2] * (*(ptr3 + i));

	conv = sqrt(xconv * xconv + yconv * yconv);

	return conv;
}

void* detection(void *data)
{
	int x, y, conv;
	uchar *sptr, *pptr, *dptr;
	struct shared_data *shrd = data;
	IplImage *img1 = shrd->img1;
	IplImage *img2 = shrd->img2;
	IplImage *img3 = shrd->img3;
	int th = shrd->th;

	while (1) {
		sem_wait(&shrd->start);

		for (y = 1; y < img1->height - 1; y++) {
			sptr= (uchar*) (img1->imageData + y * img1->widthStep);
			dptr= (uchar*) (img2->imageData + y * img2->widthStep);
			for (x = 1; x < img1->width - 1; x++) {
				pptr = sptr + x - 1;
				conv = convolution(pptr, pptr + img1->widthStep,
						pptr + 2 * img1->widthStep);
				/* threshold on the basis of covolution value */
				if (conv > th) {
					*(dptr + x) = 0;
				}
				else
					*(dptr + x) = 255;
			}
		}
		cvCvtColor(img2, img3, CV_GRAY2RGB);
		sem_post(&shrd->end);
	}
	return NULL;
}
static int user_sig_rcvd = 0;
void sig_handler (int signal_number)
{
	user_sig_rcvd = 1;
}
/******************************************************************************/
/* Main function */
/******************************************************************************/
int main(int argc, char** argv)
{

	int next_option, err = 0;
	const char* const short_options = "hi:o:t:n:c:";
	char input[MAX_NAME_LEN];
	char output[MAX_NAME_LEN];
	IplImage *img;
	int thd, num, i, j, cpu;
	CvCapture* capture = 0;
	CvVideoWriter *writer = 0;
	struct timeval tvi, tvf;
	cpu_set_t cpuset[MAX_CPU_NUM];
	pthread_t thread[MAX_CPU_NUM];
	struct shared_data data[MAX_CPU_NUM];
	int height, width;
	const struct option long_options[] = {
		{ "help",     0, NULL, 'h' },
		{ "input",   1, NULL, 'i' },
		{ "output",   1, NULL, 'o' },
		{ "thd",   1, NULL, 't' },
		{ "num",   1, NULL, 'n' },
		{ "cpu",   1, NULL, 'c' },
		{ NULL,       0, NULL, 0}
	};
	long long timei, timef;
	struct sigaction sa;
	pthread_t threads = pthread_self();
	cpu_set_t cset;
	CPU_ZERO(&cset);
	CPU_SET(0, &cset);
	pthread_setaffinity_np(threads, sizeof(cpu_set_t), &cset);

	/* get all user input in local variables */
	do {
		next_option = getopt_long (argc, argv, short_options,
				long_options, NULL);
		switch (next_option) {
			case 'h':
				help_detect(argv[0]);
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
				thd = atoi(optarg);
				break;
			case 'n':
				num = atoi(optarg);
				break;
			case 'c':
				cpu = atoi(optarg);
				break;
			case -1:
				break;
			default:
				pr_err("Wrong options\n");
				abort ();
		}
	} while (next_option != -1);

	memset (&sa, 0, sizeof (sa));
	sa.sa_handler = &sig_handler;
	sigaction (SIGINT, &sa, NULL);

	capture = cvCaptureFromFile(input);

	if (!capture) {
		pr_err("Error with Video Imgae Capture\n");
		return -1;
	};

	width = cvGetCaptureProperty(capture, CV_CAP_PROP_FRAME_WIDTH);
	height = cvGetCaptureProperty(capture, CV_CAP_PROP_FRAME_HEIGHT);

	for (i = 0; i < num; i++) {
		CPU_ZERO(&cpuset[i]);
		CPU_SET((i + 1) % cpu, &cpuset[i]);
	}
	for (i = 0; i < num; i++) {
		data[i].img1 = cvCreateImage(cvSize(width, height),
				IPL_DEPTH_8U, 1);
		cvZero(data[i].img1);
		data[i].img2 = cvCreateImage(cvSize(width, height),
				IPL_DEPTH_8U, 1);
		cvZero(data[i].img2);
		data[i].img3 = cvCreateImage(cvSize(width, height),
				IPL_DEPTH_8U, 3);
		cvZero(data[i].img3);
		data[i].th = thd;
		sem_init (&data[i].start, 0, 0);
		sem_init (&data[i].end, 0, 0);
		pthread_create(&thread[i], NULL, &detection, (void*)&data[i]);
		pthread_setaffinity_np(thread[i], sizeof(cpu_set_t),
				&cpuset[i]);
	}
	writer = cvCreateVideoWriter(output, CV_FOURCC('P', 'I', 'M', '1'), 30,
			cvSize(width, height), 1);
	while (!user_sig_rcvd);
	gettimeofday(&tvi, NULL);
	for(;;) {
		for (i = 0; i < num; i++) {
			img = cvQueryFrame(capture);
			if(!img)
				break;
			if (img->nChannels == 3)
				cvCvtColor(img, data[i].img1, CV_RGB2GRAY);
			else
				cvCopyImage(img, data[i].img1);

			sem_post(&data[i].start);
		}

		for (j = 0; j < i; j++) {
			sem_wait(&data[j].end);
			cvWriteFrame(writer, data[j].img3);
		}
		if (i < num)
			break;
	}
	gettimeofday(&tvf, NULL);
	printf("Time taken in execution is %ld uS\n",
			(tvf.tv_sec - tvi.tv_sec) * 1000000 +
			(tvf.tv_usec - tvi.tv_usec));

	for (i = 0; i < num; i++) {
		cvReleaseImage(&data[i].img1);
		cvReleaseImage(&data[i].img2);
		cvReleaseImage(&data[i].img3);
	}
	cvReleaseVideoWriter(&writer);
	cvReleaseCapture(&capture);

	return err;
}
