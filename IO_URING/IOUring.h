#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/syscall.h>
#include <sys/mman.h>
#include <sys/uio.h>
#include <linux/fs.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdatomic.h>
#include <linux/io_uring.h>
#include <atomic>
#define QUEUE_DEPTH 1
#define BLOCK_SZ    1024

class IOUring
{
public:
	bool initIOUring(void);
	int readFromCq(void);
	int submitToSq(int fd, int op);
private:
	int ioUringSetup(unsigned entries, struct io_uring_params* p);
	int ioUringEnter(int ring_fd, unsigned int to_submit,
		unsigned int min_complete, unsigned int flags);
public:
	off_t offset;
private:
	// Ring
	int ring_fd;
	char buff[BLOCK_SZ];
	// Submission Queue
	struct io_uring_sqe* sqes;
	std::atomic<unsigned*> sring_tail;
	std::atomic<unsigned*> sring_mask;
	std::atomic<unsigned*> sring_array;

	// Comletion Queue
	struct io_uring_cqe* cqes;
	std::atomic<unsigned*> cring_head;
	std::atomic<unsigned*> cring_tail;
	std::atomic<unsigned*> cring_mask;

};

