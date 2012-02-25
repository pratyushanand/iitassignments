#include <cv.h>
#include <highgui.h>
#include <pthread.h>
#include <semaphore.h>
#include "helper.h"
#include "lib.h"

void *image_grabber(void *data)
{
	struct image_info *info = (struct image_info *)data;

	for(;;) {
		sem_wait(&info->frame_executed);
		/*
		 * Also, need to maintain user specified delay
		 * between reception of two consecutive frmaes.
		 * Sstem might go into low power mode to save
		 * power.
		 * */
		info->img = cvQueryFrame(info->capture);
		if (!info->img) {
			pr_info("Frame is not received\n");
			return NULL;
			/* TODO
			 * Ideally should not be a situation when code
			 * will reach here. Camera should always be able
			 * to receive frmae.But, if at all it reaches
			 * here handle the event coorectly.
			 */
		}
		sem_post(&info->frame_posted);
	}
}
