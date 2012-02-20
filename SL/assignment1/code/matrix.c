#include <errno.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string.h>

#define pr_info(args...) fprintf(stdout, ##args)
#define pr_err(args...) fprintf(stdout, ##args)
#define NOMEM "No memory available\n"
#define OPINVAL "Not a valid operation\n"
#define MAX_CHAR_NUM 8

/* enum for permitted operations */
enum operation {
	ADDITION,
	SUBTRACTION,
	MULTIPLICATION,
	INVALID,
};
/* Helper function for user */
void help_matrix (char *prog_name)
{
	pr_info ("Usage: %s options [ inputfile ... ]\n", prog_name);
	pr_info (" -h	--help		Display this usage information.\n"
		" -o	--operation	Matrix operation to be permitted (add, mult, sub)\n"
		" -i	--row1		Number of row in first matrix(should be integer).\n"
		" -j	--col1	Number of column in first matrix(should be integer).\n"
		" -k	--row2		Number of row in second matrix(should be integer).\n"
		" -l	--col2	Number of column in second matrix(should be integer).\n"
		" -v	--values	All float values for input matrix separated by comma(,).\n"
		"input must be row wise for first and second matrix respectively.\n"
		"for correct operation , options must be in order of o, i, j, k, l, v.\n");
	return;
}
/* function for matrix multiplication.
 * m1 : Pointer to first input matrix
 * m2 : Pointer to second input matrix
 * m3 : Pointer to output matrix
 * row1 : row size of first matrix
 * col1 : column size of first matrix
 * col2 : column size of second matrix
 * It expects that row size of second matrix is same as column size of
 * first matrix.
 */

void matrix_multiplication(float *m1, float *m2, float *m3, int row1, int col1, int col2)
{
	int t_row, t_col, count, sum;

	for (t_row = 0; t_row < row1; t_row++) {
		for (t_col = 0; t_col < col2; t_col++) {
			sum = 0;
			for (count = 0; count < col1; count++) {
				sum += m1[t_row * col1 + count] * m2[count * col2 + t_col];
			}
			m3[t_row * col2 + t_col] = sum;
		}
	}
}

/* function for matrix addition.
 * m1 : Pointer to first input matrix
 * m2 : Pointer to second input matrix
 * m3 : Pointer to output matrix
 * row1 : row size of first matrix
 * col1 : column size of first matrix
 * It expects that row and column size of second matrix is same as that of row
 * and column size of first matrix.
 */
void matrix_addition(float *m1, float *m2, float *m3, int row, int col)
{
	int t_row, t_col;

	for (t_row = 0; t_row < row; t_row++) {
		for (t_col = 0; t_col < col; t_col++) {
			m3[t_row * col + t_col] =
				m1[t_row * col + t_col] +
				m2[t_row * col + t_col];
		}
	}
}

/* function for matrix subtraction.
 * m1 : Pointer to first input matrix
 * m2 : Pointer to second input matrix
 * m3 : Pointer to output matrix
 * row1 : row size of first matrix
 * col1 : column size of first matrix
 * It expects that row and column size of second matrix is same as that of row
 * and column size of first matrix.
 * it will perform m1-m2
 */
void matrix_subtraction(float *m1, float *m2, float *m3, int row, int col)
{
	int t_row, t_col;

	for (t_row = 0; t_row < row; t_row++) {
		for (t_col = 0; t_col < col; t_col++) {
			m3[t_row * col + t_col] =
				m1[t_row * col + t_col] -
				m2[t_row * col + t_col];
		}
	}
}

/*
 * helper function for matrix print on console
 * m3: pointer for the matrix to be printed
 * row: row size of matrix to be printed
 * col: col size of matrix to be printed
 */
void print_matrix(float *m3, int row, int col)
{
	int t_row, t_col;

	for (t_row = 0; t_row < row; t_row++) {
		for (t_col = 0; t_col < col; t_col++) {
			pr_info("%f\t", m3[t_row * col + t_col]);
		}
		pr_info("\n");
	}

}

/*
 * main routine.
 * receives input from user and calls subroutine accordingly.
 */

int main(int argc, char** argv)
{

	int next_option;
	const char* const short_options = "ho:i:j:k:l:v:";
	enum operation op = INVALID;
	int row1, col1, row2, col2, err = 0;
	float *m1 = NULL, *m2 = NULL, *m3 = NULL;
	const struct option long_options[] = {
		{ "help",     0, NULL, 'h' },
		{ "operation",   1, NULL, 'o' },
		{ "row1",   1, NULL, 'i' },
		{ "col1",   1, NULL, 'j' },
		{ "row2",   1, NULL, 'k' },
		{ "col2",   1, NULL, 'l' },
		{ "values",   1, NULL, 'v' },
		{ NULL,       0, NULL, 0   }
	};
	char number[MAX_CHAR_NUM];
	int i, j, count = 0;

	/* get all user input in local variables */
	do {
		next_option = getopt_long (argc, argv, short_options,
				long_options, NULL);
		switch (next_option) {
			case 'h':
				help_matrix(argv[0]);
				return 0;
			case 'o':
				if (!strcmp(optarg, "add"))
					op = ADDITION;
				else if (!strcmp(optarg, "sub"))
					op = SUBTRACTION;
				else if(!strcmp(optarg, "mult"))
					op = MULTIPLICATION;
				else {
					pr_err("Operation is not allowed. Please look for help\n");
					err = -EINVAL;
					goto free_m2;
				}
				break;
			case 'i':
				row1 = atoi(optarg);
				if (!row1) {
					pr_err("Incorrect row number for first matrix. Please look for help\n");
					err = -EINVAL;
					goto free_m2;
				}
				break;
			case 'j':
				col1 = atoi(optarg);
				if (!col1) {
					pr_err("Incorrect column number for first matrix. Please look for help\n");
					err = -EINVAL;
					goto free_m2;
				}
				break;
			case 'k':
				row2 = atoi(optarg);
				if (!row2) {
					pr_err("Incorrect row number for second matrix. Please look for help\n");
					err = -EINVAL;
					goto free_m2;
				}
				break;
			case 'l':
				col2 = atoi(optarg);
				if (!col2) {
					pr_err("Incorrect column number for second matrix. Please look for help\n");
					err = -EINVAL;
					goto free_m3;
				}
				break;

			case 'v':
				if (op == INVALID) {
					pr_err("Please provide input in order, see help\n");
					return -EINVAL;
				}else if (op == MULTIPLICATION) {
					if (row2 != col1) {
						pr_err(OPINVAL);
						return -EINVAL;
					}
					m3 = calloc(row1 * col2, sizeof(float));
				}
				else {
					if (row1 != row2 || col1 != col2) {
						pr_err(OPINVAL);
						return -EINVAL;
					}
					m3 = calloc(row1 * col1, sizeof(float));
				}
				if (!m3) {
					pr_err(NOMEM);
					return -ENOMEM;
				}

				m1 = calloc(row1 * col1, sizeof(float));
				if (!m1) {
					pr_err(NOMEM);
					err = -ENOMEM;
					goto free_m3;
				}
				m2 = calloc(row2 * col2, sizeof(float));
				if (!m2) {
					pr_err(NOMEM);
					err = -ENOMEM;
					goto free_m1;
				}
				i = 0;
				count = 0;
				do {
					for (j = 0; j < MAX_CHAR_NUM; j++) {
						if ((optarg[i] >= '0' && optarg[i] <= '9')
								|| (optarg[i] == '.'))
							number[j] = optarg[i];
						else {
							number[j] = '\0';
							i++;
							break;
						}
						i++;
					}
					if (j == MAX_CHAR_NUM) {
						pr_err("Too long input value\n");
						err = -EINVAL;
						goto free_m2;
					}
					if (count < row1 * col1)
						m1[count] = atof(number);
					else if (count < row1 * col1 + row2 * col2)
						m2[count - row1 * col1] = atof(number);
					count++;
				} while(optarg[i-1] != '\0');
			case -1:
				break;
			default:
				abort();
		}
	} while (next_option != -1);

	switch(op) {
	case ADDITION:
		matrix_addition(m1, m2, m3, row1, col1);
		print_matrix(m3, row1, col1);
		break;
	case SUBTRACTION:
		matrix_subtraction(m1, m2, m3, row1, col1);
		print_matrix(m3, row1, col1);
		break;
	case MULTIPLICATION:
		matrix_multiplication(m1, m2, m3, row1, col1, col2);
		print_matrix(m3, row1, col2);
		break;
	default:
		abort();
	}
free_m2:
	if (m2)
		free(m2);
free_m1:
	if (m1)
		free(m1);
free_m3:
	if (m3)
		free(m3);
	return err;
}
