
#include <GL/glut.h>
#include <stdlib.h>
#include <math.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>

static GLint xstart = 10, ystart = 10, xend = 50, yend = 50, width = 500, height = 500;

/* Prints help for bresenhem program */

void help_bresenham (char *prog_name)
{
	FILE *stream = stdout;

	fprintf (stream, "Usage: %s options [ inputfile ... ]\n", prog_name);
	fprintf (stream,
			" -h	--help		Display this usage information.\n"
			" -x	--xstart	x coordinate of start point.\n"
			" -y	--ystart	y coordinate of start point.\n"
			" -e	--xend		x coordinate of end point.\n"
			" -z	--yend		y coordinate of end point.\n"
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


/*function to draw bresenham_line*/
void bresenham_line()
{
	GLint x, y, d0, dpp, dpn;
	GLint deltax = fabs(xend - xstart);
	GLint deltay = fabs(yend - ystart);

	glClear(GL_COLOR_BUFFER_BIT);
	glColor3f(1.0, 0.0, 0.0);
	glPointSize(4.0);
	draw_axis();
	if (deltay < deltax) {

		/* determine where to start */
		if (xstart > xend) {
			x = xend;
			y = yend;
			xend = xstart;
			yend = ystart;
		} else {
			x = xstart;
			y = ystart;
		}

		d0 = 2 * deltay - deltax;
		dpn = 2 * deltay;
		dpp = 2 * (deltay - deltax);
		while (x < xend) {
			draw_pixel(x, y);
			if (d0 < 0){
				d0 += dpn;
			} else {
				d0 += dpp;
				if (yend > y)
					y++;
				else
					y--;
			}
			x++;
		}
	}
	else {
		/* determine where to start */
		if (ystart > yend) {
			x = xend;
			y = yend;
			yend = ystart;
			xend = xstart;
		} else {
			x = xstart;
			y = ystart;
		}
		d0 = 2 * deltax - deltay;
		dpn = 2 * deltax;
		dpp = 2 * (deltax - deltay);
		while (y < yend) {
			draw_pixel(x, y);
			if (d0 < 0){
				d0 += dpn;
			} else {
				d0 += dpp;
				if (xend > x)
					x++;
				else
					x--;
			}
			y++;
		}
	}
}
/* main function for bresenham algorithm inplementation.
 * it takes some standard glut input parameters and also program specific.
 * standard parameter for this program:
 * -geometry WIDTHxHEIGHT+XOFF+YOFF
 */
int main(int argc, char **argv)
{
	int next_option;
	const char* const short_options = "hx:y:e:z:w:t:";
	const struct option long_options[] = {
		{ "help",     0, NULL, 'h' },
		{ "xstart",   1, NULL, 'x' },
		{ "ystart",   1, NULL, 'y' },
		{ "xend",   1, NULL, 'e' },
		{ "yend",   1, NULL, 'z' },
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
				help_bresenham(argv[0]);
				return 0;
			case -1:
				break;
			case 'x':
				xstart = atoi(optarg);
				break;
			case 'y':
				ystart = atoi(optarg);
				break;
			case 'e':
				xend = atoi(optarg);
				break;
			case 'z':
				yend = atoi(optarg);
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
	glutDisplayFunc(bresenham_line);
	/*call infinite main loop*/
	glutMainLoop();
}
