#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <signal.h>
#include <dirent.h>
#include "com.h"

int write_demon(int fd, demon *d) {
	char tmp[5000];
	int len;

	sprintf(tmp, "%s %d\n", d->path, d->pid);
	len = strlen(tmp);
	if(write(fd, tmp, len) != len) {
		fprintf(stderr, "write error for monitor_list.txt\n");
		return -1;
	}
	return 0;
}

int is_valid_pid(d_queue *daemons, pid_t pid) {
	int i;
	demon *cur;

	cur = daemons->front;
	for(i = 0; i < daemons->size; i++) {
		if(cur->pid == pid) {
			return 1;
		}
		cur = cur->next;
	}
	return 0;
}

void check_option(int argc, char **argv, pid_t *pid) {
	
	if(argc != 2) {
		fprintf(stderr, "Usage: <DAEMON PID>\n");
		exit(EXIT_FAILURE);
	}

	*pid = atoi(argv[1]);
}

int main(int argc, char **argv) {
	pid_t pid;
	int fd;
	demon *cur, *target;
	d_queue *daemons;

	check_option(argc, argv, &pid);

	create_d_queue(&daemons);
	// read_demons
	read_demons(daemons);

	// is_valid_pid
	if(!is_valid_pid(daemons, pid)) {
		fprintf(stderr, "%d is not running process\n", pid);
		exit(EXIT_FAILURE);
	}

	// signal
	kill(pid, SIGUSR1);

	// rewrite monitor_list.txt
	if((fd = open("monitor_list.txt", O_WRONLY | O_CREAT | O_TRUNC, 0666)) < 0) {
		fprintf(stderr, "open error for monitor_list.txt\n");
		exit(EXIT_FAILURE);
	}

	cur = daemons->front;
	while(cur != NULL) {
		if(cur->pid != pid) {
			write_demon(fd, cur);
		}else {
			target = cur;
		}
		cur = cur->next;
	}
	close(fd);

	// print teminate string
	//printf("monitoring ended (%s)\n", target->path);

	delete_d_queue(daemons);
}
