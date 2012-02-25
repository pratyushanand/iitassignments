/*-----------All includes of file-----------------*/
#include <errno.h>
#include <cv.h>
#include <highgui.h>
#include <pthread.h>
#include <semaphore.h>
#include <string.h>
#include <getopt.h>
#include <fcntl.h>
#include <unistd.h>
#include "helper.h"
#include "lib.h"

/* Prints help for encoding program */
static void help_encoding (char *prog_name)
{
	FILE *stream = stdout;

	fprintf (stream, "Usage: %s options [ select... ]\n", prog_name);
	fprintf (stream,
		" -c	--camera	Input from a camera.\n"
		" -i	--input		Input from a video file.\n"
		"An image frame can either be taken from a \
		video camera or can be taken for a video file.\n");
	exit (0);
}

int main(int argc, char **argv)
{
	int next_option, ret;
	const char* const short_options = "hci:";
	const struct option long_options[] = {
		{ "help",     0, NULL, 'h' },
		{ "camera",   0, NULL, 'c' },
		{ "input",   1, NULL, 'i' },
		{ NULL,       0, NULL, 0}
	};
	struct image_info *info;
	pthread_t grabber_thread;
	pthread_t executer_thread;

	info = calloc(sizeof(*info), 1);
	if (!info) {
		pr_err("No dynamic memory available\n");
		return -ENOMEM;
	}
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
				info->capture = cvCaptureFromFile(optarg);
				if (!info->capture) {
					pr_err("Error with Video Imgae \
							Capture\n");
					return -1;
				};

				break;
			case 'c':
				info->capture = cvCreateCameraCapture(CV_CAP_V4L2);
				if (!info->capture) {
					pr_err("Could not open Camera to \
							capture image");
					return -1;
				};

				break;
			case -1:
				break;
			default:
				abort ();
		}
	} while (next_option != -1);

	if (!info->capture)
		return;
	/* Initilize synchronizer semaphores */
	sem_init (&info->frame_posted, 0, 0);
	sem_init (&info->frame_executed, 0, 1);
	/* Create Task for Image frame grabbing */
	pthread_create(&grabber_thread, NULL, image_grabber, (void*)info);
	/* Create Task for Image frame execution */
	pthread_create(&executer_thread, NULL, image_executer, (void*)info);
	/* wait for threads to complete */
	pthread_join(grabber_thread, NULL);
	pthread_join(executer_thread, NULL);
	cvReleaseCapture(&info->capture);
	free(info);
}
