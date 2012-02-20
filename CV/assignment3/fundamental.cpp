/*
This program is to compute fundamental matrix and then to
draw corresponding eplines.
it also claculate world co-ordinate considering image's left corner
as the world coordinate system's origin.
*/
#include <cv.h>
#include <highgui.h>
#include "stdio.h"
#include "string.h"
#include "getopt.h"

/* all defines for this program*/
#define U_L  186.11619191
#define V_L  164.15264850
#define SX_L 1.0166343583
#define F_L  16.551086572
#define TX_L -621.06754176
#define TY_L -58.069551431
#define TZ_L 984.55520522
#define RX_L 0.15540547317 
#define RY_L 0.27888534145 
#define RZ_L 0.017528059127

#define U_R  193.89675221
#define V_R  144.43431051
#define SX_R 1.0116374294
#define F_R  16.842326127
#define TX_R -659.19737229 
#define TY_R -76.572279751
#define TZ_R 1055.8014876
#define RX_R 0.16112722935
#define RY_R 0.36219027236
#define RZ_R 0.026911763 

#define MAX_POINT	20	
/* all static/global variables for this program*/
static CvPoint2D32f ctrl_f[MAX_POINT];
static CvPoint2D32f ctrl_s[MAX_POINT];
static int ctrl_fcount = 0;
static int ctrl_scount = 0;
int number = 5;

/* Prints help for this program */
void help_fundamental (char *prog_name)
{
	FILE *stream = stdout;

	fprintf (stream, "Usage: %s options [ first ... ]\n", prog_name);
	fprintf (stream,
			" -f	--first		name of first image\n"
			" -s	--second	name of second image\n"
			" -n	--number	number of points on each images\n");
	exit (0);
}

/*mouse callback function for selecting points on first image*/

void select_point1( int event, int x, int y, int flags, void* param )
{
	if( event == CV_EVENT_LBUTTONDOWN){
		printf("x = %d & y = %d\n", x, y);

		if (ctrl_fcount  < number) {
			ctrl_f[ctrl_fcount].x = x;
			ctrl_f[ctrl_fcount++].y = y;
		}
	}
}

/*mouse callback function for selecting points on second image*/

void select_point2( int event, int x, int y, int flags, void* param )
{
	if( event == CV_EVENT_LBUTTONDOWN){
		printf("x = %d & y = %d\n", x, y);

		if (ctrl_scount  < number){
			ctrl_s[ctrl_scount].x = x;
			ctrl_s[ctrl_scount++].y = y;
		}
	}
}

void compute_rotation(float rx, float ry, float rz, CvMat *rot)
{
	CvMat *temp_1 = cvCreateMat(3, 3,CV_32F);
	CvMat *temp_2 = cvCreateMat(3, 3,CV_32F);
	
	cvmSet(temp_1, 0, 0, 1); 
	cvmSet(temp_1, 0, 1, 0); 
	cvmSet(temp_1, 0, 2, 0); 
	cvmSet(temp_1, 1, 0, 0);
	cvmSet(temp_1, 1, 1, cos(rx));
	cvmSet(temp_1, 1, 2, -sin(rx));
	cvmSet(temp_1, 2, 0, 0);
	cvmSet(temp_1, 2, 1, sin(rx));
	cvmSet(temp_1, 2, 2, cos(rx));
	cvmSet(temp_2, 0, 0, cos(ry)); 
	cvmSet(temp_2, 0, 1, 0); 
	cvmSet(temp_2, 0, 2, sin(ry)); 
	cvmSet(temp_2, 1, 0, 0);
	cvmSet(temp_2, 1, 1, 1);
	cvmSet(temp_2, 1, 2, 0);
	cvmSet(temp_2, 2, 0, -sin(ry));
	cvmSet(temp_2, 2, 1, 0);
	cvmSet(temp_2, 2, 2, cos(ry));
	cvMatMul(temp_1, temp_2, rot);
	cvmSet(temp_1, 0, 0, cos(rz)); 
	cvmSet(temp_1, 0, 1, -sin(rz)); 
	cvmSet(temp_1, 0, 2, 0); 
	cvmSet(temp_1, 1, 0, sin(rz));
	cvmSet(temp_1, 1, 1, cos(rz));
	cvmSet(temp_1, 1, 2, 0);
	cvmSet(temp_1, 2, 0, 0);
	cvmSet(temp_1, 2, 1, 0);
	cvmSet(temp_1, 2, 2, 1);
	temp_2 = cvCloneMat(rot);
	cvMatMul(temp_1, temp_2, rot);
	cvReleaseMat(&temp_1);
	cvReleaseMat(&temp_2);
}
void compute_translation(float tx, float ty, float tz, CvMat *tr)
{
	cvmSet(tr, 0, 0, tx); 
	cvmSet(tr, 1, 0, ty); 
	cvmSet(tr, 2, 0, tz); 
}
void compute_cross(CvMat *tr, CvMat *tr_cross)
{
	cvmSet(tr_cross, 0, 0, 0);
	cvmSet(tr_cross, 0, 1, -cvmGet(tr, 2, 0));
	cvmSet(tr_cross, 0, 2, cvmGet(tr, 1, 0));
	cvmSet(tr_cross, 1, 0, cvmGet(tr, 2, 0));
	cvmSet(tr_cross, 1, 1, 0);
	cvmSet(tr_cross, 1, 2, -cvmGet(tr, 0, 0));
	cvmSet(tr_cross, 2, 0, -cvmGet(tr, 1, 0));
	cvmSet(tr_cross, 2, 1, cvmGet(tr, 0, 0));
	cvmSet(tr_cross, 2, 2, 0);
}
void compute_internal(float u, float v, float sx, float f, float p_width, float p_height, CvMat *a)
{
	cvmSet(a, 0, 0, f * p_width);
	cvmSet(a, 0, 1, sx);
	cvmSet(a, 0, 2, u);
	cvmSet(a, 1, 0, 0);
	cvmSet(a, 1, 1, f * p_height);
	cvmSet(a, 1, 2, v);
	cvmSet(a, 2, 0, 0);
	cvmSet(a, 2, 1, 0);
	cvmSet(a, 2, 2, 1);
}
void compute_trs(CvMat *tr, CvMat *trs, int number)
{
	int i;
	for(i = 0; i < number; i++) {
		cvmSet(trs, 0, i, cvmGet(tr, 0, 0));	
		cvmSet(trs, 1, i, cvmGet(tr, 1, 0));	
		cvmSet(trs, 2, i, cvmGet(tr, 2, 0));	
	}
}
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
/*main routine of this program.
takes varibale arg. details can be seen in help*/
int main(int argc, char **argv)
{
	int next_option;
	const char* const short_options = "hf:s:n:";
	const struct option long_options[] = {
		{ "help",     0, NULL, 'h' },
		{ "first",   1, NULL, 'f' },
		{ "second",   1, NULL, 's' },
		{ "number",   1, NULL, 'n' },
		{ NULL,       0, NULL, 0   }
	};
	IplImage	*img1, *img2;
	char first[50];
	char second[50];
	CvMat* points1;
	CvMat* points2;
	CvMat* status;
	CvMat* rot_l;
	CvMat* rot_r;
	CvMat* rot_l_i;
	CvMat* rot_r_i;
	CvMat* rot;
	CvMat* rot_i;
	CvMat* tr_l;
	CvMat* tr_r;
	CvMat* tr;
	CvMat* tr_cross;
	CvMat* e;
	CvMat* a_l;
	CvMat* a_l_i;
	CvMat* a_l_i_t;
	CvMat* a_r;
	CvMat* a_r_i;
	CvMat* a_r_i_t;
	CvMat* temp3x3;
	CvMat* temp3x1;
	CvMat*	f1;
	CvMat*	f2;
	CvMat* lines;
	CvMat* trs;
	CvMat* p_min_trs;
	CvMat* pws;
	int i, j, i1, i2 = 0;
	CvScalar s1[MAX_POINT];
	CvScalar s2[MAX_POINT];

	/* get all user input in local variables */
	do {
		next_option = getopt_long (argc, argv, short_options,
				long_options, NULL);
		switch (next_option)
		{
			case 'h':
				help_fundamental(argv[0]);
				return 0;
			case -1:
				break;
			case 'f':
				strcpy(first, optarg);
				break;
			case 's':
				strcpy(second, optarg);
				break;
			case 'n':
				number = atoi(optarg);
				break;
			default:
				abort ();
		}
	} while (next_option != -1);

	/*alocate all temp matrices.Could have been implemented with lesser number
	of variables. But took so many for better understanding of code*/
	rot_l = cvCreateMat(3, 3, CV_32F);
	rot_l_i = cvCreateMat(3, 3, CV_32F);
	rot_r = cvCreateMat(3, 3, CV_32F);
	rot_r_i = cvCreateMat(3, 3, CV_32F);
	rot = cvCreateMat(3, 3, CV_32F);
	rot_i = cvCreateMat(3, 3, CV_32F);
	tr_l = cvCreateMat(3, 1, CV_32F);
	tr_r = cvCreateMat(3, 1, CV_32F);
	tr = cvCreateMat(3, 1, CV_32F);
	tr_cross = cvCreateMat(3, 3, CV_32F);
	e = cvCreateMat(3, 3, CV_32F);
	a_l = cvCreateMat(3, 3, CV_32F);
	a_l_i = cvCreateMat(3, 3, CV_32F);
	a_l_i_t = cvCreateMat(3, 3, CV_32F);
	a_r = cvCreateMat(3, 3, CV_32F);
	a_r_i = cvCreateMat(3, 3, CV_32F);
	a_r_i_t = cvCreateMat(3, 3, CV_32F);
	temp3x3 = cvCreateMat(3, 3, CV_32F);
	temp3x1 = cvCreateMat(3, 1, CV_32F);
	f1 = cvCreateMat(3, 3, CV_32F);
	f2 = cvCreateMat(3, 3, CV_32F);
	lines= cvCreateMat(3,number,CV_32F);
	points1 = cvCreateMat(3, number,CV_32F);
	points2 = cvCreateMat(3, number,CV_32F);  
	trs = cvCreateMat(3, number,CV_32F);  
	p_min_trs = cvCreateMat(3, number,CV_32F);  
	pws = cvCreateMat(3, number,CV_32F);  

	/*calculate fundamental matrix for both the cases now*/
	compute_rotation(RX_L, RY_L, RZ_L, rot_l);
	compute_rotation(RX_R, RY_R, RZ_R, rot_r);
	cvInvert(rot_l, rot_l_i); 
	cvInvert(rot_r, rot_r_i); 
	compute_translation(TX_L, TY_L, TZ_L, tr_l);
	compute_translation(TX_R, TY_R, TZ_R, tr_r);
	compute_internal(U_L, V_L, SX_L, F_L, 384/8.8, 288/6.6, a_l);
	compute_internal(U_L, V_L, SX_L, F_L, 384/8.8, 288/6.6, a_r);
	cvInvert(a_l, a_l_i);
	cvTranspose(a_l_i, a_l_i_t);
	cvInvert(a_r, a_r_i);
	cvTranspose(a_r_i, a_r_i_t);

	cvMatMul(rot_r, rot_l_i, rot);
	cvInvert(rot, rot_i);
	cvMatMul(rot, tr_l, temp3x1);
	cvSub(tr_r, temp3x1, tr);
	compute_cross(tr, tr_cross);
	cvMatMul(tr_cross, rot, e); 

	cvMatMul(a_r_i_t, e, temp3x3);
	cvMatMul(temp3x3, a_l_i, f1);
	printf("Fundamental Matrix 1 is:\n");
	print_matrix(f1, 3, 3);

	cvMatMul(rot_l, rot_r_i, rot);
	cvMatMul(rot, tr_r, temp3x1);
	cvSub(tr_l, temp3x1, tr);
	compute_cross(tr, tr_cross);
	cvMatMul(tr_cross, rot, e); 

	cvMatMul(a_l_i_t, e, temp3x3);
	cvMatMul(temp3x3, a_r_i, f2);
	printf("Fundamental Matrix 2 is:\n");
	print_matrix(f2, 3, 3);

	/*calculate epiploar line now*/

	/*load src & dst image*/
	img1 = cvLoadImage( "left.jpg", 1);
	//img1 = cvLoadImage( first, 1);
	if (!img1){
		printf("Error in reading first image\n");
		return -1;
	}
	img2 = cvLoadImage( "right.jpg", 1);
	if (!img2){
		printf("Error in reading second image\n");
		return -1;
	}

	//assume that images are the same size
	CvSize i_size = cvGetSize(img1);


	printf("select %d points with mouse left click on IMAGE1 to draw corresponding epipolar line in IMAGE2 and vice-versa\n", number);
	printf("selcet first point on IMAGE1, second on IMAGE2, third on IMAGE1 and so on...\n");
	/*create window for image display*/
	cvNamedWindow( "IMAGE1", 1 );
	cvNamedWindow( "IMAGE2", 1 );
	cvMoveWindow("IMAGE2",400,0);
	/*slect  points from mouse*/
	cvSetMouseCallback( "IMAGE1", select_point1, 0 );
	cvSetMouseCallback( "IMAGE2", select_point2, 0 );
	cvShowImage( "IMAGE1", img1 );
	cvShowImage( "IMAGE2", img2 );
	for (i = 0;i < number; i++) {
		/* wait till user selects point*/
		while (i >= ctrl_fcount)
			cvWaitKey(300);
		for (j = -1;j < i;j++) {
			CvPoint	tmp;
			CvScalar s = cvScalar(0+rand()%255, 0+rand()%255, 0+rand()%255, 0);			
			s1[j+1] = s;
			tmp.x =  ctrl_f[j+1].x;	
			tmp.y =  ctrl_f[j+1].y;	
			cvSetReal2D(points1, 0, j + 1, tmp.x);
			cvSetReal2D(points1, 1, j + 1, tmp.y);
			cvSetReal2D(points1, 2, j + 1, 1);
			cvCircle(img1, tmp, 2, s, 2, 8, 0);
		}
		cvShowImage( "IMAGE1", img1 );
		/* wait till user selects point*/
		while (i >= ctrl_scount)
			cvWaitKey(300);
		for (j = -1;j < i;j++) {
			CvPoint	tmp;
			CvScalar s = cvScalar(0+rand()%255, 0+rand()%255, 0+rand()%255, 0);			
			s2[j+1] = s;
			tmp.x = ctrl_s[j+1].x;	
			tmp.y = ctrl_s[j+1].y;	
			cvSetReal2D(points2, 0, j + 1, tmp.x);
			cvSetReal2D(points2, 1, j + 1, tmp.y);
			cvSetReal2D(points2, 2, j + 1, 1);
			cvCircle(img2, tmp, 2, s, 2, 8, 0);
		}
		cvShowImage( "IMAGE2", img2 );
	}

	/*calculate eplines and draw them*/
	cvMatMul(f2, points2, lines);
	for (i = 0; i < number; i++) {
		CvPoint ep1, ep2;
		float a, b, c;
		cvCircle(img2,cvPoint(cvmGet(points2,0,i),cvmGet(points2,1,i)),5,CV_RGB(255,255,0),1);
		cvShowImage("IMAGE2",img2);
		//printf("press any key to go ahead\n");
		cvWaitKey(200);

		a = cvmGet(lines, 0, i);
		b = cvmGet(lines, 1, i);
		c = cvmGet(lines, 2, i);
		ep1.x = 0;
		ep2.x = i_size.width - 1; 
		ep1.y = -c / b; 
		ep2.y = (-c - a * ep2.x) / b;
		cvLine(img1, ep1, ep2, s2[i]);
		cvShowImage("IMAGE1",img1);
		//printf("press any key to go ahead\n");
		cvWaitKey(200);

	}

	cvMatMul(f1, points1, lines);
	for (i = 0; i < number; i++) {
		CvPoint ep1, ep2;
		float a, b, c;
		cvCircle(img1,cvPoint(cvmGet(points1,0,i),cvmGet(points1,1,i)),5,CV_RGB(255,255,0),1);
		cvShowImage("IMAGE1",img1);
		cvSaveImage("left_op.jpg", img1);
		//printf("press any key to go ahead\n");
		cvWaitKey(200);

		a = cvmGet(lines, 0, i);
		b = cvmGet(lines, 1, i);
		c = cvmGet(lines, 2, i);
		ep1.x = 0;
		ep2.x = i_size.width - 1; 
		ep1.y = -c / b; 
		ep2.y = (-c - a * ep2.x) / b;
		cvLine(img2, ep1, ep2, s1[i]);
		cvShowImage("IMAGE2",img2);
		cvSaveImage("right_op.jpg", img2);
		//printf("press any key to go ahead\n");
		cvWaitKey(200);

	}
	printf("press any key to calculate world co-ordinate\n");
	cvWaitKey(0);

	/*calculate world coordinates corresponding to first image*/
	for(j = 0; j < number; j++) {
		float x = cvmGet(points1, 0, j);
		x -= U_L;
		x *= 8.8;
		x /= SX_L;
		x /= F_L;
		cvmSet(points1, 0, j, x);	
	}

	for(j = 0; j < number; j++) {
		float y = cvmGet(points1, 1, j);
		y -= V_L;
		y *= 6.6;
		y /= F_L;
		cvmSet(points1, 0, j, y);	
	}

	compute_trs(tr_l, trs, number);
	cvSub(points1, trs, p_min_trs);
	cvMatMul(rot_l_i, p_min_trs, pws);
	printf("world co-ordinate corresponding to points in first images are\n");
	print_matrix(pws, 3, number);
	/*calculate world coordinates corresponding to second image*/
	for(j = 0; j < number; j++) {
		float x = cvmGet(points2, 0, j);
		x -= U_R;
		x *= 8.8;
		x /= SX_R;
		x /= F_R;
		cvmSet(points2, 0, j, x);	
	}

	for(j = 0; j < number; j++) {
		float y = cvmGet(points2, 1, j);
		y -= V_R;
		y *= 6.6;
		y /= F_R;
		cvmSet(points2, 0, j, y);	
	}
	compute_trs(tr_r, trs, number);
	cvSub(points2, trs, p_min_trs);
	cvMatMul(rot_r_i, p_min_trs, pws);
	printf("world co-ordinate corresponding to points in second images are\n");
	print_matrix(pws, 3, number);

	/*release all memory now*/
	cvReleaseImage(&img1);
	cvReleaseImage(&img2);
	cvReleaseMat(&rot_l);
	cvReleaseMat(&rot_l_i);
	cvReleaseMat(&rot_r);
	cvReleaseMat(&rot_r_i);
	cvReleaseMat(&rot);
	cvReleaseMat(&tr_l);
	cvReleaseMat(&tr_r);
	cvReleaseMat(&tr);
	cvReleaseMat(&tr_cross);
	cvReleaseMat(&e);
	cvReleaseMat(&a_l);
	cvReleaseMat(&a_l_i);
	cvReleaseMat(&a_l_i_t);
	cvReleaseMat(&a_r);
	cvReleaseMat(&a_r_i);
	cvReleaseMat(&a_r_i_t);
	cvReleaseMat(&temp3x3);
	cvReleaseMat(&temp3x1);
	cvReleaseMat(&f1);
	cvReleaseMat(&f2);
	cvReleaseMat(&lines);
	cvReleaseMat(&points1);
	cvReleaseMat(&points2);
	cvReleaseMat(&trs);
	cvReleaseMat(&p_min_trs);
	cvReleaseMat(&pws);
	cvDestroyWindow("IMAGE1");
	cvDestroyWindow("IMAGE2");
}
