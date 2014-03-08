/* linux.c basic wait3 for linux Copyright Jon Tombs 4/4/92 */
/* If gcc2.x contains wait3 this may not be needed */


#include <sys/time.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <sys/types.h>

pid_t
wait3(int *status, int opts, struct rusage *rusage)
{
   pid_t child_pid;
   
   child_pid = wait(status);
   getrusage(RUSAGE_CHILDREN, rusage);
   
   return child_pid;
}

int
getpagesize(void)
{
   return 4096;  /* is this right? Maybe I'll check one day */
}
