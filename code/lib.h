#ifndef _LIB_H
#define _LIB_H
#include <pthread.h>
#include <semaphore.h>

#define MAX_HISTORY_FRAMES 4
/*
 * This struct have information about image grabber and its
 * synchronization with image analyser module
 */

struct image_info {
	CvCapture* capture;
	CvMemStorage *storage;
	CvSeq *seq;
	IplImage *img;
	IplImage *frame_array[MAX_HISTORY_FRAMES];
	IplImage *mhi;
	IplImage *segmask;
	sem_t frame_posted;
	sem_t frame_executed;
};

void *image_grabber(void *data);
void *image_executer(void *data);
#endif
