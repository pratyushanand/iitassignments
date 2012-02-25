#ifndef _HELPER_H
#define _HELPER_H
#include <stdio.h>
/* Common helper definitions */
#define MAX_NAME_LEN		30
#define pr_info(args...) fprintf(stdout, ##args)
#define pr_dbg(args...) fprintf(stdout, ##args)
#define pr_err(args...) fprintf(stdout, ##args)
#endif


