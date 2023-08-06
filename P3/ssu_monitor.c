#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <dirent.h>
#include "com.h"

int is_monitored_dir(d_queue *monitor_list, char *path) {
	demon *cur = monitor_list->front;
	
	for(int i = 0; i < monitor_list->size; i++) {
		if(!strcmp(path, cur->path)) {
			return 1;
		}
		cur = cur->next;
	}
	return 0;
}

void do_tree(int argc, char **argv) {
	file *root = NULL;
	d_queue *daemons;
	char target_path[4096];
	char pwd[4096];
	char dir[256];
	char *tmp;
	int depth = 0, fd;
	
	// check args
	if(argc != 2) {
		fprintf(stderr, "Usage: tree <DIRPATH>\n");
		exit(EXIT_FAILURE);
	}

	// make monitor list from monitor_list.txt
	create_d_queue(&daemons);

	if(access("monitor_list.txt", F_OK) == 0) {
		read_demons(daemons);
	}else {
		if((fd = open("monitor_list.txt", O_CREAT | O_RDWR | O_TRUNC, 0666)) < 0) {
			fprintf(stderr, "open error for monitor_list.txt\n");
			exit(EXIT_FAILURE);
		}
		close(fd);
	}
	read_demons(daemons);
	
	getcwd(pwd, 4096);
	if(chdir(argv[1]) < 0) {
		fprintf(stderr, "Usage : tree <DIRPATH>\n");
		exit(EXIT_FAILURE);
	}
	getcwd(target_path, 4096);
	tmp = strrchr(target_path, '/');
	tmp++;
	strcpy(dir, tmp);
	chdir("..");
	
	// valid directory
	if(!is_monitored_dir(daemons, target_path)) {
		fprintf(stderr, "%s is not monitored\n", target_path);
		exit(EXIT_FAILURE);
	}

	// make file list
	make_file_list(dir, &root, depth);

	print_file_list(root);
	
	// delete file list
	delete_file_list(root);
	
	// delete daemon queue
	delete_d_queue(daemons);
	

}

void do_exit(char **args) {

}

void do_help() {
	printf("Usage : add <DIRPATH> [OPTION]\n");
	printf("                       -t <TERM>\n");
	printf("        delete <DAEMON_PID>\n");
	printf("        tree <DIRPATH>\n");
	printf("        help\n");
	printf("        exit\n");
}

int do_monitoring(int argc, char **args) {
	pid_t pid, target_pid;

	pid = fork();
	if(pid == -1) {
		perror("fork()");
		exit(EXIT_FAILURE);
	}else if(pid == 0) {
		if(!strcmp(args[0], "add")) {
			if(execv("./add", args) == -1) {
				fprintf(stderr, "add exec failed\n");
				exit(EXIT_FAILURE);
			}
			exit(EXIT_SUCCESS);
		}else if(!strcmp(args[0], "delete")) {
			if(execv("./delete", args) == -1) {
				fprintf(stderr, "delete exec failed\n");
				exit(EXIT_FAILURE);
			}
			exit(EXIT_SUCCESS);
		}else if(!strcmp(args[0], "tree")) {
			do_tree(argc, args);
			exit(EXIT_SUCCESS);
		}else if(!strcmp(args[0], "exit")) {
			// do_exit(args);
			printf("Usage : exit\n");
			exit(EXIT_SUCCESS);
		}else {
			do_help();
			exit(EXIT_SUCCESS);

		}
	}else {
		if(wait(NULL) != pid) {
			fprintf(stderr, "wait error\n");
			return -1;
		}
	}
	return 0;
}

int main() {
	char input[4096];
	char *args[6];
	char *ptr;
	int argnum;

	while(1) {
		argnum = 0;

		printf("20180740> ");
		fgets(input, 4096, stdin);
		input[strlen(input) - 1] = 0;

		if(strlen(input) == 0) {
			continue;
		}

		if(!strcmp(input, "exit")) {
			break;
		}

		ptr = strtok(input, " ");
		while(ptr != NULL) {
			if(argnum < 5) {
					args[argnum++] = ptr;
			}
			ptr = strtok(NULL, " ");
		}
		args[argnum] = 0;

		do_monitoring(argnum, args);

		for(int i = 0; i < 6; i++) {
			args[i] = NULL;
		}
	}

	exit(EXIT_SUCCESS);
}
