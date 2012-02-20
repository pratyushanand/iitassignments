
#include <GL/glut.h>
#include <stdlib.h>
#include <math.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>


struct point{
    GLint x;
    GLint y;
};

static GLint xcenter = 0, ycenter = 0, redius = 100, width = 500, height = 500;

/* Prints help for bresenhem program */

void help_circle (char *prog_name)
{
	FILE *stream = stdout;

	fprintf (stream, "Usage: %s options [ inputfile ... ]\n", prog_name);
	fprintf (stream,
			" -h	--help		Display this usage information.\n"
			" -x	--xcenter	x coordinate of center.\n"
			" -y	--ycenter	y coordinate of center.\n"
			" -r	--redius	redius of circle.\n"
			" -w	--width		width of window.\n"
			" -t	--height	height of window.\n");
	exit (0);
}

/* draw pixel using opengl function */
void draw_pixel(GLint x, GLint y)
{
	glBegin(GL_POINTS);
	glVertex2i(x + width/2, y + height/2);
	glEnd();
	glFlush();
}



void draw_axis()
{
	GLint i;
	for (i = -width/2; i <= width/2;i++)
		draw_pixel(i, 0);
	for (i = -height/2; i <= height/2;i++)
		draw_pixel(0, i);
}


/*function to draw circle*/
void midpoint_circle()
{
	struct point *circle_pts;
	int dp, x, y, i, j;

	glClear(GL_COLOR_BUFFER_BIT);
	glColor3f(1.0, 0.0, 0.0);
	glPointSize(4.0);
	draw_axis();

	circle_pts = (struct point *)malloc(redius*sizeof(struct point));
	x = 0; y = redius; dp = 1-redius;i=0;
	(circle_pts+i)->x   = x;
	(circle_pts+i++)->y = y;
	while (y > x) {
		if (dp < 0) { // East pixel
			dp = dp + 2 * x + 3;
			x  = x+1;
			y  = y;
		} else { // SE pixel
			dp = dp + 2 * (x -y) + 5;
			x  = x+1;
			y  = y-1;
		}
		(circle_pts+i)->x   = x;
		(circle_pts+i++)->y = y;
	}

	for (j=0; j<8; j++) { // 8 octates
		for (i=0; i <= x; i++) { // all points
			switch(j){
				case 0 :  draw_pixel((circle_pts+i)->x+ xcenter,
							  (circle_pts+i)->y+ ycenter);
					  break;
				case 1 :  draw_pixel((circle_pts+i)->y+ xcenter,
							  (circle_pts+i)->x+ ycenter);
					  break;
				case 2 :  draw_pixel((circle_pts+i)->x+ xcenter,
							  - (circle_pts+i)->y+ ycenter);
					  break;
				case 3 :  draw_pixel((circle_pts+i)->y+ xcenter,
							  - (circle_pts+i)->x+ ycenter);
					  break;
				case 4 :  draw_pixel(-(circle_pts+i)->x+ xcenter,
							  (circle_pts+i)->y+ ycenter);
					  break;
				case 5 :  draw_pixel(-(circle_pts+i)->y+ xcenter,
							  (circle_pts+i)->x+ ycenter);
					  break;
				case 6 :  draw_pixel(-(circle_pts+i)->x+ xcenter,
							  - (circle_pts+i)->y+ ycenter);
					  break;
				case 7 :  draw_pixel(-(circle_pts+i)->y+ xcenter,
							  - (circle_pts+i)->x+ ycenter);
					  break;
			} // switch
		} // for i
	} // for j
	free(circle_pts);
}
/* main function for bresenham algorithm inplementation.
 * it takes some standard glut input parameters and also program specific.
 * standard parameter for this program:
 * -geometry WIDTHxHEIGHT+XOFF+YOFF
 */
int main(int argc, char **argv)
{
	int next_option;
	const char* const short_options = "hx:y:r:w:t:";
	const struct option long_options[] = {
		{ "help",     0, NULL, 'h' },
		{ "xcenter",   1, NULL, 'x' },
		{ "ycenter",   1, NULL, 'y' },
		{ "redius",   1, NULL, 'z' },
		{ "width",   1, NULL, 'w' },
		{ "height",   1, NULL, 't' },
		{ NULL,       0, NULL, 0   }
	};


	/* get all user input in local variables */
	do {
		next_option = getopt_long (argc, argv, short_options,
				long_options, NULL);
		switch (next_option)
		{
			case 'h':
				help_circle(argv[0]);
				return 0;
			case -1:
				break;
			case 'x':
				xcenter = atoi(optarg);
				break;
			case 'y':
				ycenter = atoi(optarg);
				break;
			case 'r':
				redius = atoi(optarg);
				break;
			case 'w':
				width = atoi(optarg);
				break;
			case 't':
				height = atoi(optarg);
				break;
			default:
				abort ();
		}
	} while (next_option != -1);

	/* this is necessary function to be called for using glut
	 * library. Should be called only once in a program
	 */
	glutInit(&argc, argv);
	glutInitDisplayMode (GLUT_SINGLE|GLUT_RGB);
     	/* Set the window size */
	glutInitWindowSize (width, height);
 	/* Set the window position */
	glutInitWindowPosition (100, 100);
	/* name the output window*/
	glutCreateWindow ("Scan Conversion");    /* Create the window */
	/*set window's background color */
	glClearColor(0.0, 1.0, 0.0, 0.0);
	/*set correct matrix mode */
	glMatrixMode(GL_PROJECTION);
	/* set clipping parameters:left, right, top, bottom*/
	gluOrtho2D(0.0, width, 0.0, height);
	/* implement callback to draw line */
	glutDisplayFunc(midpoint_circle);
	/*call infinite main loop*/
	glutMainLoop();
}
