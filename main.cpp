#include <Python.h>
#include <getopt.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <signal.h>
#include <cv.h>
#include <cxtypes.h>
#include <highgui.h>
#include <cvaux.h>
#include "vibe-background.h"

#define MIN_AREA 1000
#define DEBUG_GRAY_IMAGE	1
#define DEBUG_FG_IMAGE		0
#define DEBUG_CLEANED_IMAGE	0
#define DEBUG_SEPARATED_IMAGE	0
#define DEBUG_OUTPUT_IMAGE	0
#define pr_info(args...) fprintf(stdout, ##args)
#define pr_error(args...) fprintf(stdout, ##args)

struct python_query {
	char type[20];
};

struct python_reply {
	/* 
	 * 1 - resolution
	 * 2 - centroid
	 * 3 - centroild of last moving object of same frame
	 */
	int type;
	int x;
	int y;
};

struct configuration_params {
	char video[100];
	char python[100];
	PyObject *py_name_obj;
	PyObject *py_mod_obj;
	PyObject *connect_to_server;
	PyObject *disconnect_from_server;
	PyObject *receive_query_from_server;
	PyObject *send_reply_to_server;
       	PyObject *xml_pack;
	struct python_query query;
	struct python_reply reply;
	CvCapture *stream;
};

/* Prints help for this program */
static void help_main (char *prog_name)
{
	FILE *stream = stdout;

	fprintf (stream, "Usage: %s options [ first ... ]\n", prog_name);
	fprintf (stream,
			" -v	--video		Path of test video stream\n"
			" -p	--python	Name of python client routine\n");
	exit (0);
}

static int32_t get_image_width(CvCapture *stream)
{
	return cvGetCaptureProperty(stream, CV_CAP_PROP_FRAME_WIDTH);
}

static int32_t get_image_height(CvCapture *stream)
{
	return cvGetCaptureProperty(stream, CV_CAP_PROP_FRAME_HEIGHT);
}

static int32_t get_image_stride(CvCapture *stream)
{
	/* we insure that it input image will always be gray scale */
	return cvGetCaptureProperty(stream, CV_CAP_PROP_FRAME_WIDTH);
}

static int32_t acquire_grayscale_image(CvCapture *stream, IplImage *gray)
{
	IplImage *img = cvQueryFrame(stream);

	if (!img)
		return -1;

	cvCvtColor(img, gray, CV_BGR2GRAY);
	return 0;
}

static int release_python (struct configuration_params *param)
{
	if (param->py_mod_obj)
		Py_DECREF(param->py_mod_obj);
	if (param->py_name_obj)
		Py_DECREF(param->py_name_obj);
	Py_Finalize();

	return 0;
}

static int initialize_python(struct configuration_params *param)
{
	PyObject *dict_obj;

	/* Initialize the Python Interpreter */
	Py_Initialize();

	/* Build the name object */
	param->py_name_obj = PyString_FromString(param->python);
	if (!param->py_name_obj) {
		pr_error("Could not build %s object\n", param->python);
		release_python(param);
		return -1;
	}

	/* Load the module object */
	param->py_mod_obj = PyImport_Import(param->py_name_obj);
	if (!param->py_mod_obj) {
		pr_error("Could not load %s object\n", param->python);
		release_python(param);
		return -1;
	}

	/* dict_obj is a borrowed reference */
	dict_obj = PyModule_GetDict(param->py_mod_obj);

	/* connect_to_server also a borrowed reference */
	param->connect_to_server = PyDict_GetItemString(dict_obj, "connect_to_server");
	if (!param->connect_to_server) {
		pr_error("Could not load connect_to_server function\n");
		release_python(param);
		return -1;
	} else if (!PyCallable_Check(param->connect_to_server)) {
		pr_error("connect_to_server function is not callable\n");
		release_python(param);
		return -1;
	}

	/* disconnect_from_server also a borrowed reference */
	param->disconnect_from_server = PyDict_GetItemString(dict_obj, "disconnect_from_server");
	if (!param->disconnect_from_server) {
		pr_error("Could not load disconnect_from_server function\n");
		release_python(param);
		return -1;
	} else if (!PyCallable_Check(param->disconnect_from_server)) {
		pr_error("disconnect_from_server function is not callable\n");
		release_python(param);
		return -1;
	}

	/* receive_query_from_server also a borrowed reference */
	param->receive_query_from_server = PyDict_GetItemString(dict_obj, "receive_query_from_server");
	if (!param->receive_query_from_server) {
		pr_error("Could not load receive_query_from_server function\n");
		release_python(param);
		return -1;
	} else if (!PyCallable_Check(param->receive_query_from_server)) {
		pr_error("receive_query_from_server function is not callable\n");
		release_python(param);
		return -1;
	}

	/* send_reply_to_server also a borrowed reference */
	param->send_reply_to_server = PyDict_GetItemString(dict_obj, "send_reply_to_server");
	if (!param->send_reply_to_server) {
		pr_error("Could not load send_reply_to_server function\n");
		release_python(param);
		return -1;
	} else if (!PyCallable_Check(param->send_reply_to_server)) {
		pr_error("send_reply_to_server function is not callable\n");
		release_python(param);
		return -1;
	}

	/* Connect to the sever */
	while (!PyObject_CallObject(param->connect_to_server, NULL))
		usleep(10000);
	return 0;
}

static unsigned int get_query(char *query) 
{
	pr_info("Received Query %s\n", query);
	if (!strcmp(query, "resolution"))
		return 1;
	else if (!strcmp(query, "start_capture"))
		return 2;
	else
		return 0;
}

static int start_capture(struct configuration_params *param)
{
	CvMemStorage *storage;
	IplImage *gray, *temp1 = NULL, *temp2 = NULL, *map, *eroded, *dilated;
	CvSeq *contours = NULL;
	CvSeq *c = NULL;
	int width, height, stride, cx, cy, i;
	float area;
	vibeModel_t *model;
	PyObject *pargs;
	int err = 0;

	width = get_image_width(param->stream);
	height = get_image_height(param->stream);
	stride = get_image_stride(param->stream);

#if DEBUG_GRAY_IMAGE
	cvNamedWindow("GRAY", 1);
#endif
#if DEBUG_FG_IMAGE
	cvNamedWindow("FG", 1);
	cvMoveWindow("FG", width, 0);
#endif
#if DEBUG_CLEANED_IMAGE
	cvNamedWindow("CLEANED", 1);
	cvMoveWindow("CLEANED", 2 * width, 0);
#endif
#if DEBUG_SEPARATED_IMAGE
	cvNamedWindow("SEPRATED_CONTOUR", 1);
	cvMoveWindow("SEPRATED_CONTOUR", 3 * width, 0);
#endif
#if DEBUG_OUTPUT_IMAGE
	cvNamedWindow("OUTPUT", 1);
	cvMoveWindow("OUTPUT", 4 * width, 0);
#endif

	storage = cvCreateMemStorage(0);
	if (!storage) {
		pr_error("Could not allocate storage memory\n");
		err = -1;
		goto exit;
	}

	/* Allocates memory to store the input images and the segmentation maps */
	temp1 = cvCreateImage(cvSize(width, height), IPL_DEPTH_8U, 1);
	if (!temp1) {
		pr_error("Could not allocate memory for temp1 image\n");
		err = -1;
		goto exit;
	}
	temp2 = cvCreateImage(cvSize(width, height), IPL_DEPTH_8U, 1);
	if (!temp2) {
		pr_error("Could not allocate memory for temp2 image\n");
		err = -1;
		goto exit;
	}
	/* Get a model data structure */
	/*
	 * Library for background detection from following paper.
	 * O. Barnich and M. Van Droogenbroeck.
	 * ViBe: A universal background subtraction algorithm for video sequences.
	 * In IEEE Transactions on Image Processing, 20(6):1709-1724, June 2011.
	*/
	model = libvibeModelNew();
	if (!model) {
		pr_error("Could not load vibe library\n");
		err = -1;
		goto exit;
	}

	/* Acquires your first image */
	gray = temp2;
	acquire_grayscale_image(param->stream, gray);

	/* Allocates the model and initialize it with the first image */
	libvibeModelAllocInit_8u_C1R(model, (const uint8_t*)gray->imageData, width, height,
			stride);

	/* Processes all the following frames of your stream:
		results are stored in "segmentation_map" */
	while(!acquire_grayscale_image(param->stream, gray)){
#if DEBUG_GRAY_IMAGE
		cvShowImage("GRAY", gray);
#endif
		/* Get FG image in temp1 */
		map = temp1;
		libvibeModelUpdate_8u_C1R(model, (const uint8_t*)gray->imageData,
				(uint8_t*)map->imageData);
#if DEBUG_FG_IMAGE
		cvShowImage("FG", map);
#endif
		/*
		 * Clean all small unnecessary FG objects. Get cleaned
		 * one in temp2
		 */
		eroded = temp2;
		cvErode(map, eroded, NULL, 2);
		/* Dilate it to get in proper shape */
		dilated = temp1;
		cvDilate(eroded, dilated, NULL, 1);
#if DEBUG_CLEANED_IMAGE
		cvShowImage("CLEANED", dilated);
#endif
		/*
		 * Find out all moving contours , so basically segment
		 * it out. Create separte image for each moving object.
		 */
		 cvFindContours(dilated, storage, &contours,
				sizeof(CvContour), CV_RETR_EXTERNAL,
				CV_CHAIN_APPROX_NONE, cvPoint(0, 0));
		cvZero(temp2);
		for (c = contours; c != NULL; c = c->h_next) {
			area = cvContourArea(c, CV_WHOLE_SEQ, 0);

			/*
			 * choose only if moving object area is greater
			 * than MIN_AREA square pixel
			 */
			if (area > MIN_AREA) {

				cvWaitKey(10);
#if DEBUG_SEPARATED_IMAGE
				cvZero(temp1);
				cvDrawContours(temp1, c,
						cvScalar(255, 255, 255, 0),
						cvScalar(0, 0, 0, 0),
						-1, CV_FILLED, 8,
						cvPoint(0, 0));
				cvShowImage("SEPRATED_CONTOUR", temp1);
#endif
				/* calculate centroid */
				cx = 0;
				cy = 0;
				for (i = 0; i < c->total; i++) {
					CvPoint* p = CV_GET_SEQ_ELEM(CvPoint, c,
							i);
					cx += p->x;
					cy += p->y;
				}
				cx /= c->total;
				cy /= c->total;
				param->reply.type =2;
				param->reply.x =cx;
				param->reply.y =cy;
				pargs = Py_BuildValue("(s#)", (char*)&param->reply,
						sizeof(param->reply));
				PyObject_CallObject(param->send_reply_to_server, pargs);
			}
		}
#if DEBUG_OUTPUT_IMAGE
		cvShowImage("OUTPUT", temp2);
#endif
		gray = temp2;
	}

exit:
	if (model)
		libvibeModelFree(model);
	if (temp1)
		cvReleaseImage(&temp1);
	if (temp2)
		cvReleaseImage(&temp2);
	if (storage)
		cvReleaseMemStorage(&storage);
#if DEBUG_GRAY_IMAGE
	cvDestroyWindow("GRAY");
#endif
#if DEBUG_FG_IMAGE
	cvDestroyWindow("FG");
#endif
#if DEBUG_CLEANED_IMAGE
	cvDestroyWindow("CLEANED");
#endif
#if DEBUG_SEPARATED_IMAGE
	cvDestroyWindow("SEPRATED_CONTOUR");
#endif
#if DEBUG_OUTPUT_IMAGE
	cvDestroyWindow("OUTPUT");
#endif
	return err;
}

static int execute(struct configuration_params *param)
{
	PyObject *query, *pargs;

	initialize_python(param);

	if (param->video)
		param->stream = cvCaptureFromFile(param->video);

	if (!param->stream) {
		pr_error("No video stream found\n");
		return -1;
	}


	while (1) {
		query = PyObject_CallObject(param->receive_query_from_server, NULL);
		if (!query) {
			pr_error("Could not Get any query\n");
			break;
		}

		switch (get_query(PyString_AS_STRING(query))) {
			case 1:
				param->reply.type = 1;
				param->reply.x = get_image_width(param->stream);
				param->reply.y = get_image_height(param->stream);
				pargs = Py_BuildValue("(s#)", (char*)&param->reply,
						sizeof(param->reply));
				PyObject_CallObject(param->send_reply_to_server, pargs);
				break;
			case 2:
				start_capture(param);
				break;
			default:
				pr_error("Could not understand query\n");
				break;
		}
	}

	PyObject_CallObject(param->disconnect_from_server, NULL);
	cvReleaseCapture(&param->stream);
	release_python(param);
}

int main(int argc, char **argv)
{
	int next_option;
	const char* const short_options = "hv:p:";
	const struct option long_options[] = {
		{ "help", 	0, 	NULL, 	'h' },
		{ "video", 	1, 	NULL, 	'v' },
		{ "python", 	1, 	NULL, 	'p' },
		{ NULL, 	0, 	NULL, 	0}
	};
	struct configuration_params cfg_param;

		/* get all user input in local variables */
	do {
		next_option = getopt_long (argc, argv, short_options,
				long_options, NULL);
		switch (next_option)
		{
			case 'h':
				help_main(argv[0]);
				return 0;
			case -1:
				break;
			case 'v':
				strcpy(cfg_param.video, optarg);
				break;
			case 'p':
				strcpy(cfg_param.python, optarg);
				break;
			default:
				abort ();
		}
	} while (next_option != -1);

	execute(&cfg_param);
}
