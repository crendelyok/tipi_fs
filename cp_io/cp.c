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

#define ERR 1
#define ERR_SUBMIT -2
#define ERR_WAIT -3



static int src_fd, dst_fd;


#define ENTRIES 4
#define _BLOCK_SIZE (256*1024)

typedef enum {
	STATUS_READ = 0,
	STATUS_WRITE = 1,
	STATUS_UNDEFINED = 2
} rw_status;

struct io_data {
	rw_status status;
	off_t start_offset, offset;
	size_t len;
	struct iovec iov;
};

static off_t get_f_size(int fd)
{
	struct stat st;
	if (fstat(fd, &st) < 0)
		return -1;
	return st.st_size;
}

static int queue_read(struct io_uring* ring, off_t size, off_t offset)
{
	struct io_uring_sqe* sqe = io_uring_get_sqe(ring);
	if (sqe == NULL) 
		return -1;

	struct io_data* data = xmalloc(size * sizeof(*data));

	data->status = STATUS_READ;
	data->start_offset = offset;
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
	struct io_uring_sqe* sqe;
	sqe = io_uring_get_sqe(ring);

	io_uring_prep_writev(sqe, dst_fd, &data->iov, 1, data->offset);
	io_uring_sqe_set_data(sqe, data);
	io_uring_submit(ring);
	return 0;
}

static int cp(struct io_uring* ring, off_t src_size) 
{
	struct io_uring_cqe* cqe;

	off_t can_read, can_write, offset;
	can_write = src_size;
	can_read = src_size; 

	unsigned long readn, writen;
	readn = 0;
	writen = 0;

	// First 4 submits
	offset = 0;
	for (int i = 0; i < ENTRIES && can_read; i++) {
		off_t current_size = can_read;
		if (current_size > _BLOCK_SIZE)
			current_size = _BLOCK_SIZE;
		if (!current_size)
			break;

		if (queue_read(ring, current_size, offset) < 0)
			break;
		readn += 1;
		offset += current_size;
		can_read -= current_size;	
	}
	if (io_uring_submit(ring) < 0)
		return ERR_SUBMIT;	

	while (can_read || can_write) {
		// first we wait for at least 1 read
		// then we submit 1 write	
		if (can_write) {
			struct io_data* data;
			if (io_uring_wait_cqe(ring, &cqe) < 0)
				return ERR_WAIT;
			
			data = io_uring_cqe_get_data(cqe);
			queue_write(ring, data);
			can_write -= data->len;
			writen += 1;
		}

		if (can_read) {
			off_t active_size = can_read;
			// ring if full
			// choose active block size
			if (can_read > BLOCK_SIZE)
				active_size = BLOCK_SIZE;
			// no read left
			if (can_read == 0)
				break;

			if (queue_read(ring, active_size, offset) < 0)
				break;

			can_read -= active_size;
			offset += active_size;
			readn += 1;

		}
	}
	return 0;
}
int main(int argc, char* argv[])
{
	if (argc != 3) {
		printf("cp: missing file opeand\nexample: ./cp <src> <dest>\n");
		return ERR;
	}
	
	// Open <src> and <dst>
	if ((src_fd = open(argv[1], O_RDONLY)) < 0) {	
		LOG("err: open <src>");
		return ERR;
	}
	if ((dst_fd = open(argv[2], O_WRONLY | O_CREAT | O_TRUNC, 0644)) < 0) {
		LOG("err: open <dst>");
		return ERR;
	}

	// Get <src> size
	off_t src_size = get_f_size(src_fd);
	if (src_size < 0) {
		LOG("err: can`t get <src> file size");
		return ERR;
	}
	// io_uring_queue init
	struct io_uring ring;
	if (io_uring_queue_init(ENTRIES, &ring, 0) < 0) {
		LOG("err: io_uring_queue_init");
		return ERR;
	}

	int ret = cp(&ring, src_size);
	printf("ret cp: %d\n", ret);

	// free 
	close(src_fd);
	close(dst_fd);
	io_uring_queue_exit(&ring);
	return ret;
}
