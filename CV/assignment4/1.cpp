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
int frame_num = 0;
//int last_change = 100;

// temporary images
IplImage *mhi = 0; // MHI
//IplImage *orient = 0; // orientation
IplImage *mask = 0; // valid orientation mask
IplImage *segmask = 0; // motion segmentation map
CvMemStorage* storage = 0; // temporary storage

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
	//double angle;
	CvPoint center;
	double magnitude;
	CvScalar color;
        int largest_element = 0;
        double largest_area, current_area;
//	cvNamedWindow( "test", 1 );

	// allocate images at the beginning or
	// reallocate them if the frame size is changed
	if( !mhi || mhi->width != size.width || mhi->height != size.height ) {
//		cvNamedWindow( "test", 1 );
		if( buf == 0 ) {
			buf = (IplImage**)malloc(N*sizeof(buf[0]));
			memset( buf, 0, N*sizeof(buf[0]));
		}

		for( i = 0; i < N; i++ ) {
			cvReleaseImage( &buf[i] );
			buf[i] = cvCreateImage( size, IPL_DEPTH_8U, 1 );
			cvZero( buf[i] );
	//		cvCvtColor( img, buf[i], CV_BGR2GRAY ); // convert frame to grayscale
		}
		cvReleaseImage( &mhi );
		//cvReleaseImage( &orient );
		cvReleaseImage( &segmask );
		cvReleaseImage( &mask );

		mhi = cvCreateImage( size, IPL_DEPTH_32F, 1 );
		cvZero( mhi ); // clear MHI at the beginning
		//orient = cvCreateImage( size, IPL_DEPTH_32F, 1 );
		segmask = cvCreateImage( size, IPL_DEPTH_32F, 1 );
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
	printf("frame number is %d, timestamp%f\n", frame_num, timestamp);
#if 1
	// convert MHI to blue 8u image
	cvCvtScale( mhi, mask, 255./MHI_DURATION,
			(MHI_DURATION - timestamp)*255./MHI_DURATION );
	//cvCvtScale( mhi, mask, 255, 0);
	cvZero( dst );
//	cvCopy( img, dst );
//	cvMerge( mask, 0, 0, 0, dst );
#endif
#if 1
	// calculate motion gradient orientation and valid orientation mask
	//cvCalcMotionGradient( mhi, mask, orient, MAX_TIME_DELTA, MIN_TIME_DELTA, 3 );

	if( !storage )
		storage = cvCreateMemStorage(0);
	else
		cvClearMemStorage(storage);

	// segment motion: get sequence of motion components
	// segmask is marked motion components map. It is not used further
	seq = cvSegmentMotion( mhi, segmask, storage, timestamp, MAX_TIME_DELTA );
//	cvShowImage( "test", segmask );

	// iterate through the motion components,
	// One more iteration (i == -1) corresponds to the whole image (global motion)
	printf("seq total is %d", seq->total);
        largest_area = 0.0;
	color = CV_RGB(255,0,0);
	magnitude = 30;

	if(seq->total == 0) {
		cvCopy(img, dst);
		return;
	}
	for( i = 0; i < seq->total; i++ ) {
			comp_rect = ((CvConnectedComp*)cvGetSeqElem( seq, i ))->rect;
			if (comp_rect.width == 320)
				continue;

//                        current_area = comp_rect.width * comp_rect.height;
                        current_area =  ((CvConnectedComp*)cvGetSeqElem( seq, i ))->area;
                        if (current_area > largest_area) {
                                largest_area = current_area;
                                largest_element = i;
                        }
	}
        i=largest_element;
        comp_rect = ((CvConnectedComp*)cvGetSeqElem( seq, i ))->rect;
        printf("Total Elements : %d, Largest Element = %d, size - %d, %d\n", seq -> total, i, comp_rect.width, comp_rect.height);
	//cvDrawContours( temp, ((CvConnectedComp*)cvGetSeqElem( seq, i ))->contour, cvScalar(100), cvScalar(200), -1, CV_FILLED, 8 );
	//cvDrawContours( temp, ((CvConnectedComp*)cvGetSeqElem( seq, i ))->contour, CV_RGB(100, 100, 100), CV_RGB(200, 200, 200), -1, CV_FILLED, 8 );
	//cvDrawContours( temp, seq, CV_RGB(100, 100, 100), CV_RGB(200, 200, 200), -1, CV_FILLED, 8 );
//	cvShowImage( "test", mask);

			

/*
		if( i < 0 ) { // case of the whole image
			comp_rect = cvRect( 0, 0, size.width, size.height );
			color = CV_RGB(255,255,255);
			magnitude = 100;
		}
		else { // i-th motion component
			comp_rect = ((CvConnectedComp*)cvGetSeqElem( seq, i ))->rect;
			if( comp_rect.width + comp_rect.height < 200 ) // reject very small components
				continue;
			color = CV_RGB(255,0,0);
			magnitude = 30;
		}
*/

		// select component ROI
		cvSetImageROI( silh, comp_rect );
		cvSetImageROI( mhi, comp_rect );
		//cvSetImageROI( orient, comp_rect );
		cvSetImageROI( mask, comp_rect );

		// calculate orientation
		//angle = cvCalcGlobalOrientation( orient, mask, mhi, timestamp, MHI_DURATION);
		//angle = 360.0 - angle;  // adjust for images with top-left origin

		count = cvNorm( silh, 0, CV_L1, 0 ); // calculate number of points within silhouette ROI
			cvResetImageROI( mhi );
			//cvResetImageROI( orient );
			cvResetImageROI( mask );
			cvResetImageROI( silh );
		// check for the case of little motion
		if( count < comp_rect.width*comp_rect.height * 0.05 ){
                        printf("Seems some error ... Small no of pixels moving\n");
			//continue;
		}
#if 1
		printf("i is %d\n", i);
		for (i = 0; i < size.height; i++) {
			for (j = 0 ; j < size.width; j++) {
				uchar	*ptrm;
				uchar	*ptrd;
				uchar	*ptri;
				ptrm= (uchar*) (mask->imageData + i * mask->widthStep);
				ptri= (uchar*) (img->imageData + i * img->widthStep);
				ptrd= (uchar*) (dst->imageData + i * dst->widthStep);
//				if ( ptrm[j] && (i >= comp_rect.y) && (j >= comp_rect.x) && (i < comp_rect.y + comp_rect.height) && (j < comp_rect.x + comp_rect.width) ) {
				//if (( seq->total <= 1) ||  (comp_rect.height > 310 ) || (comp_rect.width > 230) || (last_x > comp_rect.x+25) || ((frame_num > 100)&& (last_change + 10 < frame_num ))) {
				if (frame_num > 314 ){//|| (last_x > comp_rect.x+25) || ((frame_num > 100)&& (last_change + 10 < frame_num ))) {
					ptrd[3*j] = ptri[3*j];
					ptrd[3*j+1] = ptri[3*j+1];
					ptrd[3*j+2] = ptri[3*j+2];
				} else if ( ptrm[j] && (i >= comp_rect.y) && (j >= comp_rect.x) && (i < comp_rect.y + comp_rect.height) && (j < comp_rect.x + comp_rect.width) ) {
                                        //last_change = frame_num;
                                        //last_x = comp_rect.x;
					ptrd[3*j] = 255;
					ptrd[3*j+1] = 255;
					ptrd[3*j+2] = 255;
				} else {
					ptrd[3*j] = ptri[3*j];
					ptrd[3*j+1] = ptri[3*j+1];
					ptrd[3*j+2] = ptri[3*j+2];
				}
			}
		}
#endif



		// draw a clock with arrow indicating the direction
/*		center = cvPoint( (comp_rect.x + comp_rect.width/2),
				(comp_rect.y + comp_rect.height/2) );

		cvCircle( dst, center, cvRound(magnitude*1.2), color, 3, CV_AA, 0 );
		cvLine( dst, center, cvPoint( cvRound( center.x + magnitude*cos(angle*CV_PI/180)),
					cvRound( center.y - magnitude*sin(angle*CV_PI/180))), color, 3, CV_AA, 0 );*/
	//}
#endif
      frame_num++;
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

	if( capture )
	{
		cvNamedWindow( "Motion", 1 );

		for(;;)
		{
                        printf("***************************************\n");
                        printf("             New Frame                 \n");
			IplImage* image = cvQueryFrame( capture );
			if( !image )
				break;

			if( !motion )
			{
				motion = cvCreateImage( cvSize(image->width,image->height), 8, 3 );
				cvZero( motion );
				motion->origin = image->origin;
			}

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
