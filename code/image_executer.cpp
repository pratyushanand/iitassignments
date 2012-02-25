#include <cv.h>
#include <errno.h>
#include <highgui.h>
#include <pthread.h>
#include <semaphore.h>
#include "helper.h"
#include "lib.h"

#define DEBUG 1

/*
 * get_segmented_seq
 */
static void get_segmented_seq(struct image_info *info)
{
	static int last = 0;
	int i, idx1 = last, idx2;
	static double timestamp = 0;
	IplImage *silh;

	cvCvtColor(info->img, info->frame_array[last], CV_BGR2GRAY);
	idx2 = (last + 1) % MAX_HISTORY_FRAMES;
	last = idx2;

	silh = info->frame_array[idx2];
	cvAbsDiff(info->frame_array[idx1], info->frame_array[idx2], silh);

	cvThreshold(silh, silh, 30, 1, CV_THRESH_BINARY);
	cvUpdateMotionHistory(silh, info->mhi, timestamp, MHI_DURATION);
	info->seq = cvSegmentMotion(info->mhi, info->segmask, info->storage,
			timestamp, MAX_TIME_DELTA);
	timestamp++;
}

/*
 * get_object_code
 */
int get_object_code(IplImage *img)
{
	/*
	 * TODO
	 */
}

/*
 * detect_object_from_seq
 */
static void detect_object_from_seq(struct image_info *info)
{
	CvSeq *seq = info->seq;
	CvRect comp_rect;
	int i, id;
	printf("%d\n", seq->total);

	for(i = 0; i < seq->total; i++) {
		comp_rect = ((CvConnectedComp*)cvGetSeqElem(seq, i))->rect;
//		printf("%d\t%d\t%d\t%d\n", comp_rect.x, comp_rect.y,
//				comp_rect.width, comp_rect.height);

		/* reject improbable components */
		if(comp_rect.width + comp_rect.height < 100)
			continue;

		/* select component ROI */
		cvSetImageROI(info->img, comp_rect);
		id = get_object_code(info->img);
		cvShowImage("TEST", info->img);
		/*
		 * TODO
		 * Send Object code, and coordinate over zigbee
		 */
		cvResetImageROI(info->img);
	}

}

/*
 *
 */

static int allocate_run_time_images(struct image_info *info)
{
	int width = cvGetCaptureProperty(info->capture,
			CV_CAP_PROP_FRAME_WIDTH);
	int height = cvGetCaptureProperty(info->capture,
			CV_CAP_PROP_FRAME_HEIGHT);
	int i, j, ret = 0;

	for (i = 0; i < MAX_HISTORY_FRAMES; i++) {
		info->frame_array[i] = cvCreateImage(cvSize(width, height),
			IPL_DEPTH_8U, 1);
		if (!info->frame_array[i]) {
			ret = -ENOMEM;
			goto err_frame_array;
		}
		cvZero(info->frame_array[i]);
	}

	info->mhi = cvCreateImage(cvSize(width, height),
			IPL_DEPTH_32F, 1);
	if (!info->mhi) {
		ret = -ENOMEM;
		goto err_frame_array;
	}
	cvZero(info->mhi);

	info->segmask = cvCreateImage(cvSize(width, height),
			IPL_DEPTH_32F, 1);
	if (!info->segmask) {
		ret = -ENOMEM;
		goto err_segmask;
	}

	info->storage = cvCreateMemStorage(0);
	if (!info->storage) {
		ret = -ENOMEM;
		goto err_storage;
	}
	return 0;

err_storage:
	cvClearMemStorage(info->storage);
err_segmask:
	cvReleaseImage(&info->mhi);
err_frame_array:
	for (j = 0; j < i; j++)
		cvReleaseImage(&info->frame_array[j]);
	pr_err("Error with Memory Allocation\n");
	return ret;
}

/*
 *
 */
static int free_run_time_images(struct image_info *info)
{
	int i;

	for (i = 0; i < MAX_HISTORY_FRAMES; i++)
		cvReleaseImage(&info->frame_array[i]);
	cvReleaseImage(&info->mhi);
	cvReleaseImage(&info->segmask);
	cvClearMemStorage(info->storage);
}

void *image_executer(void *data)
{
	struct image_info *info = (struct image_info *)data;
	IplImage *img;
	CvSeq* seq;

	if (allocate_run_time_images(info)) {
		/*
		 * TODO
		 */
	}
#ifdef DEBUG
	cvNamedWindow("TEST" , 0);
#endif
	for(;;) {
		sem_wait(&info->frame_posted);
		get_segmented_seq(info);
		detect_object_from_seq(info);
#ifdef DEBUG
		if(cvWaitKey(30) >= 0)
			break;
//		cvShowImage("TEST", info->mhi);
#endif
		sem_post(&info->frame_executed);
	}
#ifdef DEBUG
	cvDestroyWindow("TEST");
#endif
	free_run_time_images(info);
}
