#include "IOUring.h"

bool IOUring::initIOUring(void)
{
	io_uring_params p;
	memset(&p, 0, sizeof(p));
	void *sq_ptr;
	void *cq_ptr;

	ring_fd = ioUringSetup(QUEUE_DEPTH, &p);
	if (ring_fd < 0)
	{
		printf("[%d] ioUringSetup failed (%d)", __func__, __LINE__);
		return false;
	}

	int sring_sz = p.sq_off.array + p.sq_entries * sizeof(unsigned);
	int cring_sz = p.cq_off.cqes + p.cq_entries * sizeof(io_uring_sqe);

	if (p.features & IORING_FEAT_SINGLE_MMAP)
	{
		if (cring_sz > sring_sz)
			sring_sz = cring_sz;
		cring_sz = sring_sz;
	}

	sq_ptr = mmap(0, sring_sz, PROT_READ | PROT_WRITE,
		MAP_SHARED | MAP_POPULATE,
		ring_fd, IORING_OFF_SQ_RING);
	if (sq_ptr == MAP_FAILED)
	{
		printf("[%d] sq_ptr mmap failed (%d)", __func__, __LINE__);
		return false;
	}

	if (p.features & IORING_FEAT_SINGLE_MMAP) {
		cq_ptr = sq_ptr;
	}
	else {
		cq_ptr = mmap(0, cring_sz, PROT_READ | PROT_WRITE,
			MAP_SHARED | MAP_POPULATE,
			ring_fd, IORING_OFF_CQ_RING);
		if (cq_ptr == MAP_FAILED)
		{
			printf("[%d] cq_ptr mmap failed (%d)", __func__, __LINE__);
			return false;
		}
	}

	sring_tail = static_cast<unsigned*>(sq_ptr + p.sq_off.tail);
	sring_mask = static_cast<unsigned*>(sq_ptr + p.sq_off.ring_mask);
	sring_array = static_cast<unsigned*>(sq_ptr + p.sq_off.array);

	sqes = static_cast<io_uring_sqe*>(mmap(0, p.sq_entries * sizeof(struct io_uring_sqe),
		PROT_READ | PROT_WRITE, MAP_SHARED | MAP_POPULATE,
		ring_fd, IORING_OFF_SQES));
	if (sqes == MAP_FAILED)
	{
		printf("[%d] sqes mmap failed (%d)", __func__, __LINE__);
		return false;
	}

	cring_head = static_cast<unsigned*>(cq_ptr + p.cq_off.head);
	cring_tail = static_cast<unsigned*>(cq_ptr + p.cq_off.tail);
	cring_mask = static_cast<unsigned*>(cq_ptr + p.cq_off.ring_mask);
	cqes = static_cast<io_uring_cqe*>(cq_ptr + p.cq_off.cqes);

	return false;
}

int IOUring::readFromCq(void)
{
	io_uring_cqe* cqe;
	unsigned head = *cring_head;

	if (head == *cring_tail)
	{
		printf("[%d] head == cring_tail (%d)", __func__, __LINE__);
		return -1;
	}
	cqe = &cqes[head & (*cring_mask)];
	if (cqe->res < 0) printf("[%d] cqe->res < 0 (%d)", __func__, __LINE__);
	head++;

	*cring_head = head;
	return cqe->res;
}

int IOUring::submitToSq(int fd, int op)
{
	unsigned index, tail;
	tail = *sring_tail;
	index = tail & *sring_mask;
	io_uring_sqe* sqe = &sqes[index];
	sqe->opcode = op;
	sqe->fd = fd;
	sqe->addr = (unsigned long)buff;
	if (op == IORING_OP_READ) {
		memset(buff, 0, sizeof(buff));
		sqe->len = BLOCK_SZ;
	}
	else {
		sqe->len = strlen(buff);
	}
	sqe->off = offset;
	sring_array[index] = index;
	tail++;
	*sring_tail = tail;
	int ret = ioUringEnter(ring_fd, 1, 1,
		IORING_ENTER_GETEVENTS);
	if (ret < 0)
	{
		printf("[%d] ioUringEnter ret < 0 (%d)", __func__, __LINE__);
		return -1;
	}
	return ret;
}

int IOUring::ioUringSetup(unsigned entries, io_uring_params* p)
{
	return static_cast<int>(syscall(__NR_io_uring_setup, entries, p));
}

int IOUring::ioUringEnter(int ring_fd, unsigned int to_submit,
	unsigned int min_complete, unsigned int flags)
{
	return static_cast<int>(syscall(__NR_io_uring_enter, ring_fd, to_submit,
		min_complete, flags, NULL, 0));
}
