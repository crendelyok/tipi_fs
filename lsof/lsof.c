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
#define ERR_SPRINTF      -11
#define ERR_READLINK     -12

#define TRUE  1
#define FALSE 0

#define DEFAULT_SIZE 1024
#define STAT_SIZE    16384    

int isnumber(char* str)
{
	if (str == NULL)
		exit(ERR_NULL_POINTER);
	if (strlen(str) <= 0)
		return FALSE;
	for (int i = 0; i < strlen(str); ++i)
		if (str[i] < '0' || str[i] > '9')
			return FALSE;
	return TRUE;	
}

int show_lsof(DIR* dirp)
{
	if (dirp == NULL)
		exit(ERR_NULL_POINTER);

	struct dirent* dirsp = readdir(dirp);
	if (dirsp == NULL)
		return FALSE;

	if (isnumber(dirsp->d_name) == FALSE)
		return TRUE;
	//get cmd
	char* cmd_pathname = calloc(DEFAULT_SIZE, sizeof(char));
	if (cmd_pathname == NULL)
		exit(ERR_CALLOC);
	if (sprintf(cmd_pathname, "/proc/%s/comm", dirsp->d_name) < 0)
		exit(ERR_SPRINTF);
	int cmd_fd = open(cmd_pathname, O_RDONLY);
	if (cmd_fd == -1)
		exit(ERR_OPEN);
	char cmd[DEFAULT_SIZE] = "";
	if (read(cmd_fd, cmd, DEFAULT_SIZE-1) == -1)
		exit(ERR_READ);
	if (cmd[strlen(cmd)-1] == '\n')
		cmd[strlen(cmd)-1] = '\0';
	close(cmd_fd);
	free(cmd_pathname);

	//get files
	char* fd_pathname = calloc(DEFAULT_SIZE, sizeof(char));
	if (fd_pathname == NULL)
		exit(ERR_CALLOC);
	if (sprintf(fd_pathname, "/proc/%s/fd", dirsp->d_name) < 0) {
		exit(ERR_SPRINTF);
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
		char* buf = calloc(DEFAULT_SIZE, sizeof(char));
		if (buf == NULL)
			exit(ERR_CALLOC);
		unsigned int bufsize = DEFAULT_SIZE;
	
		char* name = calloc(DEFAULT_SIZE, sizeof(char));
		if (name == NULL)
			exit(ERR_CALLOC);

		if (sprintf(name, "/proc/%s/fd/%s", dirsp->d_name, entry->d_name) < 0)
			exit(ERR_SPRINTF);
		//printf("%s\n", name);
		if (readlink(name, buf, bufsize) == -1)
			exit(ERR_READLINK);
		//printf("%s\n", buf);
		printf("%s %s %s\n", dirsp->d_name, cmd, buf);
		free(name);
		free(buf);
	}

	free(fd_pathname);
	if (closedir(fd_fd) == -1)
		exit(ERR_CLOSEDIR);
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
	while(show_lsof(dirp)) {}

	/* close directory stream of /proc */
	if (closedir(dirp) == -1)
		return ERR_CLOSEDIR;

	return SUCCESS;
}
