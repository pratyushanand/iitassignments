#include <cv.h>
#include <highgui.h>
#include "stdio.h"

int n_boards = 0;
const int board_dt = 20;
int board_w;
int board_h;


void print_matrix(CvMat *mat, int row, int col)
{
	int i, j;
	for (i = 0; i < row; i++) {
		for (j = 0; j < col; j++) {
			printf("%4.10f\t", cvmGet(mat, i, j));	
		}
		printf("\n");
	} 
}

int main(int argc, char* argv[])
{
	board_w = 10; // Board width in squares
	board_h = 10; // Board height 
	n_boards = 2; // Number of boards
	int board_n = board_w * board_h;
	CvSize board_sz = cvSize( board_w, board_h );

	double quality_level = 0.1;
	double min_distance = 5;
	int eig_block_size = 3;
	int input = 1;
	int i, j, k, max;


	cvNamedWindow( "Calibration", 1 );
	// Allocate Sotrage
	CvMat* image_points		= cvCreateMat( n_boards*board_n, 2, CV_32FC1 );
	CvMat* object_points		= cvCreateMat( n_boards*board_n, 3, CV_32FC1 );
	CvMat* point_counts			= cvCreateMat( n_boards, 1, CV_32SC1 );
	CvMat* intrinsic_matrix		= cvCreateMat( 3, 3, CV_32FC1 );
	CvMat* distortion_coeffs	= cvCreateMat( 4, 1, CV_32FC1 );

	CvPoint2D32f* corners = new CvPoint2D32f[ board_n ];
	int corner_count = board_n;
	int successes = 0;
	int step ;
	float world_x, world_y;

	IplImage *image = cvLoadImage("left1.16mm.jpg", 1);
	IplImage *gray_image = cvCreateImage( cvGetSize( image ), IPL_DEPTH_8U, 1 );
	IplImage *eig_image = cvCreateImage( cvGetSize( image ), IPL_DEPTH_32F, 1 );
	IplImage *temp_image = cvCreateImage( cvGetSize( image ), IPL_DEPTH_32F, 1 );



	while( successes < n_boards ){
		cvCvtColor(image, gray_image, CV_BGR2GRAY);

		cvGoodFeaturesToTrack(gray_image, eig_image, temp_image, corners,
				&corner_count, quality_level, min_distance,
				NULL, eig_block_size, 0);
		for(i=0;i<corner_count;i++){
			if(corners[i].x == 381 && corners[i].y == 286) {
				if(input==1) {
					corners[i].x = 228;
					corners[i].y = 140;
				}

				if(input==2) {
					corners[i].x = 235;
					corners[i].y = 221;
				}
			}
		}

		for(i=0;i<corner_count;i++) {
			for(j=i+1;j<corner_count;j++) {
				if(corners[i].x > corners[j].x)	{
					int tmpx, tmpy;
					tmpx = corners[i].x;
					tmpy = corners[i].y;
					corners[i].x=corners[j].x;
					corners[i].y=corners[j].y;
					corners[j].x = tmpx;
					corners[j].y = tmpy;
				}
			}
		}

		for(k=0;k<corner_count;k=k+10) {
			max=10;
			for(i=k;i<k+max;i++) {
				for(j=i+1;j<k + max;j++) {
					if(corners[i].y > corners[j].y) {
						int tmpx, tmpy;
						tmpx = corners[i].x;
						tmpy = corners[i].y;
						corners[i].x=corners[j].x;
						corners[i].y=corners[j].y;
						corners[j].x = tmpx;
						corners[j].y = tmpy;
					}
				}
			}
		}




		// Draw it
		cvDrawChessboardCorners( image, board_sz, corners, corner_count, 1 );
		cvShowImage( "Calibration", image );



		// If we got a good board, add it to our data
		if( corner_count == board_n ){
			step = successes*board_n;
			world_x = 500.0f; world_y = 0.0f;
			for( int i=step, j=0; j < board_n; ++i, ++j ){
				//			        printf("world = %f,%f,%f\n",world_x, world_y, 200.0*successes);
				CV_MAT_ELEM( *image_points, float, i, 0 ) = corners[j].x;
				CV_MAT_ELEM( *image_points, float, i, 1 ) = corners[j].y;
				CV_MAT_ELEM( *object_points, float, i, 0 ) = world_x;
				CV_MAT_ELEM( *object_points, float, i, 1 ) = world_y;
				CV_MAT_ELEM( *object_points, float, i, 2 ) = 200.0*successes;

				switch(j%2){
					case 0 : world_y = world_y+40.0; break; 
					case 1 : world_y = world_y+20.0; break;
				}

				if (j%10 == 9){
					world_y = 0.0;
					switch((j/10)%2){
						case 0 : world_x = world_x+40.0; break; 
						case 1 : world_x = world_x+20.0; break;
					}
				}
			}
			CV_MAT_ELEM( *point_counts, int, successes, 0 ) = board_n;
			successes++;
		}

		cvWaitKey( 0 );
		image = cvLoadImage("left2.16mm.jpg", 1);
		input = 2;
	} 

	// Allocate matrices according to how many chessboards found
	CvMat* object_points2 = cvCreateMat( successes*board_n, 3, CV_32FC1 );
	CvMat* image_points2 = cvCreateMat( successes*board_n, 2, CV_32FC1 );
	CvMat* point_counts2 = cvCreateMat( successes, 1, CV_32SC1 );

	// Transfer the points into the correct size matrices
	for( int i = 0; i < successes*board_n; ++i ){
		CV_MAT_ELEM( *image_points2, float, i, 0) = CV_MAT_ELEM( *image_points, float, i, 0 );
		CV_MAT_ELEM( *image_points2, float, i, 1) = CV_MAT_ELEM( *image_points, float, i, 1 );
		CV_MAT_ELEM( *object_points2, float, i, 0) = CV_MAT_ELEM( *object_points, float, i, 0 );
		CV_MAT_ELEM( *object_points2, float, i, 1) = CV_MAT_ELEM( *object_points, float, i, 1 );
		CV_MAT_ELEM( *object_points2, float, i, 2) = CV_MAT_ELEM( *object_points, float, i, 2 );
	}

	for( int i=0; i < successes; ++i ){
		CV_MAT_ELEM( *point_counts2, int, i, 0 ) = CV_MAT_ELEM( *point_counts, int, i, 0 );
	}

	CV_MAT_ELEM( *intrinsic_matrix, float, 0, 0 ) = 16.0;
	CV_MAT_ELEM( *intrinsic_matrix, float, 0, 1 ) = 0.0;
	CV_MAT_ELEM( *intrinsic_matrix, float, 0, 2 ) = 192.0;
	CV_MAT_ELEM( *intrinsic_matrix, float, 1, 0 ) = 0.0;
	CV_MAT_ELEM( *intrinsic_matrix, float, 1, 1 ) = 16.0*288/384;
	CV_MAT_ELEM( *intrinsic_matrix, float, 1, 2 ) = 144.0;
	CV_MAT_ELEM( *intrinsic_matrix, float, 2, 0 ) = 0.0;
	CV_MAT_ELEM( *intrinsic_matrix, float, 2, 1 ) = 0.0;
	CV_MAT_ELEM( *intrinsic_matrix, float, 2, 2 ) = 1.0;

	CvMat* rot	= cvCreateMat( 2, 3,  CV_32FC1 );
	CvMat* tr	= cvCreateMat( 2, 3, CV_32FC1 );
	// Calibrate the camera
	printf("\nCalibrating Camera ......\n");
	cvCalibrateCamera2( object_points2, image_points2, point_counts2, cvGetSize( image ), 
			intrinsic_matrix, distortion_coeffs, rot, tr,
			CV_CALIB_USE_INTRINSIC_GUESS|CV_CALIB_ZERO_TANGENT_DIST ); 

	printf("\nIntrinsic Matrix......\n");
	print_matrix(intrinsic_matrix, 3, 3);
	printf("\nDistortion Matrix.....\n");
	print_matrix(distortion_coeffs, 4, 1);
	printf("\nRoation Vector.....\n");
	print_matrix(rot, 2, 3);
	printf("\nTranslation Vector.....\n");
	print_matrix(tr, 2, 3);

	float alpha1, beta1, gamma1, image_error, object_error;
	CvMat *rMatrix = cvCreateMat( 3, 4, CV_32FC1 );
	CvMat *m_matrix = cvCreateMat( 3, 3, CV_32FC1 );
	CvMat *objectPrime = cvCreateMat( 3,n_boards*board_n, CV_32FC1 );
	image_error = 0.0;

	for (k=0;k<2;k++){
		alpha1 = cvmGet(rot, k, 0);
		beta1  = cvmGet(rot, k, 1);
		gamma1 = cvmGet(rot, k, 2);
		CV_MAT_ELEM( *rMatrix, float, 0, 0 ) = cos(beta1)*cos(gamma1);
		CV_MAT_ELEM( *rMatrix, float, 0, 1 ) = -cos(alpha1)*sin(gamma1)+cos(gamma1)*sin(alpha1)*sin(beta1);
		CV_MAT_ELEM( *rMatrix, float, 0, 2 ) = sin(alpha1)*sin(gamma1)+cos(alpha1)*cos(gamma1)*sin(beta1);
		CV_MAT_ELEM( *rMatrix, float, 0, 3 ) = cvmGet(tr, k, 0);
		CV_MAT_ELEM( *rMatrix, float, 1, 0 ) = cos(beta1)*sin(gamma1);
		CV_MAT_ELEM( *rMatrix, float, 1, 1 ) = cos(alpha1)*cos(gamma1)+sin(alpha1)*sin(beta1)*sin(gamma1);
		CV_MAT_ELEM( *rMatrix, float, 1, 2 ) = -cos(gamma1)*sin(alpha1)+cos(alpha1)*sin(beta1)*sin(gamma1);
		CV_MAT_ELEM( *rMatrix, float, 1, 3 ) = cvmGet(tr, k, 1);
		CV_MAT_ELEM( *rMatrix, float, 2, 0 ) = -sin(beta1);
		CV_MAT_ELEM( *rMatrix, float, 2, 1 ) = cos(beta1)*sin(alpha1);
		CV_MAT_ELEM( *rMatrix, float, 2, 2 ) = cos(alpha1)*cos(beta1);
		CV_MAT_ELEM( *rMatrix, float, 2, 3 ) = cvmGet(tr, k, 2);

		cvMatMul(intrinsic_matrix,rMatrix,rMatrix);

		for(i=0;i<3;i++){
			for(j=0;j<3;j++){
				CV_MAT_ELEM( *m_matrix, float, i, j ) = cvmGet(rMatrix, i, j);
			}
		}

		cvTranspose(object_points,objectPrime);
		cvMatMul(m_matrix,objectPrime,objectPrime);
		cvTranspose(objectPrime,object_points);

		for(i=0;i<n_boards*board_n;i++) {
			for(j=0;j<3;j++){
				CV_MAT_ELEM( *object_points, float, i, j ) = cvmGet(object_points, i, j) + cvmGet(rMatrix, j, 3); 
			}
		}

		for(i=0;i<board_n;i++) {
			for(j=0;j<2;j++){
				image_error = image_error + pow(cvmGet(image_points, i+k*board_n, j) -(cvmGet(object_points, i+k*board_n, j) / cvmGet(object_points, i+k*board_n, 2)),2);
			}
		}
	}

	image_error = (pow,image_error/(2*board_n),0.5);
	printf("\nImage Point Error = %f\n", image_error);

	cvReleaseMat( &object_points );
	cvReleaseMat( &image_points );
	cvReleaseMat( &point_counts );
	cvReleaseMat( &image_points );
	cvReleaseMat( &object_points );
	cvReleaseMat( &point_counts );
	cvReleaseMat( &intrinsic_matrix );
	cvReleaseMat( &distortion_coeffs );
	cvReleaseMat( &object_points2 );
	cvReleaseMat( &image_points2 ); 
	cvReleaseMat( &point_counts2 ); 
	cvReleaseMat( &rot );
	cvReleaseMat( &tr );
	cvReleaseMat( &rMatrix );
	cvReleaseMat( &m_matrix);
	cvReleaseMat( &objectPrime);
	return 0;
}

