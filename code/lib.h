#ifndef _LIB_H
#define _LIB_H
#include <pthread.h>
#include <semaphore.h>

/* various tracking parameters (in frames) */
/* No of frmaes for which motion history will be stored. */
#define	MHI_DURATION		30
/* Maximum number of frames to be accepted as attached motion */
#define MAX_TIME_DELTA 		1
/* Maximum number of frmaes to be used for backgound estimation */
#define MAX_HISTORY_FRAMES 4
/*
 * This struct have information about image grabber and its
 * synchronization with image analyser module
 * Additionally, it also keeps pointer for all the useful runtime
 * iamges.
 */

struct image_info {
	CvCapture* capture; /* Camera / Video Capture */
	CvMemStorage *storage; /* Memory Storage for segmentation */
	CvSeq *seq; /* Array of motion segmented sequence */
	IplImage *img; /* Current Frame */
	IplImage *frame_array[MAX_HISTORY_FRAMES];
	/* Pointer for past frames for background estimation */
	IplImage *mhi; /* Motion history Image */
	IplImage *segmask; /* Mask for segmentation */
	sem_t frame_posted;
	sem_t frame_executed;
};

void *image_grabber(void *data);
void *image_executer(void *data);
#endif
