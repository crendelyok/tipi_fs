#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include "xmalloc.h"

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
#define ERR_SPRINTF      -11
#define ERR_READLINK     -12
#define ERR_DIRSP_DNAME  -13
#define ERR_READDIR      -14
#define ERR_OPEN_CMD     -15
#define ERR_READ_CMD     -16

#define TRUE  1
#define FALSE 0

#define DEFAULT_SIZE 1024
#define STAT_SIZE    16384    

int isnumber(char* str)
{
	for (int i = 0; i < strlen(str); ++i)
		if (str[i] < '0' || str[i] > '9')
			return FALSE;
	return TRUE;	
}

int show_lsof(DIR* dirp)
{

	errno = 0;
	struct dirent* dirsp = readdir(dirp);
	if (dirsp == NULL) {
		if (errno != 0)
			return ERR_READDIR;
		return FALSE;
	}

	if (dirsp->d_name == NULL)
		return ERR_DIRSP_DNAME;
	if (isnumber(dirsp->d_name) == FALSE)
		return TRUE;
	
	//get cmd
	char* cmd_pathname = xmalloc(DEFAULT_SIZE * sizeof(char));
	if (sprintf(cmd_pathname, "/proc/%s/comm", dirsp->d_name) < 0)
		return ERR_SPRINTF;
	int cmd_fd = open(cmd_pathname, O_RDONLY);
	if (cmd_fd == -1)
		return ERR_OPEN_CMD;

	char cmd[DEFAULT_SIZE] = "";
	if (read(cmd_fd, cmd, DEFAULT_SIZE-1) == -1)
		return ERR_READ_CMD;
	if (cmd[strlen(cmd)-1] == '\n')
		cmd[strlen(cmd)-1] = '\0';
	close(cmd_fd);
	free(cmd_pathname);

	//get files
	char* fd_pathname = xmalloc(DEFAULT_SIZE * sizeof(char));
	if (sprintf(fd_pathname, "/proc/%s/fd", dirsp->d_name) < 0) {
		return ERR_SPRINTF;
	}
	DIR* fd_fd = NULL;
	if ((fd_fd = opendir(fd_pathname)) == NULL) {
		free(fd_pathname);
		return TRUE; 
	}
	
	struct dirent *entry = NULL;
	while ((entry = readdir(fd_fd)) != NULL) {
		if (entry->d_name[0] == '.'){
			//printf("name: %s\n", entry->d_name);
			continue;
		}
		char* buf = xmalloc(DEFAULT_SIZE * sizeof(char));
		unsigned int bufsize = DEFAULT_SIZE;
	
		char* name = xmalloc(DEFAULT_SIZE * sizeof(char));

		if (sprintf(name, "/proc/%s/fd/%s", dirsp->d_name, entry->d_name) < 0)
			return ERR_SPRINTF;
		if (readlink(name, buf, bufsize) == -1)
			return ERR_READLINK;
		printf("%s %s %s\n", dirsp->d_name, cmd, buf);
		free(name);
		free(buf);
	}

	free(fd_pathname);
	if (closedir(fd_fd) == -1)
		return ERR_CLOSEDIR;
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

	if (printf("PID CMD FILE\n") < 0)
		return ERR_PRINTF;

	int ret_code = TRUE;
	while((ret_code = show_lsof(dirp)) == TRUE) {}
	if (ret_code == ERR_READDIR)
                printf("in show_lsof(dirp):\nreaddir(dirp) returned errno: %i (%s)\n", errno, strerror(errno));

        if (ret_code == ERR_DIRSP_DNAME)
                printf("in show_lsof(dirp):\nFound NULL name inside \"/proc\": %i (%s)\n", errno, strerror(errno));

        if (ret_code == ERR_SPRINTF)
                printf("in show_lsof(dirp):\nError sprintf: %i (%s)\n", errno, strerror(errno));

        if (ret_code == ERR_OPEN_CMD)
                printf("in show_lsof(dirp):\nError open(cmd_pathname): %i (%s)\n", errno, strerror(errno));

        if (ret_code == ERR_READ_CMD)
                printf("in show_lsof(dirp):\nError read(cmd_fd, cmd_pathname, DEFAULT_SIZE - 1): %i (%s)\n", errno, strerror(errno));

        if (ret_code == ERR_SSCANF)
                printf("in show_lsof(dirp):\nError sscanf(stat, ...): %i (%s)\n", errno, strerror(errno));

        if (ret_code == ERR_PRINTF)
                printf("in show_lsof(dirp):\nError printf: %i (%s)\n", errno, strerror(errno));

	if (ret_code == ERR_READLINK)
                printf("in show_lsof(dirp):\nError realink: %i (%s)\n", errno, strerror(errno));
	
	if (ret_code == ERR_CLOSEDIR)
                printf("in show_lsof(dirp):\nError closedir: %i (%s)\n", errno, strerror(errno));
		
	if (ret_code == FALSE)
		ret_code = SUCCESS;

	/* close directory stream of /proc */
	if (closedir(dirp) == -1)
		return ERR_CLOSEDIR;

	return ret_code;
}
