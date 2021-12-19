#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include "xmalloc.h"
#include <errno.h>

#define SUCCESS           0
#define ERR_OPENDIR      -1
#define ERR_DIRFD        -2
#define ERR_CLOSEDIR     -3
#define ERR_NULL_POINTER -4
#define ERR_READ         -5
#define ERR_OPEN         -6
#define ERR_CALLOC       -7
#define ERR_STRCAT       -8
#define ERR_PRINTF       -9
#define ERR_SSCANF       -10
#define ERR_READDIR      -11
#define ERR_SPRINTF      -12
#define ERR_OPEN_CMD     -13
#define ERR_OPEN_STAT    -14
#define ERR_READ_CMD     -15
#define ERR_READ_STAT    -16
#define ERR_DIRSP_DNAME  -17


#define TRUE  1
#define FALSE 0

#define DEFAULT_SIZE 1024
#define STAT_SIZE    16384    

const char stat_format_string[DEFAULT_SIZE] = "%*d %*s %*c %*d %*d %*d %*d %*d %*u %*u %*u %*u %*u %*u %*u %*d %*d %*d %*d %*d %*d %*u %lu %ld"; 

int isnumber(char* str)
{
	//let them die:)
	//if (str == NULL)
	//	my_errno = ERR_NULL_POINTER;
	for (int i = 0; i < strlen(str); ++i)
		if (str[i] < '0' || str[i] > '9')
			return FALSE;
	return TRUE;	
}

int show_ps(DIR* dirp)
{
	errno = 0; // to detect if readdir return NULL by error
	struct dirent* dirsp = readdir(dirp);
	if (dirsp == NULL) { 
		if (errno != 0)
			return ERR_READDIR;
		return FALSE;
	}

	//get pid
	if (dirsp->d_name == NULL) 
		return ERR_DIRSP_DNAME;
	if (isnumber(dirsp->d_name) == FALSE)
		return TRUE; //Means we skip this dir
	
	//get cmd
	char* cmd_pathname = xmalloc(DEFAULT_SIZE * sizeof(char));
	if (sprintf(cmd_pathname, "/proc/%s/comm", dirsp->d_name) < 0)
		return ERR_SPRINTF;

	int cmd_fd = -1;
	if ((cmd_fd = open(cmd_pathname, O_RDONLY)) == -1)
		return ERR_OPEN_CMD;
	
	char cmd[DEFAULT_SIZE] = "";
	if (read(cmd_fd, cmd, DEFAULT_SIZE-1) == -1) 
		return ERR_READ_CMD;
	close(cmd_fd);
	
	//get vsize and rss
	unsigned long vsize = 0;
	long rss = 0;
	char* vsize_pathname = xmalloc(DEFAULT_SIZE * sizeof(char));
	if (sprintf(vsize_pathname, "/proc/%s/stat", dirsp->d_name) < 0)
		return ERR_SPRINTF;

	int stat_fd = -1;
	if ((stat_fd = open(vsize_pathname, O_RDONLY)) == -1)
		return ERR_OPEN_STAT;

	char stat[STAT_SIZE] = "";
	if (read(stat_fd, stat, STAT_SIZE-1) == -1) 
		return ERR_READ_STAT;

	close(stat_fd);

	if (sscanf(stat, stat_format_string, &vsize, &rss) == EOF) 
		return ERR_SSCANF;
		
	if (printf("%s %lu %ld %s", dirsp->d_name, rss, vsize, cmd) < 0)
		return ERR_PRINTF;

	free(cmd_pathname);
	free(vsize_pathname);
	return TRUE;
}


int main()
{
	/* open directory stream and fd of /proc */
	DIR* dirp = opendir("/proc");
	if (dirp == NULL) {
		printf("can`t opendir(\"/proc\"): %i (%s)\n", errno, strerror(errno));
		return ERR_OPENDIR;
	}
	
	int proc_fd = dirfd(dirp); // dirfd is probably not std=c99 standard
	if (proc_fd == -1) {
		printf("can`t dirfd(\"/proc\"): %i (%s)\n", errno, strerror(errno));
		return ERR_DIRFD;
	}

	if (printf("PID RSS  VSIZE(bytes)    CMD\n") < 0)
		return ERR_PRINTF;

	int ret_code = TRUE;
	while((ret_code = show_ps(dirp)) == TRUE) {}
	if (ret_code == ERR_READDIR) 
		printf("in show_ps(dirp):\nreaddir(dirp) returned errno: %i (%s)\n", errno, strerror(errno));

	if (ret_code == ERR_DIRSP_DNAME)
		printf("in show_ps(dirp):\nFound NULL name inside \"/proc\": %i (%s)\n", errno, strerror(errno));
	
	if (ret_code == ERR_SPRINTF)
		printf("in show_ps(dirp):\nError sprintf: %i (%s)\n", errno, strerror(errno));
	
	if (ret_code == ERR_OPEN_CMD)
		printf("in show_ps(dirp):\nError open(cmd_pathname): %i (%s)\n", errno, strerror(errno));
	
	if (ret_code == ERR_READ_CMD) 
		printf("in show_ps(dirp):\nError read(cmd_fd, cmd_pathname, DEFAULT_SIZE - 1): %i (%s)\n", errno, strerror(errno));
		
	if (ret_code == ERR_OPEN_STAT)
		printf("in show_ps(dirp):\nError open(vsize_pathname): %i (%s)\n", errno, strerror(errno));

	if (ret_code == ERR_READ_STAT)
		printf("in show_ps(dirp):\nError read(stat_fd, vsize_pathname, STAT_SIZE - 1): %i (%s)\n", errno, strerror(errno));

	if (ret_code == ERR_SSCANF)
		printf("in show_ps(dirp):\nError sscanf(stat, ...): %i (%s)\n", errno, strerror(errno));

	if (ret_code == ERR_PRINTF)
		printf("in show_ps(dirp):\nError printf: %i (%s)\n", errno, strerror(errno));

	if (ret_code == FALSE)
		ret_code = SUCCESS;

	/* close directory stream of /proc */
	if (closedir(dirp) == -1) {
		printf("can`t closedir \"/proc\": %i (%s)\n", errno, strerror(errno));
		return ERR_CLOSEDIR;
	}

	return ret_code;
}
