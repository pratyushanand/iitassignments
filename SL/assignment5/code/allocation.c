/**
 * This program will implement a trick of card as stated in the
 * assignment problem.
 */
#include <errno.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string.h>
#include <ncurses.h>
#include <sys/queue.h>

#define MAX_NAME_LEN 50

/**
 * struct to hold cgpa info
 */
struct cgpa {
	char name[MAX_NAME_LEN];
	char dept[4];
	float marks;
	LIST_ENTRY(cgpa) list;
};

/**
 * Declares a structure which contains a pointer to the first cgpa
 * structure in the list
 */
static LIST_HEAD(cgpahead, cgpa) cgpa_head;

/**
 * struct for vacancy table
 */
struct vacancy {
	char dept[4];
	int max;
	int vacant;
	int special_vacant;
};

/**
 * declare global vacancy table for all department
 */
static struct vacancy vacancy[10];

/**
 * struct to hold choice info
 */
struct choice {
	char name[MAX_NAME_LEN];
	char choice[11][4];
	LIST_ENTRY(choice) list;
};

/**
 * Declares a structure which contains a pointer to the first choice
 * structure in the list
 */
static LIST_HEAD(choicehead, choice) choice_head;

/**
 * Login function
 * Allows login for admin and student
 */
static void login(char *name)
{
	clear();
	printw("Login:");
	refresh();
	scanw("%s", name);
}

/**
 * returns selected menu for admin
 */
static char get_admin_menu(void)
{
	char menu;

	clear();
	printw("A. Print CGPA\n");
	printw("B. Add CGPA\n");
	printw("C. Print Vacancy\n");
	printw("D. Add Vacancy\n");
	printw("E. Allocate Sheet\n");
	printw("F. View Allocation\n");
	printw("G. Lock Aplly\n");
	refresh();
	scanw("%c", &menu);
	return menu;
}

/**
 * returns selected menu for students
 */
static char get_student_menu(void)
{
	char menu;

	clear();
	printw("C. Print Vacancy\n");
	printw("F. View Allocation\n");
	printw("H. Add choice\n");
	refresh();
	scanw("%c", &menu);
	return menu;
}

/**
 * this function reads cgpa table from a file cgpa.txt and prints it to
 * the window.
 */
static void print_cgpa(void)
{
	FILE *cgpa;
	char name[MAX_NAME_LEN], ch;
	float marks;

	clear();
	cgpa = fopen("cgpa.txt", "r");
	if (!cgpa) {
		printw("Could not find CGPA info\n");
		refresh();
		return;
	}

	while (fread(&ch, sizeof(ch), 1, cgpa))
		printw("%c", ch);
	refresh();
	fclose(cgpa);
	getch();
}

/**
 * this function reads vacancy table from a file vacancy.txt and prints it to
 * the window.
 */
static void print_vacancy(void)
{
	FILE *cgpa;
	char name[MAX_NAME_LEN], ch;
	float marks;

	clear();
	cgpa = fopen("vacancy.txt", "r");
	if (!cgpa) {
		printw("Could not find vacancy info\n");
		refresh();
		return;
	}

	while (fread(&ch, sizeof(ch), 1, cgpa))
		printw("%c", ch);
	refresh();
	fclose(cgpa);
	getch();
}

static void prepare_cgpa_list()
{
	FILE *cgpa;
	char name[MAX_NAME_LEN], ch;
	float marks;
	int size;
	char *file, *cfile, *sname, *smarks, *sdept;
	struct cgpa *node, *pnode;

	cgpa = fopen("cgpa.txt", "r");
	if (!cgpa) {
		printw("Could not find CGPA info\n");
		refresh();
		return;
	}
	/* get file size */
	fseek(cgpa, 0, SEEK_END);
	size = ftell(cgpa);
	fseek(cgpa, 0, SEEK_SET);
	/*alocate memory for complete file size */
	file = malloc(size);
	fread(file, sizeof(char), size, cgpa);
	fclose(cgpa);
	cfile = file;
	sname = strsep(&cfile, "\t");
	sdept = strsep(&cfile, "\t");
	smarks = strsep(&cfile, "\n");
	if (sname && smarks && sdept) {
		marks = atof(smarks);
		node = malloc(sizeof(*node));
		strcpy(node->name, sname);
		strcpy(node->dept, sdept);
		node->marks = marks;
		LIST_INSERT_HEAD(&cgpa_head, node, list);
		pnode = node;

	}
	while (1) {
		sname = strsep(&cfile, "\t");
		if (!sname)
			break;
		sdept = strsep(&cfile, "\t");
		if (!sdept)
			break;
		smarks = strsep(&cfile, "\n");
		if (!smarks)
			break;
		marks = atof(smarks);
		node = malloc(sizeof(*node));
		strcpy(node->name, sname);
		strcpy(node->dept, sdept);
		node->marks = marks;
		LIST_INSERT_AFTER(pnode, node, list);
		pnode = node;
	}
	free(file);
}

static void dump_cgpa_list()
{
	FILE *cgpa;
	struct cgpa *node;

	cgpa = fopen("cgpa.txt", "w");
	if (!cgpa) {
		printw("Could not open CGPA file for writing\n");
		refresh();
		return;
	}
	LIST_FOREACH(node, &cgpa_head, list) {
		fprintf(cgpa, "%s\t%s\t%f\n", node->name, node->dept, node->marks);
		LIST_REMOVE(node, list);
		free(node);
	}
	fclose(cgpa);
}

/**
 * Always verify if student is valid to apply
 * returns 1 if valid student
 * else returns 0
 */
static int valid_student(char *name)
{
	struct cgpa *node;
	int valid = 0;

	prepare_cgpa_list();
	LIST_FOREACH(node, &cgpa_head, list) {
		if (!strcmp(node->name, name)) {
			if (node->marks >= 8.0)
				valid = 1;
			break;
		}
	}
	dump_cgpa_list();
	return valid;
}

/**
 * this function adds cgpa of any student into table
 */
static void add_cgpa(void)
{
	FILE *cgpa;
	struct cgpa *cnode, *node, *pnode;
	char ch = '\0';
	int count, inserted = 0;

	prepare_cgpa_list();
	while(ch != 'X' && ch != 'x') {
		clear();
		cnode = malloc(sizeof(*cnode));
		printw("ID:");
		refresh();
		scanw("%s", cnode->name);
		printw("Dept:");
		refresh();
		scanw("%s", cnode->dept);
		printw("Marks:");
		refresh();
		scanw("%f", &cnode->marks);
		count = 0;
		LIST_FOREACH(node, &cgpa_head, list) {
			if (node->marks < cnode->marks) {
				if (!count)
					LIST_INSERT_HEAD(&cgpa_head, cnode, list);
				else
					LIST_INSERT_AFTER(pnode, cnode, list);
				inserted = 1;
				break;
			}
			pnode = node;
			count++;
		}
		if (!inserted)
			LIST_INSERT_AFTER(pnode, cnode, list);
		clear();
		printw("X for exit from add cgpa\n");
		refresh();
		scanw("%c", &ch);
	}
	dump_cgpa_list();
}

static void prepare_vacancy_table()
{
	FILE *vac;
	int size, i;
	char *file, *sdept, *smax, *svac, *cfile, *ssvac;

	vac = fopen("vacancy.txt", "r");
	if (!vac) {
		printw("Could not find vacancy info\n");
		refresh();
		return;
	}
	/* get file size */
	fseek(vac, 0, SEEK_END);
	size = ftell(vac);
	fseek(vac, 0, SEEK_SET);
	/*alocate memory for complete file size */
	file = malloc(size);
	fread(file, sizeof(char), size, vac);
	fclose(vac);
	cfile = file;
	for (i = 0; i < 10; i++) {
		sdept = strsep(&cfile, "\t");
		if (!sdept)
			abort();
		smax = strsep(&cfile, "\t");
		if (!smax)
			abort();
		svac = strsep(&cfile, "\t");
		if (!svac)
			abort();
		ssvac = strsep(&cfile, "\n");
		if (!ssvac)
			abort();
		strcpy(vacancy[i].dept, sdept);
		vacancy[i].max = atoi(smax);
		vacancy[i].vacant = atoi(svac);
		vacancy[i].special_vacant = atoi(ssvac);
	}
	free(file);
}

static void dump_vacancy_table()
{
	FILE *vac;
	int i;

	vac = fopen("vacancy.txt", "w");
	if (!vac) {
		printw("Could not open vacancy file for writing\n");
		refresh();
		return;
	}
	for (i = 0; i < 10 ; i++) {
		fprintf(vac, "%s\t%d\t%d\t%d\n", vacancy[i].dept,
				vacancy[i].max, vacancy[i].vacant,
				vacancy[i].special_vacant);
	}
	fclose(vac);
}

/**
 * variable to indiacte that no more choice entry is possible now
 */

static int entry_locked = 0;

/**
 * function to add choice by a student
 */
static void add_choice(char *name)
{
	struct choice *choice;
	int ch = 0, i, j;

	if (entry_locked) {
		clear();
		printw("choice entry has been locked\n");
		refresh();
		getch();
		return;
	}

	LIST_FOREACH(choice, &choice_head, list) {
		if (!strcmp(choice->name, name)) {
			clear();
			printw("You have already added your choice\n");
			refresh();
			getch();
			return;
		}
	}
	prepare_vacancy_table();
	choice = malloc(sizeof(*choice));
	strcpy(choice->name, name);
	for (j = 0; j < 11; j++) {
		ch = 0;
		while (ch < 1 || ch > 10) {
			clear();
			for (i = 0; i < 10; i++)
				printw("%d\t%s\n", i+1, vacancy[i].dept);
			printw("%d Choice:", j + 1);
			refresh();
			scanw("%d", &ch);
		}
		strcpy(choice->choice[j], vacancy[ch-1].dept);
	}

	LIST_INSERT_HEAD(&choice_head, choice, list);
}
/**
 * allows admin to lock choice entry
 */

static void lock_apply(void)
{
	entry_locked = 1;
}

/**
 * Vacancy to be added by admin.
 * Has been kept in todo list.
 */
static void add_vacancy(void)
{
	/*TODO*/
}
static int dept_to_id(char *dept)
{
	if (!strcmp(dept, "DOA"))
		return 1;
	else if (!strcmp(dept, "DOB"))
		return 2;
	else if (!strcmp(dept, "DOC"))
		return 3;
	else if (!strcmp(dept, "DOD"))
		return 4;
	else if (!strcmp(dept, "DOE"))
		return 5;
	else if (!strcmp(dept, "DOF"))
		return 6;
	else if (!strcmp(dept, "DOG"))
		return 7;
	else if (!strcmp(dept, "DOH"))
		return 8;
	else if (!strcmp(dept, "DOI"))
		return 9;
	else if (!strcmp(dept, "DOJ"))
		return 10;
	return 0;
}
static int sheet_allocated = 0;
/**
 * function to allocate sheet by admin
 * still lot of modification is needed in this function.
 */
static void allocate_sheet(void)
{
	struct cgpa *cgpa;
	struct choice *choice;
	int c_id, id, i, choice_entered, round;

	lock_apply();
	for (round = 0; round < 3; round++) {
		prepare_vacancy_table();
		prepare_cgpa_list();
		LIST_FOREACH(cgpa, &cgpa_head, list) {
			if (round == 0 && cgpa->marks < 9.0)
				break;
			else if (round == 1 && cgpa->marks < 8.5)
				break;
			else if (round == 2 && cgpa->marks < 8.0)
				break;
			choice_entered = 0;
			LIST_FOREACH(choice, &choice_head, list) {
				if (!strcmp(cgpa->name, choice->name)) {
					choice_entered = 1;
					break;
				}
			}
			if (!choice_entered)
				continue;
			c_id = dept_to_id(cgpa->dept);
			for (i = 0; i < 10; i++) {
				id = dept_to_id(choice->choice[i]);
				if (vacancy[id - 1].vacant > 0) {
					vacancy[id - 1].vacant--;
					vacancy[c_id - 1].vacant++;
					strcpy(cgpa->dept, vacancy[id - 1].dept);
					break;
				} else if (cgpa->marks >= 9.0) {
					if (vacancy[id - 1].special_vacant > 0) {
						vacancy[id - 1].special_vacant--;
						vacancy[c_id - 1].special_vacant++;
						strcpy(cgpa->dept, vacancy[id - 1].dept);
						break;
					}
				}
			}
		}
		dump_cgpa_list();
		dump_vacancy_table();
	}
	sheet_allocated = 1;
}

/**
 * dispalys new department list
 */
static void display_allocation(void)
{
	if (sheet_allocated)
		print_cgpa();
}

/**
 * Main function of program reads input file and then implement the
 * algorithm described in design document.
 */
int main(int argc, char** argv)
{
	int err = 0;
	char menu;
	char name[MAX_NAME_LEN];
	char ch = '\0';

	initscr();
	cbreak();
	keypad(stdscr, TRUE);

	LIST_INIT(&cgpa_head);
	LIST_INIT(&choice_head);

	while (1) {
		login(name);
		while(ch != 'X' && ch != 'x') {
			if(!strcmp(name, "admin"))
				menu = get_admin_menu();
			else {
				if (valid_student(name))
					menu = get_student_menu();
			}

			switch(menu) {
			case 'A':
			case 'a':
				print_cgpa();
				break;
			case 'B':
			case 'b':
				add_cgpa();
				break;
			case 'C':
			case 'c':
				print_vacancy();
				break;
			case 'd':
			case 'D':
				add_vacancy();
				break;
			case 'E':
			case 'e':
				allocate_sheet();
				break;
			case 'F':
			case 'f':
				display_allocation();
				break;
			case 'G':
			case 'g':
				lock_apply();
				break;
			case 'H':
			case 'h':
				add_choice(name);
				break;
			}
			clear();
			printw("X for logout\n");
			refresh();
			scanw("%c", &ch);
		}
		ch = '\0';
	}
	getch();
	endwin();
	return err;
}
