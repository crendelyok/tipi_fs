#include <liburing.h>
#include "xmalloc.h"
#include "log.h"
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

static int src_fd, dst_fd;


#define ENTRIES 4
#define _BLOCK_SIZE (256*1024)

typedef enum {
	STATUS_READ = 0,
	STATUS_WRITE = 1,
} rw_status;

struct io_data {
	rw_status status;
	off_t offset;
	size_t len;
	struct iovec iov;
};

static off_t get_f_size(int fd)
{
	struct stat st;
	if (fstat(fd, &st) < 0)
		return -errno;
	return st.st_size;
}

static int queue_read(struct io_uring* ring, off_t size, off_t offset)
{
	struct io_uring_sqe* sqe = io_uring_get_sqe(ring);
	if (sqe == NULL) 
		return -1;

	struct io_data* data = xmalloc(size + sizeof(*data));

	data->status = STATUS_READ;
        data->offset = offset;
	data->len = size;
	
	data->iov.iov_base = data + 1;
	data->iov.iov_len = size;

	io_uring_prep_readv(sqe, src_fd, &data->iov, 1, offset); 
	io_uring_sqe_set_data(sqe, data);

	return 0;
}

static int queue_write(struct io_uring* ring, struct io_data* data)
{
	data->status = STATUS_WRITE;
	//data->iov.iov_base = data + 1;
	//data->iov.iov_len = data->len;

	struct io_uring_sqe* sqe;
	sqe = io_uring_get_sqe(ring);
	if (sqe == NULL) {
		return -1;
	}
	io_uring_prep_writev(sqe, dst_fd, &data->iov, 1, data->offset);
	io_uring_sqe_set_data(sqe, data);

	return 0;
}

static int cp(struct io_uring* ring, off_t src_size) 
{
	struct io_uring_cqe* cqe;

	off_t offset;

	offset = 0;
	int writes_remain = 0;
	int entries_left = ENTRIES;
	int readn = 0;
	while(offset < src_size) {
		while(entries_left > 0) {
			off_t current_size = src_size - offset;
			if (current_size > _BLOCK_SIZE)
				current_size = _BLOCK_SIZE;
			if (!current_size)
				break;
			if (queue_read(ring, current_size, offset) < 0)
				break;
			offset += current_size;
			entries_left--;
			readn++;
		}

		io_uring_submit_and_wait(ring, 1);
		entries_left++;
		
		while(io_uring_peek_cqe(ring, &cqe) == 0) {
			struct io_data* data;
			data = io_uring_cqe_get_data(cqe);
			if (data == NULL)
				break;
			if (data->status == STATUS_READ) {
				if (queue_write(ring, data) < 0)
					break;
				readn--;
				entries_left--;
				writes_remain++;
			}
			else if (data->status == STATUS_WRITE) {		
				entries_left++;
				writes_remain--;
				free(data);
			}
			io_uring_cqe_seen(ring, cqe);
		}
	}
	
	// catch remaining writes
	while(writes_remain > 0) {
		struct io_data* data;
		io_uring_submit_and_wait(ring, 1);
		if (io_uring_peek_cqe(ring, &cqe) != 0)
			break;
		data = io_uring_cqe_get_data(cqe);
		if (data != NULL)
			free(data);
		writes_remain--;
	}

	return 0;
}

#define ERR -1
int main(int argc, char* argv[])
{
	if (argc != 3) {
		printf("cp: missing file operand\nexample: './cp <src> <dest>'\n");
		return ERR;
	}
	
	// Open <src> and <dst>
	if ((src_fd = open(argv[1], O_RDONLY)) < 0) {	
		printf("err: can`t open '%s' as <src>: %i (%s)", argv[1], errno, strerror(errno));
		return ERR;
	}
	if ((dst_fd = open(argv[2], O_WRONLY | O_CREAT | O_TRUNC, 0644)) < 0) {
		printf("err: can`t open '%s' as <dst>: %i (%s)", argv[2], errno, strerror(errno));
		return ERR;
	}

	// Get <src> size
	off_t src_size = get_f_size(src_fd);
	if (src_size < 0) {
		printf("err: can`t get the size of '%s': %i (%s)", argv[1], errno, strerror(errno));
		return ERR;
	}
	
	// io_uring_queue init
	struct io_uring ring;
	if (io_uring_queue_init(ENTRIES, &ring, 0) < 0) {
		printf("err: io_uring_queue_init: %i (%s)", errno, strerror(errno));
		return ERR;
	}

	cp(&ring, src_size);

	// free 
	close(src_fd);
	close(dst_fd);
	io_uring_queue_exit(&ring);

	return 0;
}
//
