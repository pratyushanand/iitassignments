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
#define DEBUG_SKELTONIZE_IMAGE	0
//#define pr_debug(args...) fprintf(stdout, ##args)
#define pr_debug(args...)
#define pr_info(args...) fprintf(stdout, ##args)
#define pr_error(args...) fprintf(stdout, ##args)

#define CMD_XML_SIZE 1024
#define REPLY_XML_SIZE   40960
#define REPLY_SIZE   10000

typedef unsigned char u8;
typedef unsigned short u16;

#define CMD_TYPE_RESOLUTION 1
#define CMD_TYPE_START_CAM 2
#define CMD_TYPE_STOP_CAM 3
#define CMD_TYPE_DISCONNECT 4
#define CMD_TYPE_IMAGE_MODE_1 5
#define CMD_TYPE_IMAGE_MODE_2 6
#define CMD_TYPE_ACK 7

struct python_cmd {
	u8 data[CMD_XML_SIZE];
};

#define REPLY_TYPE_CAM_RESOLUTION 1
#define REPLY_TYPE_DATA 2
#define REPLY_TYPE_LAST_DATA	3

struct python_reply {
	/* Max pay load size is expected 2 KB */
	u8 data[REPLY_XML_SIZE];
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
	PyObject *extract_cmd_from_xml;
	PyObject *encode_reply_to_xml;
	struct python_cmd cmd;
	struct python_reply reply;
	CvCapture *stream;
	sem_t start_cam;
	sem_t data_ack;
};

static int image_mode = CMD_TYPE_IMAGE_MODE_1;

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


	/* extract_cmd_from_xml also a borrowed reference */
	param->extract_cmd_from_xml = PyDict_GetItemString(dict_obj, "extract_cmd_from_xml");
	if (!param->extract_cmd_from_xml) {
		pr_error("Could not load extract_cmd_from_xml function\n");
		release_python(param);
		return -1;
	} else if (!PyCallable_Check(param->extract_cmd_from_xml)) {
		pr_error("extract_cmd_from_xml function is not callable\n");
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

static unsigned int get_cmd(char *cmd) 
{
	pr_debug("Received cmd %s\n", cmd);
	if (!strcmp(cmd, "resolution"))
		return CMD_TYPE_RESOLUTION;
	else if (!strcmp(cmd, "start_cam"))
		return CMD_TYPE_START_CAM;
	else if (!strcmp(cmd, "stop_cam"))
		return CMD_TYPE_STOP_CAM;
	else if (!strcmp(cmd, "disconnect"))
		return CMD_TYPE_DISCONNECT;
	else if (!strcmp(cmd, "image_mode_1"))
		return CMD_TYPE_IMAGE_MODE_1;
	else if (!strcmp(cmd, "image_mode_2"))
		return CMD_TYPE_IMAGE_MODE_2;
	else if (!strcmp(cmd, "ack"))
		return CMD_TYPE_ACK;
	else
		return 0;
}

static PyObject* receive_cmd_from_server(struct configuration_params *param)
{
	PyObject *pcmd;
	int len;
	int i;

	memset(&param->cmd.data[0], '\0', CMD_XML_SIZE);
	pr_debug("waiting for command reception\n");
	if (len = recv(param->sockfd, param->cmd.data, CMD_XML_SIZE, 0) < 0)
		return NULL;
	pcmd = Py_BuildValue("(s#)", (char*)&param->cmd.data, strlen((char *)param->cmd.data));
	return PyObject_CallObject(param->extract_cmd_from_xml, pcmd);
}

static int send_reply_to_server(struct configuration_params *param)
{
	PyObject *preply, *pxml;
	char *xml;
	int reply_size = ((*(( unsigned int *)&param->reply.data[0])) >> 8) + 4;

	preply = Py_BuildValue("(s#)", (char*)&param->reply, reply_size);
	if (!preply) {
		pr_error("Error with py string encoding\n");
		return -1;
	}
	pxml = PyObject_CallObject(param->encode_reply_to_xml, preply);
	xml = PyString_AS_STRING(pxml);
	if (!pxml) {
		pr_error("Error with reply encoding\n");
		return -1;
	}
	if (write(param->sockfd, xml, strlen(xml)) < 0)
		return -1;

	return 0;
}

#if 0
static int skeltonize(IplImage *img)
{
	IplImage *skel, *eroded, *temp;
	IplConvKernel *element;
	int ret = 0;
	bool done;
	CvMemStorage* storage;
	CvSeq *line_seq;

	skel = cvCreateImage(cvSize(img->width, img->height), IPL_DEPTH_8U, 1);
	if (!skel) {
		pr_error("Could not allocate memory for skel image\n");
		return -1;
	}
	cvZero(skel);

	eroded = cvCreateImage(cvSize(img->width, img->height), IPL_DEPTH_8U, 1);
	if (!eroded) {
		pr_error("Could not allocate memory for eroded image\n");
		ret = -1;
		goto skel_free;
	}
	temp = cvCreateImage(cvSize(img->width, img->height), IPL_DEPTH_8U, 1);
	if (!skel) {
		pr_error("Could not allocate memory for temp image\n");
		ret = -1;
		goto skel_free;
	}

	element = cvCreateStructuringElementEx(3, 3, 1, 1, CV_SHAPE_CROSS, NULL);
	if (!element) {
		pr_error("Could not create structuring element\n");
		ret = -1;
		goto skel_free;
	}

	storage = cvCreateMemStorage(0);

	if (!storage) {
		pr_error("Could not create storage area\n");
		ret = -1;
		goto skel_free;
	}

	cvThreshold(img, img, 127, 255, CV_THRESH_BINARY);

	do
	{
		cvErode(img, eroded, element, 1);
		cvDilate(eroded, temp, element, 1);
		cvSub(img, temp, temp, NULL);
		cvOr(skel, temp, skel, NULL);
		cvCopy(eroded, img);

		done = (cvCountNonZero(img) == 0);
	} while (!done);

	cvSmooth(skel, skel, CV_GAUSSIAN, 5, 5 );

#if 1
	cvCopy (skel, img);
#else
	cvZero(img);
#if 0
	line_seq = cvHoughLines2( skel, storage, CV_HOUGH_STANDARD, 1, CV_PI/180, 500, 0, 0 );

		printf("\n");
	for(int i = 0; i < line_seq->total; i++ ) {
		float* line = (float*) cvGetSeqElem( line_seq , i );
	        float rho = line[0];
		float theta = line[1];
		CvPoint pt1, pt2;
		double a = cos(theta), b = sin(theta);
		double x0 = a*rho, y0 = b*rho;

//		printf("%f %f \n", rho, theta);
		pt1.x = cvRound(x0 + 1000*(-b));
		pt1.y = cvRound(y0 + 1000*(a));
		pt2.x = cvRound(x0 - 1000*(-b));
		pt2.y = cvRound(y0 - 1000*(a));

		cvLine(img, pt1, pt2, CV_RGB(255,255,255), 1, 8, 0 );
	}
#endif
	line_seq = cvHoughLines2(skel, storage, CV_HOUGH_PROBABILISTIC, 1, CV_PI/180, 100, 20, 1);
	for(int i = 0; i < line_seq->total; i++ ) {
		CvPoint* line = (CvPoint*)cvGetSeqElem(line_seq,i);
		cvLine(img, line[0], line[1], CV_RGB(255, 255, 255), 1, 8, 0 );
	}

	for(int i = 0; i < line_seq->total; i++ ) {
		float* line = (float*) cvGetSeqElem( line_seq , i );
	        float rho = line[0];
		float theta = line[1];
		CvPoint pt1, pt2;
		double a = cos(theta), b = sin(theta);
		double x0 = a*rho, y0 = b*rho;

//		printf("%f %f \n", rho, theta);
		pt1.x = cvRound(x0 + 1000*(-b));
		pt1.y = cvRound(y0 + 1000*(a));
		pt2.x = cvRound(x0 - 1000*(-b));
		pt2.y = cvRound(y0 - 1000*(a));

		cvLine(img, pt1, pt2, CV_RGB(255,255,255), 1, 8, 0 );
	}
#endif

skel_free:
	if (storage)
		cvReleaseMemStorage(&storage);
	if (element)
		cvReleaseStructuringElement(&element);
	if (eroded)
		cvReleaseImage(&eroded);
	if (temp)
		cvReleaseImage(&temp);
	if (skel)
		cvReleaseImage(&skel);
	return ret;
}
#endif

static void send_raw_image(IplImage *gray, struct configuration_params *param)
{
	unsigned int data_len = gray->height * gray->widthStep;
	unsigned int len;
	char *src = gray->imageData;
#if 0
		for (int j = 0; j < 288; j++)
			for (int i = 0; i < 384; i++)
				printf("%d %d %d\n", *(src + j * 384 + i), i, j);
#endif
	while (data_len > 0) {
		pr_debug("waiting for start_cam\n");
		sem_wait(&param->start_cam);
		pr_debug("Got start_cam\n");
		if (data_len > (REPLY_SIZE - 4))
			len = REPLY_SIZE - 4;
		else
			len = data_len;
		data_len -= len;
		if (data_len)
			*(( unsigned int *)&param->reply.data[0]) = (len << 8) | REPLY_TYPE_DATA;
		else
			*(( unsigned int *)&param->reply.data[0]) = (len << 8) | REPLY_TYPE_LAST_DATA;
		memcpy(&param->reply.data[4], src, len);
		src += len;
		send_reply_to_server(param);
		sem_post(&param->start_cam);
		pr_debug("Posted start_cam\n");
		pr_debug("waiting for data_ack\n");
		sem_wait(&param->data_ack);
		pr_debug("Got data_ack\n");
	}
}
static void* image_acquisition(void *data)
{
	struct configuration_params *param = (struct configuration_params *)data;
	CvMemStorage *storage;
	IplImage *gray, *temp, *map, *eroded, *dilated;
	CvSeq *contours = NULL;
	CvSeq *c = NULL;
	int width, height, stride, cx, cy, i, data_len, valid_contour;
	float area;
	vibeModel_t *model;
	PyObject *pargs;
	void *err = NULL;

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
#if DEBUG_SKELTONIZE_IMAGE
	cvNamedWindow("SKELTON", 1);
	cvMoveWindow("SKELTON", 4 * width, 0);
#endif

	storage = cvCreateMemStorage(0);
	if (!storage) {
		pr_error("Could not allocate storage memory\n");
		err = (void *)-1;
		goto exit;
	}

	/* Allocates memory to store the input images and the segmentation maps */
	gray = cvCreateImage(cvSize(width, height), IPL_DEPTH_8U, 1);
	if (!gray) {
		pr_error("Could not allocate memory for gray image\n");
		err = (void *)-1;
		goto exit;
	}

	map = cvCreateImage(cvSize(width, height), IPL_DEPTH_8U, 1);
	if (!map) {
		pr_error("Could not allocate memory for map image\n");
		err = (void *)-1;
		goto exit;
	}

	eroded = cvCreateImage(cvSize(width, height), IPL_DEPTH_8U, 1);
	if (!eroded) {
		pr_error("Could not allocate memory for eroded image\n");
		err = (void *)-1;
		goto exit;
	}

	dilated = cvCreateImage(cvSize(width, height), IPL_DEPTH_8U, 1);
	if (!dilated) {
		pr_error("Could not allocate memory for dilated image\n");
		err = (void *)-1;
		goto exit;
	}

	temp = cvCreateImage(cvSize(width, height), IPL_DEPTH_8U, 1);
	if (!temp) {
		pr_error("Could not allocate memory for temp image\n");
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
	acquire_grayscale_image(param->stream, gray);

	/* Allocates the model and initialize it with the first image */
	libvibeModelAllocInit_8u_C1R(model, (const uint8_t*)gray->imageData, width, height,
			stride);

	/* Processes all the following frames of your stream:
	   results are stored in "segmentation_map" */
	while(!acquire_grayscale_image(param->stream, gray)){
		send_raw_image(gray, param);
#if DEBUG_GRAY_IMAGE
		cvShowImage("GRAY", gray);
#endif
		cvWaitKey(3);
#if 0
		/* Get FG image in map */
		libvibeModelUpdate_8u_C1R(model, (const uint8_t*)gray->imageData,
				(uint8_t*)map->imageData);
#if DEBUG_FG_IMAGE
		cvShowImage("FG", map);
#endif
		/* Clean all small unnecessary FG objects. */
		cvErode(map, eroded, NULL, 1);
		/* Dilate it to get in proper shape */
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
		valid_contour = 0;
		for (c = contours; c != NULL; c = c->h_next) {
			if (cvContourArea(c, CV_WHOLE_SEQ, 0) > param->area)
				valid_contour++;
		}

		for (c = contours; c != NULL; c = c->h_next) {
			area = cvContourArea(c, CV_WHOLE_SEQ, 0);
			/*
			 * choose only if moving object area is greater
			 * than MIN_AREA square pixel
			 */
			if (area > param->area) {

				valid_contour--;
				cvWaitKey(10);
#if DEBUG_SEPARATED_IMAGE
				cvZero(temp);
				cvDrawContours(temp, c,
						cvScalar(255, 255, 255, 0),
						cvScalar(0, 0, 0, 0),
						-1, CV_FILLED, 8,
						cvPoint(0, 0));
				cvShowImage("SEPRATED_CONTOUR", temp);
#endif
				skeltonize(temp);
#if DEBUG_SKELTONIZE_IMAGE
				cvShowImage("SKELTON", temp);
#endif
				if (!valid_contour)
					param->reply.data[0] = REPLY_TYPE_LAST_CONTOUR;
				else
					param->reply.data[0] = REPLY_TYPE_CONTOUR;
				if (image_mode == CMD_TYPE_IMAGE_MODE_1) {
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
					param->reply.data[1] = cx;
					param->reply.data[2] = cx >> 8;
					param->reply.data[3] = cy;
					param->reply.data[4] = cy >> 8;
					param->reply.data[5] = '\0';
					data_len = 5;
				} else if (image_mode == CMD_TYPE_IMAGE_MODE_2) {
					for (i = 0; i < c->total; i++) {
						CvPoint* p = CV_GET_SEQ_ELEM(CvPoint, c,
								i);
						param->reply.data[1 + 4 * i] = p->x;
						param->reply.data[2 + 4 * i] = p->x >> 8;
						param->reply.data[3 + 4 * i] = p->y;
						param->reply.data[4 + 4 * i] = p->y >> 8;
					}
					data_len = 5 + 4 * i;
					param->reply.data[data_len] = '\0';
				}
				
				pr_debug("waiting for data_ack\n");
				sem_wait(&param->data_ack);
				pr_debug("Got data_ack\n");
				send_reply_to_server(param, data_len);
			} 
		}
#endif
	}

exit:
	if (model)
		libvibeModelFree(model);
	if (temp)
		cvReleaseImage(&temp);
	if (gray)
		cvReleaseImage(&gray);
	if (map)
		cvReleaseImage(&map);
	if (eroded)
		cvReleaseImage(&eroded);
	if (dilated)
		cvReleaseImage(&dilated);
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
#if DEBUG_SKELTONIZE_IMAGE
	cvDestroyWindow("SKELTON");
#endif
	return err;
}

static void* execute(void *data)
{
	struct configuration_params *param = (struct configuration_params *)data;
	PyObject *cmd;
	PyObject *pargs;
	u16 width, height;

	initialize_python(param);

	while (1) {
		cmd = receive_cmd_from_server(param);
		if (!cmd) {
			pr_error("Received wrong cmd\n");
			return (void *)-1;
		}
		switch (get_cmd(PyString_AS_STRING(cmd))) {
			case CMD_TYPE_RESOLUTION:
				*(( unsigned int *)&param->reply.data[0]) = (4 << 8) | REPLY_TYPE_CAM_RESOLUTION;
				width = get_image_width(param->stream);
				height = get_image_height(param->stream);
				param->reply.data[4] = width;
				param->reply.data[5] = width >> 8;
				param->reply.data[6] = height;
				param->reply.data[7] = height >> 8;
				send_reply_to_server(param);
				break;
			case CMD_TYPE_START_CAM:
				sem_post(&param->start_cam);
				pr_debug("Posted start_cam\n");
				break;
			case CMD_TYPE_STOP_CAM:
				pr_debug("waiting for start_cam\n");
				sem_wait(&param->start_cam);
				pr_debug("Got start_cam\n");
				break;
			case CMD_TYPE_DISCONNECT:
				return NULL;
			case CMD_TYPE_IMAGE_MODE_1:
				image_mode = CMD_TYPE_IMAGE_MODE_1;
				break;
			case CMD_TYPE_IMAGE_MODE_2:
				image_mode = CMD_TYPE_IMAGE_MODE_2;
				break;
			case CMD_TYPE_ACK:
				sem_post(&param->data_ack);
				pr_debug("Posted data_ack\n");
				break;
			default:
				pr_error("Could not understand cmd\n");
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

	sem_init (&cfg_param.start_cam, 0, 0);
	sem_init (&cfg_param.data_ack, 0, 0);
	pthread_create(&thread2, NULL, &execute, (void*)&cfg_param);
	pthread_create(&thread1, NULL, &image_acquisition, (void*)&cfg_param);
	pthread_join(thread1, NULL);
	pthread_join(thread2, NULL);
}
