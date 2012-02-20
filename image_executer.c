#include <highgui.h>
#include <pthread.h>
#include <semaphore.h>
#include "helper.h"
#include "lib.h"

void *image_executer(void *data)
{
	struct image_grabber *grabber = data;

	cvNamedWindow("TEST" , 0);
	for(;;) {
		sem_wait(&grabber->frame_posted);
		cvShowImage("TEST", grabber->img);
		cvWaitKey(30);
		sem_post(&grabber->frame_executed);
	}
	cvDestroyWindow("TEST");
}
