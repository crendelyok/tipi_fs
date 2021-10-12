#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/types.h>
#include <string.h>

#define SUCCESS       0
#define ERR_OPENDIR  -1
#define ERR_DIRFD    -2
#define ERR_CLOSEDIR -3
#define ERR_NULL_POINTER -4



#define TRUE  1
#define FALSE 0

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

	// printf("%d  ", isnumber(dirsp->d_name));
	// printf("%s\n", dirsp->d_name); 
	if (isnumber(dirsp->d_name) == FALSE)
		return TRUE;
	
			
	printf("%s %s %s %s", dirsp->name, rss, vsize, cmd);
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
	

	printf(" PID RSS  VSIZE    CMD\n");
	while(show_ps(dirp)) {}

	/* close directory stream of /proc */
	if (closedir(dirp) == -1)
		return ERR_CLOSEDIR;

	return SUCCESS;
}
