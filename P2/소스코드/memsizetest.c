#include "types.h"
#include "param.h"
#include "mmu.h"
#include "proc.h"
#include "user.h"
#include "x86.h"
#include "memlayout.h"
#include "spinlock.h"

#define SIZE 2048

int main(void)
{
	uint msize;
	msize = memsize();

	printf(1, "The process is using %dB\n", msize);

	char *tmp = (char *)malloc(SIZE * sizeof(char));

	printf(1, "Allocating more memory\n");
	msize = memsize();
	printf(1, "The process is using %dB\n", msize);

	free(tmp);
	printf(1, "Freeing memory\n");
	msize = memsize();
	printf(1, "The process is using %dB\n", msize);

	exit();
}
