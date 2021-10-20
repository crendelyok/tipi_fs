#include <sys/prctl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>

#define PAGE_SHIFT 12
#define PAGE_SIZE  (1UL << PAGE_SHIFT)
#define PAGE_MASK  (~(PAGE_SIZE-1))
#define PAGE_ALIGN(addr) (((addr)+PAGE_SIZE-1)&PAGE_MASK)

int main(int argc, char* argv[], char* envp[])
{
	long l = 0;
	for (int i = 1; i < argc; ++i)
		l += strlen(argv[i]);
		
	size_t nn_size = PAGE_ALIGN(l+1);
	char* nn = mmap(NULL, nn_size, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);

	unsigned long offset = 0;
	for (int i = 1; i < argc; ++i) {
		strncpy(nn + offset, argv[i], strlen(argv[i]));
		offset += strlen(argv[i]);
	}
	if (prctl(PR_SET_MM, PR_SET_MM_ARG_START, (unsigned long) nn, 0, 0) == -1) {
		printf("1 prctl err");
		return -1;
	}
	int i = 0;
	while(1){i++;};	
	
	return 0;
}
