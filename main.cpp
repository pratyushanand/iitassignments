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
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#define DEBUG_GRAY_IMAGE	1
#define DEBUG_FG_IMAGE		0
#define DEBUG_CLEANED_IMAGE	0
#define DEBUG_SEPARATED_IMAGE	0
#define DEBUG_OUTPUT_IMAGE	0
#define pr_debug(args...)
#define pr_info(args...) fprintf(stdout, ##args)
#define pr_error(args...) fprintf(stdout, ##args)

#define QUERY_XML_SIZE 256
struct python_query {
	char data[QUERY_XML_SIZE];
};

struct python_reply {
	/*
	 * 1 - resolution
	 * 2 - centroid of first moving object
	 * 3 - centroild of other moving object
	 */
	int type;
	int x;
	int y;
};

struct configuration_params {
	char video[100];
	int area;
	char host[50];
	int port;
	char xml[100];
	int sockfd;
	PyObject *py_name_obj;
	PyObject *py_mod_obj;
	PyObject *extract_query_from_xml;
	PyObject *encode_reply_to_xml;
	struct python_query query;
	struct python_reply reply;
	CvCapture *stream;
	sem_t start;
};

/* Prints help for this program */
static void help_main (char *prog_name)
{
	FILE *stream = stdout;

	fprintf (stream, "Usage: %s options [ first ... ]\n", prog_name);
	fprintf (stream,
			" -v	--video		Path of test video stream\n"
			" -a	--area		Min area of moving object in square pixel\n"
			" -n	--hostname 	hostname to connect\n"
			" -p	--port		port number to connect\n"
			" -x	--xml		Python xml routine\n");
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
	param->py_name_obj = PyString_FromString(param->xml);
	if (!param->py_name_obj) {
		pr_error("Could not build %s object\n", param->xml);
		release_python(param);
		return -1;
	}

	/* Load the module object */
	param->py_mod_obj = PyImport_Import(param->py_name_obj);
	if (!param->py_mod_obj) {
		pr_error("Could not load %s object\n", param->xml);
		release_python(param);
		return -1;
	}

	/* dict_obj is a borrowed reference */
	dict_obj = PyModule_GetDict(param->py_mod_obj);


	/* extract_query_from_xml also a borrowed reference */
	param->extract_query_from_xml = PyDict_GetItemString(dict_obj, "extract_query_from_xml");
	if (!param->extract_query_from_xml) {
		pr_error("Could not load extract_query_from_xml function\n");
		release_python(param);
		return -1;
	} else if (!PyCallable_Check(param->extract_query_from_xml)) {
		pr_error("extract_query_from_xml function is not callable\n");
		release_python(param);
		return -1;
	}

	/* encode_reply_to_xml also a borrowed reference */
	param->encode_reply_to_xml = PyDict_GetItemString(dict_obj, "encode_reply_to_xml");
	if (!param->encode_reply_to_xml) {
		pr_error("Could not load encode_reply_to_xml function\n");
		release_python(param);
		return -1;
	} else if (!PyCallable_Check(param->encode_reply_to_xml)) {
		pr_error("encode_reply_to_xml function is not callable\n");
		release_python(param);
		return -1;
	}

	return 0;
}

static unsigned int get_query(char *query) 
{
	pr_debug("Received Query %s\n", query);
	if (!strcmp(query, "resolution"))
		return 1;
	else if (!strcmp(query, "start_cam"))
		return 2;
	else if (!strcmp(query, "stop_cam"))
		return 3;
	else if (!strcmp(query, "disconnect"))
		return 4;
	else
		return 0;
}

static PyObject* receive_query_from_server(struct configuration_params *param)
{
	char buffer[QUERY_XML_SIZE];
	PyObject *pquery;

	memset(&param->query.data[0], 0, strlen(&param->query.data[0]));
	if (read(param->sockfd, &param->query.data[0], QUERY_XML_SIZE) < 0)
		return NULL;
	pquery = Py_BuildValue("(s#)", (char*)&param->query.data[0], strlen(&param->query.data[0]));
	return PyObject_CallObject(param->extract_query_from_xml, pquery);
}

static int send_reply_to_server(struct configuration_params *param)
{
	char buffer[QUERY_XML_SIZE];
	PyObject *preply, *pxml;
	char *xml;

	preply = Py_BuildValue("(s#)", (char*)&param->reply, sizeof(param->reply));
	pxml = PyObject_CallObject(param->encode_reply_to_xml, preply);
	xml = PyString_AS_STRING(pxml);
	if (write(param->sockfd, xml, strlen(xml)) < 0)
		return -1;
	return 0;
}

static void* image_acquisition(void *data)
{
	struct configuration_params *param = (struct configuration_params *)data;
	CvMemStorage *storage;
	IplImage *gray, *temp1 = NULL, *temp2 = NULL, *map, *eroded, *dilated;
	CvSeq *contours = NULL;
	CvSeq *c = NULL;
	int width, height, stride, cx, cy, i;
	float area;
	vibeModel_t *model;
	PyObject *pargs;
	void *err = NULL;
	bool first;

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
		err = (void *)-1;
		goto exit;
	}

	/* Allocates memory to store the input images and the segmentation maps */
	temp1 = cvCreateImage(cvSize(width, height), IPL_DEPTH_8U, 1);
	if (!temp1) {
		pr_error("Could not allocate memory for temp1 image\n");
		err = (void *)-1;
		goto exit;
	}
	temp2 = cvCreateImage(cvSize(width, height), IPL_DEPTH_8U, 1);
	if (!temp2) {
		pr_error("Could not allocate memory for temp2 image\n");
		err = (void *)-1;
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
		err = (void *)-1;
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
		sem_wait(&param->start);
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
		first = true;
		for (c = contours; c != NULL; c = c->h_next) {
			area = cvContourArea(c, CV_WHOLE_SEQ, 0);

			/*
			 * choose only if moving object area is greater
			 * than MIN_AREA square pixel
			 */
			if (area > param->area) {

				cvWaitKey(50);
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
				if (first) {
					param->reply.type =2;
					first = false;
				} else {
					param->reply.type =3;
				}
				param->reply.x =cx;
				param->reply.y =cy;
				send_reply_to_server(param);
			}
		}
#if DEBUG_OUTPUT_IMAGE
		cvShowImage("OUTPUT", temp2);
#endif
		gray = temp2;
		sem_post(&param->start);
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

static void* execute(void *data)
{
	struct configuration_params *param = (struct configuration_params *)data;
	PyObject *query;
	PyObject *pargs;

	initialize_python(param);

	while (1) {
		query = receive_query_from_server(param);
		if (!query) {
			pr_error("Received wrong query\n");
			return (void *)-1;
		}
		switch (get_query(PyString_AS_STRING(query))) {
			case 1:
				param->reply.type = 1;
				param->reply.x = get_image_width(param->stream);
				param->reply.y = get_image_height(param->stream);
				send_reply_to_server(param);
				break;
			case 2:
				sem_post(&param->start);
				break;
			case 3:
				sem_wait(&param->start);
				break;
			case 4:
				return NULL;
			default:
				pr_error("Could not understand query\n");
				break;
		}
	}

	cvReleaseCapture(&param->stream);
	release_python(param);
	return NULL;
}

int main(int argc, char **argv)
{
	int next_option;
	const char* const short_options = "hv:a:n:p:x:";
	const struct option long_options[] = {
		{ "help", 	0, 	NULL, 	'h' },
		{ "video", 	1, 	NULL, 	'v' },
		{ "area", 	1, 	NULL, 	'a' },
		{ "hostname", 	1, 	NULL, 	'n' },
		{ "port", 	1, 	NULL, 	'p' },
		{ "xml", 	1, 	NULL, 	'x' },
		{ NULL, 	0, 	NULL, 	0}
	};
	struct configuration_params cfg_param;
	pthread_t thread1, thread2;
	struct sockaddr_in serv_addr;
	struct hostent *server;

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
			case 'a':
				cfg_param.area = atoi(optarg);
				break;
			case 'n':
				strcpy(cfg_param.host, optarg);
				break;
			case 'p':
				cfg_param.port = atoi(optarg);
				break;
			case 'x':
				strcpy(cfg_param.xml, optarg);
				break;
			default:
				abort ();
		}
	} while (next_option != -1);

	cfg_param.sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (cfg_param.sockfd < 0) {
		pr_error("ERROR opening socket\n");
		return -1;
	}
	server = gethostbyname(cfg_param.host);
	if (server == NULL) {
		pr_error("ERROR, no such host\n");
		return -1;
	}
	bzero((char *) &serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
	serv_addr.sin_port = htons(cfg_param.port);
	if (connect(cfg_param.sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
		pr_error("Error in connection\n");
		return -1;
	}
	if (cfg_param.video)
		cfg_param.stream = cvCaptureFromFile(cfg_param.video);

	if (!cfg_param.stream) {
		pr_error("No video stream found\n");
		return -1;
	}

	sem_init (&cfg_param.start, 0, 0);
	pthread_create(&thread2, NULL, &execute, (void*)&cfg_param);
	pthread_create(&thread1, NULL, &image_acquisition, (void*)&cfg_param);
	pthread_join(thread1, NULL);
	pthread_join(thread2, NULL);
}
