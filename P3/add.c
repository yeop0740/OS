#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <syslog.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#include <time.h>
#include <dirent.h>
#include "com.h"

int init_monitoringd(char *path) {
	pid_t pid;
	int fd, maxfd;

	if((pid = fork()) < 0) {
		fprintf(stderr, "fork error\n");
		exit(1);
	}else if(pid != 0) {
		printf("monitoring started (%s)\n", path);
		exit(0);
	}

	pid = getpid();
	// printf("proces %d running as daemon\n", pid);
	setsid();
	signal(SIGTTIN, SIG_IGN);
	signal(SIGTTOU, SIG_IGN);
	signal(SIGTSTP, SIG_IGN);
	maxfd = getdtablesize();

	for(fd = 0; fd < maxfd; fd++) {
		close(fd);
	}

	umask(0);
	fd = open("/dev/null", O_RDWR);
	dup(0);
	dup(0);

	return pid;
}

int get_file_type(file *target) {
	if(target->mtime == 0) {
		return 0;
	}
	return 1;
}

int is_exist_file(file *src, char *path, int filetype) {
	file *cur = src;
	char tmp[4096];

	while(cur != NULL) {
		get_path(cur, tmp);
		if(!strcmp(tmp, path) && get_file_type(cur) == filetype) {
			cur->check = 1;
			return cur->mtime;
		}
		cur = cur->next;
	}
	return -1;
}

char *get_formated_time(time_t *time, char *buf) {
	struct tm *t;

	t = localtime(time);
	if(strftime(buf, 128, "%F %T", t) == 0) {
		fprintf(stderr, "strftime() error\n");
		return NULL;
	}
	return buf;
}

void compare_file_list(file *src, file *dst) {
	file *cur = dst;
	char tmp[4096];
	char abs_path[4096];
	char buf[128];
	char log[5000];
	time_t mtime;
	time_t now;
	int fd;
	char path[4096];

	if((fd = open("log.txt", O_WRONLY | O_APPEND)) < 0) {
		fprintf(stderr, "fopen error for log.txt\n");
		return;
		// exit(EXIT_FAILURE);
	}
	getcwd(path, 4096);
	chdir("..");

	while(cur != NULL) {
		get_path(cur, tmp);
		
		// if dst's file exist in src list
		if((mtime = is_exist_file(src, tmp, get_file_type(cur))) != -1) {
			// check modify or not
			if(mtime != cur->mtime) {
				// modify
				if(get_formated_time(&(cur->mtime), buf) == NULL) {
					fprintf(stderr, "get_formated_time() error\n");
					return;
				}
				// need to covert tmp to abs_path
				realpath(tmp, abs_path);
				sprintf(log, "[%s][modify][%s]\n", buf, abs_path);
				write(fd, log, strlen(log));
			}
		}
		// not exist in src list
		else {
		// craete
			if(cur->mtime != 0) {
				if(get_formated_time(&(cur->mtime), buf) == NULL) {
					fprintf(stderr, "get_formated_time() error\n");
					return;
				}
			
				realpath(tmp, abs_path);
				sprintf(log, "[%s][create][%s]\n", buf, abs_path);
				write(fd, log, strlen(log));
			}
		}
			

		cur = cur->next;
	}

	cur = src;
	while(cur != NULL) {
		if(cur -> check == 0 && cur->mtime != 0) {
			// delete
			get_path(cur, tmp);
			now = time(NULL);
			if(get_formated_time(&now, buf) == NULL) {
				fprintf(stderr, "get_formated_time() error\n");
				return;
			}
			char *ptr = strchr(tmp, '/');
			sprintf(abs_path, "%s%s", path, ptr);
			
			// realpath(tmp, abs_path);
			sprintf(log, "[%s][delete][%s]\n", buf, abs_path);
			write(fd, log, strlen(log));
			// cur -> check = 1; reset
		}
		cur = cur -> next;
	}
	chdir(path);
	close(fd);
}

void monitoring(fqueue *src, char *path) {
	file *new = NULL;
	int depth = 0;
	char pwd[4096];
	char dir[256];
	char *tmp;

	getcwd(pwd, 4096);
	chdir(path);
	chdir("..");
	
	tmp = strrchr(path, '/');
	tmp++;
	strcpy(dir, tmp);

	make_file_list(dir, &new, depth);
	

	if(new == NULL) {
		fprintf(stderr, "make_file_list() error\n");
		return;
	}

	chdir(path);
	compare_file_list(src->head, new);
	file *rfile = src->head;
	src->head = new;

	delete_file_list(rfile);

	chdir(pwd);
	return;
}


// check whether dir1 is subdirectory of dir2 or not
// input must be abs_path
int is_subdirectory(char *dir1, char *dir2) {
	char *tmp;

	tmp = strstr(dir2, dir1);
	if(tmp == NULL) {
		return 0;
	}else if(tmp == dir2) {
		int len = strlen(dir1);
		if(dir2[len] == '/' || dir2[len] == 0) {
			return 1;
		}else {
			return 0;
		}
	}else {
		return 0;
	}
}
// 절대 경로를 입력해 줘야하거나 들어와서 절대경로로 변환해 주거나
int is_valid_directory(char *directory,  d_queue *daemons) {
	int i;
	demon *cur;

	cur = daemons->front;
	for(i = 0; i < daemons->size; i++) {
		if(is_subdirectory(directory, cur->path) || is_subdirectory(cur->path, directory)) {
			return 0;
		}
		cur = cur->next;
	}
	return 1;
}

int check_option(int argc, char **argv, int *t) {
	int c;

	while((c = getopt(argc, argv, "t:")) != -1) {
		switch(c) {
			case 't':
				*t = atoi(optarg);
				break;
			case '?':
				printf("Unknown option %c\n", optopt);
				return -1;
		}
	}

	return 0;
}

int check_args(int argc, char **argv, d_queue *daemons, int *t) {
	char pwd[4096];
	char path[4096];

	getcwd(pwd, 4096);

	// check exist and directory
	if(chdir(argv[1]) < 0) {
		fprintf(stderr, "Usage : add <DIRPATH> [OPTION]\n        -t <TERM>\n");
		return -1;
	}

	// get absolute path about argv[1]
	getcwd(path, 4096);

	// change current working directory for write monitor_list.txt
	if(chdir(pwd) < 0) {
		fprintf(stderr, "chdir error for %s\n", pwd);
		return -1;
	}

	if(check_option(argc, argv, t) < 0) {
		return -1;
	}

	if(*t < 0) {
		fprintf(stderr, "Usage : add <DIRPATH> [OPTION]\n        -t <TERM>\n TERM >= 0\n");
		return -1;
	}

	if(!is_valid_directory(path, daemons)) {
		fprintf(stderr, "%s is already monitored directory\n", path);
		return -1;
	}
	return 0;
}

void write_demon(char *path, pid_t pid) {
	int fd;
	char tmp[1024];
	int len;

	if((fd = open("monitor_list.txt", O_WRONLY | O_APPEND)) < 0) {
		fprintf(stderr, "open error for monitor_list.txt\n");
		return ;
	}

	sprintf(tmp, "%s %d\n", path, pid);
	len = strlen(tmp);
	if(write(fd, tmp, len) != len) {
		fprintf(stderr, "write error for monitor_list.txt\n");
		return;
	}

	close(fd);
}


file *root = NULL;
d_queue *daemons;
volatile sig_atomic_t signal_recv = 0;
char terminal[4096];

void ssu_handler(int signo) {
	signal_recv = 1;
	int fd = open(terminal, O_RDWR);
	dup2(fd, 0);
	dup2(fd, 1);
	dup2(fd, 2);
	return;
}

char target_path[4096];
char dir[256];
int t = 1;

int main(int argc, char **argv) {

	pid_t pid;
	int fd;
	char *tmp;
	char path[4096];
	opterr = 0;
	fqueue *src;

	create_d_queue(&daemons);
	create_fqueue(&src);

	getcwd(path, 4096);
	
	if(argc < 2 || argc > 4) {
		fprintf(stderr, "Usage : add <DIRPATH> [OPTION]\n        -t <TERM>\n");
		return -1;
	}

	if(access("monitor_list.txt", F_OK) == 0) {
		read_demons(daemons);
	}else {
		if((fd = open("monitor_list.txt", O_CREAT | O_RDWR | O_TRUNC, 0666)) < 0) {
			fprintf(stderr, "open error for monitor_list.txt\n");
			exit(EXIT_FAILURE);
		}
		close(fd);
	}
	
	if(chdir(argv[1]) < 0) {
		fprintf(stderr, "Usage : add <DIRPATH> [OPTION]\n        -t <TERM>\n");
		exit(EXIT_FAILURE);
	}
	getcwd(target_path, 4096);
	tmp = strrchr(target_path, '/');
	tmp++;
	strcpy(dir, tmp);

	if(access("log.txt", F_OK) != 0) {
		if((fd = open("log.txt", O_CREAT | O_RDONLY | O_TRUNC, 0666)) < 0) {
			fprintf(stderr, "open error for log.txt\n");
			exit(EXIT_FAILURE);
		}
		close(fd);
	}

	chdir(path);
	
	if(check_args(argc, argv, daemons, &t) < 0) {
		exit(EXIT_FAILURE);
	}

	ttyname_r(1, terminal, 4096);
	
	if(init_monitoringd(target_path) < 0) {
		fprintf(stderr, "ssu_daemon_init failed\n");
		exit(EXIT_FAILURE);
	}

	chdir(path);
	write_demon(target_path, getpid());

	signal(SIGUSR1, ssu_handler);
	if(chdir(target_path) < 0) {
		fprintf(stderr, "chdir error for %s\n", dir);
		exit(EXIT_FAILURE);
	}
	if(chdir("..") < 0) {
		fprintf(stderr, "chdir error\n");
		exit(EXIT_FAILURE);
	}
	
	int depth = 0;

	make_file_list(dir, &root, depth);
	src->head = root;

	chdir(path);
	
	while(!signal_recv) {
		//monitoring
		monitoring(src, target_path);
		sleep(t);
	}
	printf("monitoring ended (%s)\n", target_path);

	delete_file_list(src->head);
	delete_d_queue(daemons);
	free(src);
	
	exit(0);
}
