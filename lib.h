#ifndef _LIB_H
#define _LIB_H
#include <pthread.h>
#include <semaphore.h>

/*
 * This struct have information about image grabber and its
 * synchronization with image analyser module
 */

struct image_grabber {
	CvCapture* capture;
	IplImage *img;
	sem_t frame_posted;
	sem_t frame_executed;
};

void *image_grabber(void *data);
#endif
