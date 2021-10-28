#include <stdlib.h>
#include <stdio.h>

#define BUG_ON(cond, exit_stat, str)              \
do {                                   \
	if (cond) {                    \
		fprintf(stderr, str);  \
		exit(exit_stat);     \
	}                              \
} while(0)

#define ERR_XMALLOC -1
void* xmalloc(size_t size) {
	void* ret = malloc(size);
	BUG_ON(ret == NULL, ERR_XMALLOC, "xmalloc failed\n");
	return ret;
}
