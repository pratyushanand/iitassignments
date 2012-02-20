/*this program does morphing of two images. One need to select 
  control points using mouse. Number of control points can be
  either 1, 2 or 3. One can select number of intermediate frames
  as many as required.*/
#include <cv.h>
#include <highgui.h>
#include "stdio.h"
#include "string.h"
#include "getopt.h"

/* all defines for this program*/
#define MAX_CTRL_POINT	20 
#define MAX_TRIANGLE_COUNT	42 

/* all static/global variables for this program*/
static CvPoint2D32f ctrl_s[MAX_CTRL_POINT + 4];
static CvPoint2D32f ctrl_d[MAX_CTRL_POINT + 4];
static int ctrl_scount = 4;
static int ctrl_dcount = 4;
static int ctrl_point_num = 1;
static int	src_triangle[MAX_TRIANGLE_COUNT][3];
static int	dst_triangle[MAX_TRIANGLE_COUNT][3];
static int src_triangle_count = 0;
static int dst_triangle_count = 0;
static int width;
static int height;

/* Prints help for morphing program */
void help_morphing (char *prog_name)
{
	FILE *stream = stdout;

	fprintf (stream, "Usage: %s options [ first ... ]\n", prog_name);
	fprintf (stream,
			" -f	--first		name of first image.(Max 50 Char)\n"
			" -s	--second	name of second image.(Max 50 Char)\n"
			" -o	--output	name of output images.(Max 50 Char)\n"
			" -i	--iterations	no if interations or intermediate frames.\n"
			" -c	--control	no of control points.(Max 20)\n");
	exit (0);
}

/*mouse callback function for selecting points on first image*/

void select_ctrl_point1( int event, int x, int y, int flags, void* param )
{
	if( event == CV_EVENT_LBUTTONDOWN){
		printf("x = %d & y = %d\n", x, y);

		/*first 4 control points are corners*/
		if (ctrl_scount - 4 < ctrl_point_num) {
			ctrl_s[ctrl_scount].x = x;
			ctrl_s[ctrl_scount++].y = y;
		}
	}
}

/*mouse callback function for selecting points on second image*/

void select_ctrl_point2( int event, int x, int y, int flags, void* param )
{
	if( event == CV_EVENT_LBUTTONDOWN){
		printf("x = %d & y = %d\n", x, y);

		/*first 4 control points are corners*/
		if (ctrl_dcount -4 < ctrl_point_num){
			ctrl_d[ctrl_dcount].x = x;
			ctrl_d[ctrl_dcount++].y = y;
		}
	}
}

/*a generic function to initilize delaunay : taken from openC reference book*/
CvSubdiv2D* init_delaunay( CvMemStorage* storage,
		CvRect rect )
{
	CvSubdiv2D* subdiv;

	subdiv = cvCreateSubdiv2D( CV_SEQ_KIND_SUBDIV2D, sizeof(*subdiv),
			sizeof(CvSubdiv2DPoint),
			sizeof(CvQuadEdge2D),
			storage );
	cvInitSubdivDelaunay2D( subdiv, rect );

	return subdiv;
}

/*draw lines for each triangle: standard way of doing: 
again taken from opencv reference book*/
void draw_subdiv_edge( IplImage* img, CvSubdiv2DEdge edge, CvScalar color )
{
	CvSubdiv2DEdge	e;
	CvSubdiv2DPoint* org_pt;
	CvSubdiv2DPoint* dst_pt;
	CvPoint2D32f org;
	CvPoint2D32f dst;
	CvPoint iorg, idst;
	CvPoint t1, t2, t3;
	CvNextEdgeType	type = CV_NEXT_AROUND_LEFT;

	org_pt = cvSubdiv2DEdgeOrg(edge);
	dst_pt = cvSubdiv2DEdgeDst(edge);
	if( org_pt && dst_pt )
	{
		org = org_pt->pt;
		dst = dst_pt->pt;

		iorg = cvPoint( cvRound( org.x ), cvRound( org.y ));
		idst = cvPoint( cvRound( dst.x ), cvRound( dst.y ));
		cvLine( img, iorg, idst, color, 1, CV_AA, 0 );

	}
}

/*triangulate using delaunay algorithm: standard way of doing: 
again taken from opencv reference book*/

void draw_subdiv( IplImage* img, CvSubdiv2D* subdiv,
		CvScalar delaunay_color)
{
	CvSeqReader  reader;
	int i, total = subdiv->edges->total;
	int elem_size = subdiv->edges->elem_size;

	cvStartReadSeq( (CvSeq*)(subdiv->edges), &reader, 0 );

	for( i = 0; i < total; i++ )
	{
		CvQuadEdge2D* edge = (CvQuadEdge2D*)(reader.ptr);

		if( CV_IS_SET_ELEM( edge ))
		{
			draw_subdiv_edge( img, (CvSubdiv2DEdge)edge, delaunay_color );
		}

		CV_NEXT_SEQ_ELEM( elem_size, reader );
	}
}
/*this function tells whether a triangle exist for point (x,y). if it exists then it returns
triangle index. It also returns vertices of triangle in pt0, pt1 and pt2.
if src is 1 then for src image otherwise of dst image */

int is_triangle_exist(CvSubdiv2D* subdiv, int src, int x, int y, int *pt0, int *pt1, int *pt2)
{
	int count;
	CvSubdiv2DEdge e;
	CvSubdiv2DEdge e0 = 0;
	CvSubdiv2DPoint* p = 0;
	CvSubdiv2DPoint* org_pt = 0;
	CvSubdiv2DPoint* dst_pt = 0;
	CvPoint2D32f	fp;
	CvPoint2D32f org;
	CvPoint2D32f dst;
	CvPoint t1, t2, t3;
	int k, i;
	int p0, p1, p2;
	CvPoint2D32f *ctrl;

	if (src)
		ctrl = ctrl_s;	
	else
		ctrl = ctrl_d;
	/*insert point in subdivision*/
	fp.x = x;
	fp.y = y;
	cvSubdiv2DLocate( subdiv, fp, &e0, &p );

	/*travel through all 3 corners*/
	if( e0 ) {
		e = e0;
		do {
			e = cvSubdiv2DGetEdge(e,CV_NEXT_AROUND_LEFT);
			org_pt = cvSubdiv2DEdgeOrg(e);
			dst_pt = cvSubdiv2DEdgeDst(e);
			org = org_pt->pt;
			dst = dst_pt->pt;

			t3 = t1;
			t1 = cvPoint( cvRound( org.x ), cvRound( org.y ));
			t2 = cvPoint( cvRound( dst.x ), cvRound( dst.y ));
		} while( e != e0 );
	}
	/*store them in increasing order: for convenience of search*/
	for (k = 0;k < ctrl_point_num + 4;k++) {
		if (t1.x == ctrl[k].x && t1.y == ctrl[k].y) {
			p0 = k; 
			break;
		}
	}
	for (k = 0;k < ctrl_point_num + 4;k++) {
		if (t2.x == ctrl[k].x && t2.y == ctrl[k].y) {
			if (p0 < k)
				p1 = k;
			else {
				p1 = p0;
				p0 = k;
			}
			break;
		}
	}
	for (k = 0;k < ctrl_point_num + 4;k++) {
		if (t3.x == ctrl[k].x && t3.y == ctrl[k].y) {
			if (p1 < k)
				p2 = k;
			else if(p0 < k) {
				p2 = p1;
				p1 = k;
			}
			else {
				p2 = p1;
				p1 = p0;	
				p0 = k;
			}
			break;
		}
	}
	*pt0 = p0;
	*pt1 = p1;
	*pt2 = p2;
	/*now check if these points are existing in the arr*/
	if (src) {
		for (i = 0; i < src_triangle_count; i++) {
			if((src_triangle[i][0] == p0) 
					&& (src_triangle[i][1] == p1)
					&& (src_triangle[i][2] == p2))
				return i;
		}
	}
	else {
		for (i = 0; i < dst_triangle_count; i++) {
			if((dst_triangle[i][0] == p0) 
					&& (dst_triangle[i][1] == p1)
					&& (dst_triangle[i][2] == p2))
				return i;
		}
	}
	return -1;
}
/*prepaes traingle point array:
src = 1 for source image and 0 for destination image*/

void store_triangle_point( CvSubdiv2D* subdiv, int src, IplImage* img)
{
	int neighbour_x[] = {1, 1, 0, -1, -1, -1, 0, 1};	
	int neighbour_y[] = {0, 1, 1, 1, 0, -1, -1, -1};	
	int i, j, x, y;
	int p0, p1, p2, p3;
	CvPoint2D32f *ctrl;

	if (src)
		ctrl = ctrl_s;	
	else
		ctrl = ctrl_d;


	for (i = 0; i < ctrl_point_num + 4; i++) {
		for (j = 0;j < 8; j++) {
			/*handle first 4 corner points*/
			/*for first point, handle only 4th quadrant*/
			if (i == 0)
				if (j != 1)
					continue;
			/*for second point, handle only 3rd quadrant*/
			if (i == 1)
				if (j != 3)
					continue;
			/*for third point, handle only 2nd quadrant*/
			if (i == 2)
				if (j != 5)
					continue;
			/*for fourth point, handle only 1st quadrant*/
			if (i == 3)
				if (j != 7)
					continue;
			x	= ctrl[i].x + neighbour_x[j];
			y	= ctrl[i].y + neighbour_y[j];
			/*here we try to find all possible triangles across selectd control points.
			if point does not exist then insert it in array*/
			if(is_triangle_exist(subdiv, src, x, y, &p0, &p1, &p2) == -1){
				if (src) {
					src_triangle[src_triangle_count][0] = p0;
					src_triangle[src_triangle_count][1] = p1;
					src_triangle[src_triangle_count++][2] = p2;
				}
				else {
					dst_triangle[dst_triangle_count][0] = p0;
					dst_triangle[dst_triangle_count][1] = p1;
					dst_triangle[dst_triangle_count++][2] = p2;
				}
			}

		}
	}

}
/*opencv delaunay triangulation function does not find triangle correctly, if it lies
on boundary. Handle that here.*/

int triangle_number(CvSubdiv2D* subdiv, int src, int x, int y)
{
	int p0, p1, p2;
	int i ;
	/*handle boundary points*/
	if (x == 0) {
		if (src) {
			for (i = 0; i < src_triangle_count; i++) {
				if((src_triangle[i][0] == 0) 
						&& (src_triangle[i][1] == 3))
					return i;
			}
		}
		else {
			for (i = 0; i < dst_triangle_count; i++) {
				if((dst_triangle[i][0] == 0) 
						&& (dst_triangle[i][1] == 3))
					return i;
			}
		}
	}
	else if (x == width - 1) {
		if (src) {
			for (i = 0; i < src_triangle_count; i++) {
				if((src_triangle[i][0] == 1) 
						&& (src_triangle[i][1] == 2))
					return i;
			}
		}
		else {
			for (i = 0; i < dst_triangle_count; i++) {
				if((dst_triangle[i][0] == 1) 
						&& (dst_triangle[i][1] == 2))
					return i;
			}
		}
	}
	else if (y == 0) {
		if (src) {
			for (i = 0; i < src_triangle_count; i++) {
				if((src_triangle[i][0] == 0) 
						&& (src_triangle[i][1] == 1))
					return i;
			}
		}
		else {
			for (i = 0; i < dst_triangle_count; i++) {
				if((dst_triangle[i][0] == 0) 
						&& (dst_triangle[i][1] == 1))
					return i;
			}
		}
	}
	else if (y == height - 1) {
		if (src) {
			for (i = 0; i < src_triangle_count; i++) {
				if((src_triangle[i][0] == 2) 
						&& (src_triangle[i][1] == 3))
					return i;
			}
		}
		else {
			for (i = 0; i < dst_triangle_count; i++) {
				if((dst_triangle[i][0] == 2) 
						&& (dst_triangle[i][1] == 3))
					return i;
			}
		}
	}
	else
		return is_triangle_exist(subdiv, src, x, y, &p0, &p1, &p2);
}

/*main routine of this program.
takes varibale arg. details can be seen in help*/
int main(int argc, char **argv)
{
	int next_option;
	const char* const short_options = "hf:s:o:i:c:";
	const struct option long_options[] = {
		{ "help",     0, NULL, 'h' },
		{ "first",   1, NULL, 'f' },
		{ "second",   1, NULL, 's' },
		{ "output",   1, NULL, 'o' },
		{ "iterations",   1, NULL, 'i' },
		{ "control",   1, NULL, 'c' },
		{ NULL,       0, NULL, 0   }
	};
	int iterations;
	IplImage	*img1, *img2, *img3;
	IplImage	*img[MAX_TRIANGLE_COUNT];
	char	name[50];
	int i, j, k ,t;
	double	m[MAX_CTRL_POINT];
	double	c[MAX_CTRL_POINT];
	static CvPoint2D32f ctrl_i[MAX_CTRL_POINT];
	CvMat*        warp_mat;
	CvRect rect = { 0, 0, 1200, 1200 };
	CvMemStorage	*storage1, *storage2;
	CvSubdiv2D	*subdiv1, *subdiv2;
	char first[50];
	char second[50];
	char output[50];

	/*default intilization of possible user args*/
	char def_f[] = "menace.jpg";
	char def_s[] = "panther.jpg";
	char def_o[] = "output_10i_3c";
	ctrl_point_num = 3;
	iterations = 11;
	
	strcpy(first, def_f);	
	strcpy(second, def_s);	
	strcpy(output, def_o);	

	/* get all user input in local variables */
	do {
		next_option = getopt_long (argc, argv, short_options,
				long_options, NULL);
		switch (next_option)
		{
			case 'h':
				help_morphing(argv[0]);
				return 0;
			case -1:
				break;
			case 'f':
				strcpy(first, optarg);
				break;
			case 's':
				strcpy(second, optarg);
				break;
			case 'o':
				strcpy(output, optarg);
				break;
			case 'i':
				iterations = atoi(optarg);
				iterations++;
				break;
			case 'c':
				ctrl_point_num = atoi(optarg);
				break;
			default:
				abort ();
		}
	} while (next_option != -1);
	/*load src & dst image*/
	img1 = cvLoadImage( first, 1);
	img2 = cvLoadImage( second, 1);
	if ((img1->width != img2->width) && (img1->height != img2->height))
		return -1;
	else {
		width = img1->width;
		height = img1->height;
	}

	/*create space for output image*/
	img3 = cvCreateImage(cvSize(width, height), 8, 3 );
	img3->origin = img1->origin;
	cvZero(img3);

	/*space for affine transformation matrix*/
	warp_mat = cvCreateMat(2,3,CV_32FC1);

	/*create window for image display*/
	cvNamedWindow( "IMAGE1", 1 );
	cvNamedWindow( "IMAGE2", 1 );
	cvMoveWindow("IMAGE2",400,0);


	/*slect control points from mouse*/
	cvSetMouseCallback( "IMAGE1", select_ctrl_point1, 0 );
	cvSetMouseCallback( "IMAGE2", select_ctrl_point2, 0 );
	printf("select %d control point(1-2-1-2...) with mouse left click\n", ctrl_point_num);
	cvShowImage( "IMAGE1", img1 );
	cvShowImage( "IMAGE2", img2 );

	for (i = 4;i < ctrl_point_num + 4;i++) {
		/* wait till user selects point*/
		while (i >= ctrl_scount)
			cvWaitKey(300);
		for (j = -1;j < i;j++) {
			CvPoint	tmp;
			CvScalar s = cvScalar(0+rand()%255, 0+rand()%255, 0+rand()%255, 0);			
			tmp.x = ctrl_s[j+1].x;	
			tmp.y = ctrl_s[j+1].y;	
			cvCircle(img1, tmp, 2, s, 2, 8, 0);
		}
		cvShowImage( "IMAGE1", img1 );
		/* wait till user selects point*/
		while (i >= ctrl_dcount)
			cvWaitKey(300);
		for (j = -1;j < i;j++) {
			CvPoint	tmp;
			CvScalar s = cvScalar(0+rand()%255, 0+rand()%255, 0+rand()%255, 0);			
			tmp.x = ctrl_d[j+1].x;	
			tmp.y = ctrl_d[j+1].y;	
			cvCircle(img2, tmp, 2, s, 2, 8, 0);
		}
		cvShowImage( "IMAGE2", img2 );
	}
	/*initilize 4 defalut control points as corners of image*/
	ctrl_s[0].x = 0;
	ctrl_s[0].y = 0;
	ctrl_s[1].x = width - 1;
	ctrl_s[1].y = 0;
	ctrl_s[2].x = width - 1; 
	ctrl_s[2].y = height - 1;
	ctrl_s[3].x = 0;
	ctrl_s[3].y = height -1;
	ctrl_d[0].x = 0;
	ctrl_d[0].y = 0;
	ctrl_d[1].x = width - 1;
	ctrl_d[1].y = 0;
	ctrl_d[2].x = width - 1;
	ctrl_d[2].y = height - 1;
	ctrl_d[3].x = 0;
	ctrl_d[3].y = height - 1;
	ctrl_i[0].x = 0;
	ctrl_i[0].y = 0;
	ctrl_i[1].x = width - 1;
	ctrl_i[1].y = 0;
	ctrl_i[2].x = width - 1;
	ctrl_i[2].y = height - 1;
	ctrl_i[3].x = 0;
	ctrl_i[3].y = height - 1;

	/*reloade input images*/
	img1 = cvLoadImage( first, 1);
	img2 = cvLoadImage( second, 1);     

	/*triangulate both images*/
	storage1 = cvCreateMemStorage(0);
	subdiv1 = init_delaunay( storage1, rect );
	storage2 = cvCreateMemStorage(0);
	subdiv2 = init_delaunay( storage2, rect );

	for (i = 0;i < ctrl_point_num + 4;i++) 
		cvSubdivDelaunay2DInsert( subdiv1, ctrl_s[i]);


	draw_subdiv(img1, subdiv1, CV_RGB( 255,0,0));


	for (i = 0;i < ctrl_point_num + 4;i++) 
		cvSubdivDelaunay2DInsert( subdiv2, ctrl_d[i]);
	draw_subdiv(img2, subdiv2, CV_RGB( 255,0,0));


	/*store triangle points in aray*/
	store_triangle_point(subdiv1, 1, img1);
	store_triangle_point(subdiv2, 0, img2);

	/*show triangulation: just for user convenience*/
	cvShowImage( "IMAGE1", img1 );
	cvShowImage( "IMAGE2", img2 );
	printf("press any key....\n");
	cvWaitKey(0);

	if(src_triangle_count != dst_triangle_count) {
		printf("Error with control point selection!!!!\n");
		goto error;
	}
	/*create image for affine transformation for all corresponding triangles*/
	for (i = 0; i < src_triangle_count; i++) {
		img[i] = cvCreateImage(cvSize(width, height), 8, 3 );
		img[i]->origin = img1->origin;
		cvZero(img[i]);
	}

	for (i = 4;i < ctrl_point_num + 4;i++) {
		/*calculate slope and intercept of lines between control points
		  of two images*/
		/*m = (Y1 - Y2) / (X1 - X2)*/
		m[i] = (double)(ctrl_s[i].y - ctrl_d[i].y)
			/ (double) (ctrl_s[i].x - ctrl_d[i].x);
		/*c = (Y1 - m * X1)*/
		c[i] = (double)ctrl_s[i].y - m[i] * (double)ctrl_s[i].x;

	}
	for (j = 0;j <= iterations;j++) {
		printf("generating images %d\n", j);

		/*calculate intermediate control points for */
		for (i = 4;i < ctrl_point_num+4;i++) {
			ctrl_i[i].x = ctrl_s[i].x + (int)(((double)j/iterations) * (ctrl_d[i].x - ctrl_s[i].x));
			ctrl_i[i].y = m[i] * ctrl_i[i].x + c[i];
		}

		/*prepare images for all src triangles*/
		for (i = 0; i < src_triangle_count; i++) {
			CvPoint2D32f ctrls[3];
			CvPoint2D32f ctrli[3];
			for (k = 0; k < 3;k++) {
				ctrls[k].x = ctrl_s[src_triangle[i][k]].x;
				ctrls[k].y = ctrl_s[src_triangle[i][k]].y;
				ctrli[k].x = ctrl_i[src_triangle[i][k]].x;
				ctrli[k].y = ctrl_i[src_triangle[i][k]].y;
			}
			img1 = cvLoadImage( first, 1);
			cvGetAffineTransform(ctrls, ctrli, warp_mat);
			cvWarpAffine(img1, img[i], warp_mat, CV_INTER_LINEAR+CV_WARP_FILL_OUTLIERS, cvScalarAll(0));
			cvCopy(img[i], img1);
		}

		/*prepare a final image on the basis of actual points from
		all images*/
		for(i = 0; i < height;i++) {
			for(k = 0;k < width;k++) {
				uchar* ptri;
				uchar* ptrs;
				t = triangle_number(subdiv1, 1, k, i);
				if (t < 0) {
					printf("bug: not a valid pixel\n");
					goto error;
				}
				ptri= (uchar*) (img[t]->imageData + i * img[t]->widthStep);
				ptrs = (uchar*) (img1->imageData + i * img1->widthStep);
				ptrs[3*k] = ptri[3*k];
				ptrs[3*k+1] = ptri[3*k+1];
				ptrs[3*k+2] = ptri[3*k+2];
			}

		} 

		/*prepare images for all dst triangles*/
		for (i = 0; i < dst_triangle_count; i++) {
			CvPoint2D32f ctrld[3];
			CvPoint2D32f ctrli[3];
			for (k = 0; k < 3;k++) {
				ctrld[k].x = ctrl_d[dst_triangle[i][k]].x;
				ctrld[k].y = ctrl_d[dst_triangle[i][k]].y;
				ctrli[k].x = ctrl_i[dst_triangle[i][k]].x;
				ctrli[k].y = ctrl_i[dst_triangle[i][k]].y;
			}
			img2 = cvLoadImage( second, 1);
			cvGetAffineTransform(ctrld, ctrli, warp_mat);
			cvWarpAffine(img2, img[i], warp_mat, CV_INTER_LINEAR+CV_WARP_FILL_OUTLIERS, cvScalarAll(0));
			cvCopy(img[i], img2);
		}

		/*prepare a final image on the basis of actual points from
		all images*/
		for(i = 0; i < height;i++) {
			for(k = 0;k < width;k++) {
				uchar* ptri;
				uchar* ptrd;
				t = triangle_number(subdiv2, 0, k, i);
				if (t < 0) {
					printf("bug: not a valid pixel\n");
					goto error;
				}
				ptri= (uchar*) (img[t]->imageData + i * img[t]->widthStep);
				ptrd = (uchar*) (img2->imageData + i * img2->widthStep);
				ptrd[3*k] = ptri[3*k];
				ptrd[3*k+1] = ptri[3*k+1];
				ptrd[3*k+2] = ptri[3*k+2];
			}

		}

		/*do the interpolation now*/
		cvAddWeighted(img1, (1 - (double)j/iterations),
				img2, (double)j/iterations,
				0, img3);

		/*save intermediate images*/
		sprintf(name, "%s%d.jpg", output, j);
		cvSaveImage(name, img3);
	}
error:
	/*free all memoryies*/
	for (i = 0;i< src_triangle_count;i++)
		cvReleaseImage(&img[i]);
	cvReleaseImage(&img1);
	cvReleaseImage(&img2);
	cvReleaseImage(&img3);
	cvReleaseMat(&warp_mat);
	cvDestroyWindow("IMAGE1");
	cvDestroyWindow("IMAGE2");

}
