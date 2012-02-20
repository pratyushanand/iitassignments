/*this program does mosaicing of multiple images.
  name of images have been provided in a string.
  name of last image must be NULL
  name of final image would be intermediate.jpg
 */

/*-----------All includes of file-----------------*/
#include <cv.h>
#include <highgui.h>
#include "stdio.h"
#include "string.h"
#include "getopt.h"
#include "fcntl.h"
#include "unistd.h"

/*-----------All defines of file-----------------*/
/* all defines for this program*/
#define MAX_CTRL_POINT	20 
#define MAX_TRIANGLE_COUNT	42 
#define EASY_SELECT_FACTOR	4

/*-----------All static variables of file-----------------*/
/* all static/global variables for this program*/
static CvPoint2D32f ctrl_1[MAX_CTRL_POINT];
static CvPoint2D32f ctrl_2[MAX_CTRL_POINT];
static int ctrl_1count = 0;
static int ctrl_2count = 0;
static int ctrl_point_num = 4;

/*-----------All function of file-----------------*/
/* Prints help for morphing program */
static void help_mosaic (char *prog_name)
{
	FILE *stream = stdout;

	fprintf (stream, "Usage: %s options [  select... ]\n", prog_name);
	fprintf (stream,
			" -s	--select	slect control points from mouse\n"
			" -c	--control	no of control points.(Max 20)\n");
	exit (0);
}


/*mouse callback function for selecting points on first image*/

static void select_ctrl_point1( int event, int x, int y, int flags, void* param )
{
	if( event == CV_EVENT_LBUTTONDOWN){

		/*first 4 control points are corners*/
		if (ctrl_1count  < ctrl_point_num) {
			ctrl_1[ctrl_1count].x = x * EASY_SELECT_FACTOR;
			ctrl_1[ctrl_1count++].y = y * EASY_SELECT_FACTOR;
		}
	}
}

/*mouse callback function for selecting points on second image*/

static void select_ctrl_point2( int event, int x, int y, int flags, void* param )
{
	if( event == CV_EVENT_LBUTTONDOWN){

		/*first 4 control points are corners*/
		if (ctrl_2count  < ctrl_point_num){
			ctrl_2[ctrl_2count].x = x * EASY_SELECT_FACTOR;
			ctrl_2[ctrl_2count++].y = y * EASY_SELECT_FACTOR;
		}
	}
}
/*main routine of this program. takes varibale arg. details can be seen in help*/
int main(int argc, char **argv)
{
	IplImage	*img, *img1, *img2;
	char images[][50] = { "m0607_0.jpg" ,"m0607_5.jpg", "m0607_1.jpg", "m0607_4.jpg", "m0607_2.jpg", "m0607_3.jpg", "NULL"};
	int i, j, k, px;
	CvMat*        homography;
	int fd;
	int next_option;
	const char* const short_options = "hs:c:";
	int select_ctrl_pt = 0;
	int height, width;
	const struct option long_options[] = {
		{ "help",     0, NULL, 'h' },
		{ "select",   1, NULL, 's' },
		{ "control",   1, NULL, 'c' },
		{ NULL,       0, NULL, 0   }
	};
	int image_number = 1;

	/*save input arguments*/

	do {
		next_option = getopt_long (argc, argv, short_options,
				long_options, NULL);
		switch (next_option)
		{
			case 'h':
				help_mosaic(argv[0]);
				return 0;
			case 's':
				select_ctrl_pt = atoi(optarg);
				break;
			case 'c':
				ctrl_point_num = atoi(optarg);
				break;
			case -1:
				break;
			default:
				abort ();
		}
	} while (next_option != -1);

	/*get width and height of original image*/

	img1= cvLoadImage( images[0], 1);
	height = img1->height;
	width = img1->width;
	cvReleaseImage(&img1);

	fd = open ("ctrl_pt.txt", O_RDWR);
	if(fd == -1) {
		printf("error with file open \n");
		return 0;
	}


	while(strcmp(images[image_number++],"NULL")) {

		/*if user specifes to select control point from mouse*/
		if (select_ctrl_pt) {
			/*create window for image display*/
			cvNamedWindow( "IMAGE1", 1 );
			cvNamedWindow( "IMAGE2", 1 );
			/*slect control points from mouse*/
			cvSetMouseCallback( "IMAGE1", select_ctrl_point1, 0 );
			cvSetMouseCallback( "IMAGE2", select_ctrl_point2, 0 );
			printf("select %d control point(1-2-1-2...) with mouse left click\n", ctrl_point_num);


			/*load images*/
			/*first image would be original only first time. After the successive iterations 
			  intermediate image of previous stage will be taken as first image*/
			if (image_number == 2)
				img = cvLoadImage( images[image_number - 2], 1);
			else 
				img = cvLoadImage( "intermediate.jpg", 1);
			img1 = cvCreateImage(cvSize(width / EASY_SELECT_FACTOR, height / EASY_SELECT_FACTOR ), 8, 3 );
			cvResize( img, img1, CV_INTER_CUBIC );
			cvReleaseImage(&img);
			img = cvLoadImage( images[image_number - 1], 1);

			/*always equate size of images*/
			if ((width != img->width) || (height != img->height)) {
				img2= cvCreateImage(cvSize(width , height ), 8, 3 );

				for(i = 0; i < img->height;i++) {
					for(k = 0;k < img->width;k++) {
						uchar *ptr2, *ptr;
						ptr2= (uchar*) (img2->imageData + i * img2->widthStep);
						ptr = (uchar*) (img->imageData + i * img->widthStep);
						ptr2[3*k] = ptr[3*k];
						ptr2[3*k + 1] = ptr[3*k + 1];
						ptr2[3*k + 2] = ptr[3*k + 2];
					}
				}
				cvReleaseImage(&img);
				img = cvCloneImage(img2);
				cvReleaseImage(&img2);
			}
			img2= cvCreateImage(cvSize(width / EASY_SELECT_FACTOR, height / EASY_SELECT_FACTOR), 8, 3 );
			cvResize( img, img2, CV_INTER_CUBIC );
			cvReleaseImage(&img);

			cvShowImage( "IMAGE1", img1 );
			cvShowImage( "IMAGE2", img2 );
			/*select control points*/
			for (i = 0;i < ctrl_point_num;i++) {
				/* wait till user selects point*/
				while (i >= ctrl_1count)
					cvWaitKey(300);
				for (j = -1;j < i;j++) {
					CvPoint	tmp;
					CvScalar s = cvScalar(0+rand()%255, 0+rand()%255, 0+rand()%255, 0);			
					tmp.x = ctrl_1[j+1].x / EASY_SELECT_FACTOR;
					tmp.y = ctrl_1[j+1].y / EASY_SELECT_FACTOR;
					cvCircle(img1, tmp, 2, s, 2, 8, 0);
				}
				cvShowImage( "IMAGE1", img1 );
				/* wait till user selects point*/
				while (i >= ctrl_2count)
					cvWaitKey(300);
				for (j = -1;j < i;j++) {
					CvPoint	tmp;
					CvScalar s = cvScalar(0+rand()%255, 0+rand()%255, 0+rand()%255, 0);			
					tmp.x = ctrl_2[j+1].x / EASY_SELECT_FACTOR;
					tmp.y = ctrl_2[j+1].y / EASY_SELECT_FACTOR;
					cvCircle(img2, tmp, 2, s, 2, 8, 0);
				}
				cvShowImage( "IMAGE2", img2 );
			}
			px = ctrl_point_num;
			write(fd, &px, sizeof(int));
			/*save points to file for next time use*/
			for (i = 0; i < ctrl_point_num; i++) {
				px = ctrl_1[i].x;
				write(fd, &px, sizeof(int));
				px = ctrl_1[i].y;
				write(fd, &px, sizeof(int));
			}
			for (i = 0; i < ctrl_point_num; i++) {
				px = ctrl_2[i].x;
				write(fd, &px, sizeof(int));
				px = ctrl_2[i].y;
				write(fd, &px, sizeof(int));
			}
			cvReleaseImage(&img1);
			cvReleaseImage(&img2);
			cvDestroyWindow("IMAGE1");
			cvDestroyWindow("IMAGE2");
			ctrl_1count = 0;
			ctrl_2count = 0;
		} else {
			/*if user dos not specifes to select control point from mouse,
			  then read it from file*/
			read(fd, &px, sizeof(int));

			for (i = 0; i < ctrl_point_num; i++) {
				read(fd, &px, sizeof(int));
				ctrl_1[i].x = px;
				read(fd, &px, sizeof(int));
				ctrl_1[i].y = px;
			}
			for (i = 0; i < ctrl_point_num; i++) {
				read(fd, &px, sizeof(int));
				ctrl_2[i].x = px;
				read(fd, &px, sizeof(int));
				ctrl_2[i].y = px;
			}


		}

		/*space for homography transformation matrix*/
		homography = cvCreateMat(3,3,CV_32FC1);
		/*get homography matrix between two images*/
		cvGetPerspectiveTransform(ctrl_2, ctrl_1, homography);


		/*load image again */
		if (image_number == 2)
			img1 = cvLoadImage( images[image_number - 2], 1);
		else
			img1 = cvLoadImage( "intermediate.jpg", 1);
		img2 = cvLoadImage( images[image_number - 1], 1);
		/*always equate size of images*/
		if ((width != img2->width) || (height != img2->height)) {
			img = cvCreateImage(cvSize(width , height ), 8, 3 );
			for(i = 0; i < img2->height;i++) {
				for(k = 0;k < img2->width;k++) {
					uchar *ptr2, *ptr;
					ptr2= (uchar*) (img2->imageData + i * img2->widthStep);
					ptr = (uchar*) (img->imageData + i * img->widthStep);
					ptr[3*k] = ptr2[3*k];
					ptr[3*k + 1] = ptr2[3*k + 1];
					ptr[3*k + 2] = ptr2[3*k + 2];
				}
			}
			cvReleaseImage(&img2);
			img2 = cvCloneImage(img);
			cvReleaseImage(&img);
		}
		/*do perspective transformation from image2 to image 1*/
		img = cvCreateImage(cvSize(width * 2, height *2), 8, 3 );
		cvWarpPerspective( img2, img, homography );

		/*now stich new image with image1 */
		/*take average for common pixels*/
		for(i = 0; i < height;i++) {
			for(k = 0;k < width;k++) {
				uchar *ptr1, *ptr;
				uchar r1, r, g1, g, b1, b;
				ptr1= (uchar*) (img1->imageData + i * img1->widthStep);
				ptr = (uchar*) (img->imageData + i * img->widthStep);
				r1 = ptr1[3*k];
				r = ptr[3*k];
				g1 = ptr1[3*k + 1];
				g = ptr[3*k + 1];
				b1 = ptr1[3*k + 2];
				b = ptr[3*k + 2];
				if (!((r1 == 0) && (b1 == 0) && (g1 == 0))
						&& ((r == 0) && (b == 0) && (g == 0))) {
					ptr[3*k] = r1;
					ptr[3*k + 1] = g1;
					ptr[3*k + 2] = b1;
				} else if (!((r1 == 0) && (b1 == 0) && (g1 == 0))
						&& !((r == 0) && (b == 0) && (g == 0))) {
					ptr[3*k] = (r1 + r)/2;
					ptr[3*k + 1] = (g1 + g)/2;
					ptr[3*k + 2] = (b1 + b)/2;
				}
			}

		}

		/*create window for image display*/
		cvNamedWindow( "SELECT_ROI", 1 );
		cvSetMouseCallback( "SELECT_ROI", select_ctrl_point1, 0 );
		cvReleaseImage(&img1);
		img1= cvCreateImage(cvSize(width * 2 / EASY_SELECT_FACTOR,
					height * 2 / EASY_SELECT_FACTOR), 8, 3 );
		cvResize( img, img1, CV_INTER_CUBIC );
		cvShowImage( "SELECT_ROI", img1 );
		printf("select region of interest (four corners):\n");
		printf("one corner is (0,0) (select three others):\n");
		/*region of intereset for next iterations*/
		for (i = 0;i < 3;i++) {
			CvPoint	tmp;
			/* wait till user selects point*/
			while (i >= ctrl_1count)
				cvWaitKey(300);
			for (j = -1;j < i;j++) {
				CvScalar s = cvScalar(0+rand()%255, 0+rand()%255, 0+rand()%255, 0);			
				tmp.x = ctrl_1[j+1].x;
				tmp.y = ctrl_1[j+1].y;
				cvCircle(img1, tmp, 2, s, 2, 8, 0);
			}
			if (tmp.x  > width)
				width = tmp.x;
			if (tmp.y  > height)
				height = tmp.y;

			cvShowImage( "SELECT_ROI", img1 );
		}
		ctrl_1count = 0;

		cvSetImageROI(img, cvRect(0, 0, width, height));
		cvSaveImage("intermediate.jpg", img);
		cvResetImageROI(img);
		cvDestroyWindow("SELECT_ROI");

		cvReleaseImage(&img1);
		cvReleaseImage(&img2);
		cvReleaseImage(&img);


	}

	close(fd);
}
