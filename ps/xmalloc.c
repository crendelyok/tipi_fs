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
#define ERR_ALIGNED_ALLOC -2
void* xmalloc(size_t size) {
	void* ret = malloc(size);
	BUG_ON(ret == NULL, ERR_XMALLOC, "xmalloc failed\n");
	return ret;
}

void* aligned_xmalloc(size_t al, size_t size) {
	void* ret = aligned_alloc(al, size);
	BUG_ON(ret == NULL, ERR_ALIGNED_ALLOC, "aligned_xmalloc failed\n");
	return ret;
}
