#include <sys/prctl.h>
#include <unistd.h>
#include <stdio.h>
int main(int argc, char* argv[], char* envp[])
{
	char* new_argv [argc-1];
	for (int i = 0; i < argc-1; ++i) {
		new_argv[i] = argv[i+1];
		printf("%s\n", new_argv[i]);
	}
	prctl(PR_SET_MM, PR_SET_MM_ARG_START, new_argv, 0, 0);
	while(1){sleep(1);};	
	
	return 0;
}
