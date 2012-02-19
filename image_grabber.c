#include <cv.h>
#include <highgui.h>
#include <pthread.h>
#include <semaphore.h>
#include "helper.h"
#include "lib.h"

void *image_grabber(void *data)
{
	struct image_grabber *grabber = data;

	for(;;) {
		sem_wait(&grabber->frame_executed);
		/*
		 * Also, need to maintain user specified delay
		 * between reception of two consecutive frmaes.
		 * Sstem might go into low power mode to save
		 * power.
		 * */
		grabber->img = cvQueryFrame(grabber->capture);
		if (!grabber->img) {
			pr_info("Frame is not received\n");
			return;
			/* TODO
			 * Ideally should not be a situation when code
			 * will reach here. Camera should always be able
			 * to receive frmae.But, if at all it reaches
			 * here handle the event coorectly.
			 */
		}
		sem_post(&grabber->frame_posted);
	}
}
