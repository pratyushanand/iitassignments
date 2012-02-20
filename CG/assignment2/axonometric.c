#include <GL/glut.h>
#include <stdlib.h>
#include <math.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int width = 500, height = 500;
static GLfloat theta_x = 35.264, theta_y = -45.0;
static GLfloat n[6][3] = {  /* Normals for the 6 faces of a cube. */
  {-1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, {1.0, 0.0, 0.0},
  {0.0, -1.0, 0.0}, {0.0, 0.0, 1.0}, {0.0, 0.0, -1.0} };
static GLint faces[6][4] = {  /* Vertex indices for the 6 faces of a cube. */
  {0, 1, 2, 3}, {3, 2, 6, 7}, {7, 6, 5, 4},
  {4, 5, 1, 0}, {5, 6, 2, 1}, {7, 4, 0, 3} };
static GLfloat v[8][3];  /* Will be filled in with X,Y,Z vertexes. */
static GLfloat c[6][3] = { /*color for different faces*/
{1.0, 0.0, 0.0},
{1.0, 1.0, 0.0},
{0.0, 1.0, 1.0},
{0.0, 1.0, 0.0},
{0.0, 0.0, 1.0},
{1.0, 0.0, 1.0}};

/* Prints help for axonometric program */

static void help_axonometric (char *prog_name)
{
	FILE *stream = stdout;

	fprintf (stream, "Usage: %s options [ inputfile ... ]\n", prog_name);
	fprintf (stream,
			" -h	--help		Display this usage information.\n"
			" -x	--theta_x	angle of rotation around x axis.\n"
			" -y	--theta_y	angle of rotation about y axis.\n");
	exit (0);
}
static int xstart, ystart;
static void mouse_drag_callback(int x, int y)
{
	int xin = x, yin = y, i;
	char angle[20];
	x = xin - xstart;
	y = yin - ystart;
	if (x || y) {
		theta_x += asin(y/ sqrt(x*x + y*y));
		theta_y += asin(x/ sqrt(x*x + y*y));
		if (theta_x < 0)
			theta_x += 360;
		if (theta_y < 0)
			theta_y += 360;
		if (theta_x > 360)
			theta_x -= 360;
		if (theta_y > 360)
			theta_y -= 360;
	}
	printf("theta_x = %f theta_y = %f \n", theta_x, theta_y);
	xstart = xin;
	ystart = yin;
	glutPostRedisplay();
}
static void mouse_press_callback(int button, int state, int x, int y)
{
	if(button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
		xstart = x;
		ystart = y;
	}
}

/*function to draw axonometic_projection*/
static void axonometic_projection()
{
	int i;
	/*set correct matrix mode */
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(-5.0f, 5.0f, -5.0f, 5.0f, -5.0f, 5.0f);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glRotatef(theta_x, 1.0f, 0.0f, 0.0f);
	glRotatef(theta_y, 0.0f, 1.0f, 0.0f);
	glScalef(1.0f, 1.0f, -1.0f);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glEnable(GL_DEPTH_TEST);


	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	/*Draw axis */
	glBegin(GL_LINE);
	glColor3d(1.0, 0.0, 0.0);
	glVertex3d(0.0, 0.0, 0.0);
	glVertex3d(100.0, 0.0, 0.0);
	glColor3d(0.0, 1.0, 0.0);
	glVertex3d(0.0, 0.0, 0.0);
	glVertex3d(0.0, 100.0, 0.0);
	glColor3d(0.0, 0.0, 1.0);
	glVertex3d(0.0, 0.0, 0.0);
	glVertex3d(0.0, 0.0, 100.0);
	glEnd();
	glutSwapBuffers();

	for (i = 0; i < 6; i++) {
		glBegin(GL_POLYGON);
		glColor3f(c[i][0], c[i][1], c[i][2]);
		glNormal3fv(&n[i][0]);
		glVertex3fv(&v[faces[i][0]][0]);
		glVertex3fv(&v[faces[i][1]][0]);
		glVertex3fv(&v[faces[i][2]][0]);
		glVertex3fv(&v[faces[i][3]][0]);
		glEnd();
		glutSwapBuffers();
	}
}
/* main function for axonometric projection.
 * it takes input as x and y tilt angle
 */
int main(int argc, char **argv)
{
	int next_option;
	const char* const short_options = "hx:y:";
	const struct option long_options[] = {
		{ "help",     0, NULL, 'h' },
		{ "theta_x",   1, NULL, 'x' },
		{ "theta_y",   1, NULL, 'y' },
		{ NULL,       0, NULL, 0   }
	};

	/* get all user input in local variables */
	do {
		next_option = getopt_long (argc, argv, short_options,
				long_options, NULL);
		switch (next_option)
		{
			case 'h':
				help_axonometric(argv[0]);
				return 0;
			case -1:
				break;
			case 'x':
				theta_x = atof(optarg);
				break;
			case 'y':
				theta_y= atof(optarg);
				break;
			default:
				abort ();
		}
	} while (next_option != -1);
	/* Setup cube vertex data. */
	v[0][0] = v[1][0] = v[2][0] = v[3][0] = -1;
	v[4][0] = v[5][0] = v[6][0] = v[7][0] = 1;
	v[0][1] = v[1][1] = v[4][1] = v[5][1] = -1;
	v[2][1] = v[3][1] = v[6][1] = v[7][1] = 1;
	v[0][2] = v[3][2] = v[4][2] = v[7][2] = 1;
	v[1][2] = v[2][2] = v[5][2] = v[6][2] = -1;
	/* this is necessary function to be called for using glut
	 * library. Should be called only once in a program
	 */
	glutInit(&argc, argv);
	glutInitDisplayMode (GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
	/* Set the window size */
	glutInitWindowSize (width, height);
	/* Set the window position */
	glutInitWindowPosition (100, 100);
	/* name the output window*/
	glutCreateWindow ("Axonometric Projection Cube");    /* Create the window */

	/*set window's background color */
	glClearColor(1.0, 1.0, 1.0, 0.0);
	glViewport(0, 0, 500, 500);
	/* implement callback to draw line */
	glutDisplayFunc(axonometic_projection);
	glutMotionFunc(mouse_drag_callback);
	glutMouseFunc(mouse_press_callback);
	/*call infinite main loop*/
	glutMainLoop();
}
