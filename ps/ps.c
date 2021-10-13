#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

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

#define TRUE  1
#define FALSE 0

#define DEFAULT_SIZE 1024
#define STAT_SIZE    16384    

const char stat_format_string[DEFAULT_SIZE] = "%*d %*s %*c %*d %*d %*d %*d %*d %*u %*u %*u %*u %*u %*u %*u %*d %*d %*d %*d %*d %*d %*u %lu %ld"; 

int isnumber(char* str)
{
	if (str == NULL)
		exit(ERR_NULL_POINTER);
	for (int i = 0; i < strlen(str); ++i)
		if (str[i] < '0' || str[i] > '9')
			return FALSE;
	return TRUE;	
}

int show_ps(DIR* dirp)
{
	if (dirp == NULL)
		exit(ERR_NULL_POINTER);

	struct dirent* dirsp = readdir(dirp);
	if (dirsp == NULL)
		return FALSE;

	//get pid
	if (isnumber(dirsp->d_name) == FALSE)
		return TRUE;
	
	//get cmd
	char* cmd_pathname = calloc(DEFAULT_SIZE, sizeof(char));
	if (cmd_pathname == NULL)
		exit(ERR_CALLOC);
	if ((cmd_pathname = strcat(cmd_pathname, "/proc/")) == NULL)
		exit(ERR_STRCAT);
	if ((cmd_pathname = strcat(cmd_pathname, dirsp->d_name)) == NULL)
		exit(ERR_STRCAT);
	if ((cmd_pathname = strcat(cmd_pathname, "/comm")) == NULL)
		exit(ERR_STRCAT);

	int cmd_fd = -1;
	if ((cmd_fd = open(cmd_pathname, O_RDONLY)) == -1)
		exit(ERR_OPEN);
	
	char cmd[DEFAULT_SIZE] = "";
	if (read(cmd_fd, cmd, DEFAULT_SIZE-1) == -1)
		exit(ERR_READ);
	close(cmd_fd);
	
	//get vsize and rss
	unsigned long vsize = 0;
	long rss = 0;
	char* vsize_pathname = calloc(DEFAULT_SIZE, sizeof(char));
	if (vsize_pathname == NULL)
		exit(ERR_CALLOC);
	if ((vsize_pathname = strcat(vsize_pathname, "/proc/")) == NULL)
		exit(ERR_STRCAT);
	if ((vsize_pathname = strcat(vsize_pathname, dirsp->d_name)) == NULL)
		exit(ERR_STRCAT);
	if ((vsize_pathname = strcat(vsize_pathname, "/stat")) == NULL)
		exit(ERR_STRCAT);

	int stat_fd = -1;
	if ((stat_fd = open(vsize_pathname, O_RDONLY)) == -1)
		exit(ERR_OPEN);

	char stat[STAT_SIZE] = "";
	if (read(stat_fd, stat, STAT_SIZE-1) == -1)
		exit(ERR_READ);
	close(stat_fd);

	if (sscanf(stat, stat_format_string, &vsize, &rss) == EOF)
		exit(ERR_SSCANF);
	
	
	if (printf("%s %lu %ld %s", dirsp->d_name, rss, vsize, cmd) < 0)
		exit(ERR_PRINTF);

	free(cmd_pathname);
	free(vsize_pathname);
	return TRUE;
}


int main()
{
	/* open directory stream and fd of /proc */
	DIR* dirp = opendir("/proc");
	if (dirp == NULL)
		return ERR_OPENDIR;

	
	int proc_fd = dirfd(dirp); // dirfd is probably not std=c99 standard
	if (proc_fd == -1)
		return ERR_DIRFD;
	

	if (printf("PID RSS  VSIZE(bytes)    CMD\n") < 0)
		return ERR_PRINTF;
	while(show_ps(dirp)) {}

	/* close directory stream of /proc */
	if (closedir(dirp) == -1)
		return ERR_CLOSEDIR;

	return SUCCESS;
}
