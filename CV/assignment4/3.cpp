// motion templates sample code
#include "cv.h"
#include "highgui.h"
#include <time.h>
#include <math.h>
#include <ctype.h>
#include <stdio.h>

// various tracking parameters (in seconds)
const double MHI_DURATION = 0.25;
const double MAX_TIME_DELTA = 0.250;
const double MIN_TIME_DELTA = 0.030;
// number of cyclic frame buffer used for motion detection
// (should, probably, depend on FPS)
const int N = 10;

// ring image buffer
IplImage **buf = 0;
int last = 0;
//int last_x = 0;

// temporary images
IplImage *mhi = 0; // MHI
IplImage *mask = 0; // valid orientation mask
CvPoint  ptt[5];


// parameters:
//  img - input video frame
//  dst - resultant motion picture
//  args - optional parameters
void  update_mhi( IplImage* img, IplImage* dst, int diff_threshold )
{
	double timestamp = (double)clock()/CLOCKS_PER_SEC; // get current time in seconds
	CvSize size = cvSize(img->width,img->height); // get current frame size
	int i,j, idx1 = last, idx2;
	IplImage* silh;
	IplImage* temp;
	CvSeq* seq;
	CvRect comp_rect;
	double count;
	CvPoint center;
	double magnitude;
	CvScalar color;
	int largest_element = 0;
	double largest_area, current_area;

	// allocate images at the beginning or
	// reallocate them if the frame size is changed
	if( !mhi || mhi->width != size.width || mhi->height != size.height ) {
		if( buf == 0 ) {
			buf = (IplImage**)malloc(N*sizeof(buf[0]));
			memset( buf, 0, N*sizeof(buf[0]));
		}

		for( i = 0; i < N; i++ ) {
			cvReleaseImage( &buf[i] );
			buf[i] = cvCreateImage( size, IPL_DEPTH_8U, 1 );
			cvZero( buf[i] );
		}
		cvReleaseImage( &mhi );
		cvReleaseImage( &mask );

		mhi = cvCreateImage( size, IPL_DEPTH_32F, 1 );
		cvZero( mhi );
		mask = cvCreateImage( size, IPL_DEPTH_8U, 1 );
		temp = cvCreateImage( size, IPL_DEPTH_8U, 3 );
	}

	cvCvtColor( img, buf[last], CV_BGR2GRAY ); // convert frame to grayscale

	idx2 = (last + 1) % N; // index of (last - (N-1))th frame
	last = idx2;

	silh = buf[idx2];
	cvAbsDiff( buf[idx1], buf[idx2], silh ); // get difference between frames

	cvThreshold( silh, silh, diff_threshold, 1, CV_THRESH_BINARY); // and threshold it
	cvUpdateMotionHistory( silh, mhi, timestamp, MHI_DURATION ); // update MHI
	// convert MHI to blue 8u image
	cvCvtScale( mhi, mask, 255./MHI_DURATION,
			(MHI_DURATION - timestamp)*255./MHI_DURATION );
	for (i = 0; i < size.height; i++) {
		for (j = 0 ; j < size.width; j++) {
			uchar	*ptrm;
			uchar	*ptrd;
			uchar	*ptri;
			ptrm= (uchar*) (mask->imageData + i * mask->widthStep);
			ptri= (uchar*) (img->imageData + i * img->widthStep);
			ptrd= (uchar*) (dst->imageData + i * dst->widthStep);
			if ( ptrm[j] ){
				ptrd[3*j] = ptri[3*j];
				ptrd[3*j+1] = ptri[3*j +1];
				ptrd[3*j+2] = ptri[3*j +2];
			}
		}
	}
	return;
}


int main(int argc, char** argv)
{
	IplImage* motion = 0;
	CvCapture* capture = 0;

	if( argc == 1 || (argc == 2 && strlen(argv[1]) == 1 && isdigit(argv[1][0])))
		capture = cvCaptureFromCAM( argc == 2 ? argv[1][0] - '0' : 0 );
	else if( argc == 2 )
		capture = cvCaptureFromFile( argv[1] );
	capture = cvCaptureFromFile( "umcp.mpg" );
	ptt[0].x = 0;
	ptt[0].y = 220;
	ptt[1].x = 160;
	ptt[1].y = 134;
	ptt[2].x = 202;
	ptt[2].y = 180;
	ptt[3].x = 121;
	ptt[3].y = 235;
	ptt[4].x = 0;
	ptt[4].y = 240;

	if( capture )
	{
		cvNamedWindow( "Motion", 1 );

		for(;;)
		{
			IplImage* image = cvQueryFrame( capture );
			if( !image )
				break;

			if( !motion )
			{
				motion = cvCreateImage( cvSize(image->width,image->height), 8, 3 );
				cvZero( motion );
				motion->origin = image->origin;
			}
			cvCopy(image, motion);
			cvFillConvexPoly( motion, ptt, 5, cvScalar(100), CV_AA);

			update_mhi( image, motion, 30 );
			cvShowImage( "Motion", motion );

			if( cvWaitKey(33) >= 0 )
				break;
		}
		cvReleaseCapture( &capture );
		cvDestroyWindow( "Motion" );
	}

	return 0;
}
