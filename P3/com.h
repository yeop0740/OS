typedef struct demon {
	char path[4096];
	pid_t pid;
	struct demon *next;
}demon;

typedef struct d_queue {
	demon *front;
	demon *rear;
	int size;
}d_queue;

typedef struct fileinfo{
	char name[256];
	time_t mtime;
	int depth;
	int check;
	struct fileinfo *next;
	struct fileinfo *prev;
}file;

typedef struct fqueue{
	file *head;
}fqueue;

demon *create_demon(char *path, pid_t pid) {

	demon *new_demon = (demon *)malloc(sizeof(demon));
	strcpy(new_demon->path, path);
	new_demon->pid = pid;
	new_demon->next = NULL;
	
	return new_demon;
}

void create_d_queue(d_queue** q) {

	(*q) = (d_queue*)malloc(sizeof(d_queue));
	(*q)->front = NULL;
	(*q)->rear = NULL;
	(*q)->size = 0;
}

void add_demon(d_queue *q, demon* new_demon) {

	if(q->size == 0) {
		q->front = new_demon;
		q->rear = new_demon;
	}else {
		q->rear->next = new_demon;
		q->rear = new_demon;
	}
	q->size++;
}

demon *poll_demon(d_queue* q) {
	demon *rdemon;

	if(q->size == 0) {
		rdemon = NULL;
	}else if(q->size == 1) {
		rdemon = q->front;
		q->front = q->front->next;
		q->rear = NULL;
		rdemon->next = NULL;
		q->size--;
	}else {
		rdemon = q->front;
		q->front = q->front->next;
		q->size--;
		rdemon->next = NULL;
	}

	return rdemon;
}

void delete_d_queue(d_queue* q) {
	demon *rdemon;

	while(q->size >0) {
		rdemon = poll_demon(q);
		free(rdemon);
	}

	free(q);
}

demon *find_demon_by_path(d_queue *q, char *path) {
	demon *target = q->front;
	while(target != NULL) {
		if(!strcmp(target->path, path)) {
			return target;
		}
		target = target->next;
	}
	return NULL;
}

void print_d_queue(d_queue* q, FILE* fp) {
	demon *cur = q->front;

	for(int i = 0; i < q->size; i++) {
		fprintf(fp, "%s %d\n", cur->path, cur->pid);
		cur = cur->next;
	}

	if(q->size == 0) {
		printf("queue is empty\n");	
	}
}

void read_demons(d_queue *q) {
	FILE *fp;
	char path[4096];
	pid_t pid;
	demon *cur;

	if((fp = fopen("monitor_list.txt", "r")) == NULL) {
		fprintf(stderr, "file open error for monitor_list.txt\n");
		return ;
	}

	// [TODO] 없어도 될듯
	while(q->size != 0) {
		cur = poll_demon(q);
		free(cur);
	}

	while(fscanf(fp, "%s %d\n", path, &pid) != EOF) {
		add_demon(q, create_demon(path, pid));
	}

	fclose(fp);
}

file *create_file(char *filename, time_t mtime, int depth) {
	
	file* new_file = (file *)malloc(sizeof(file));
	strcpy(new_file->name, filename);
	new_file->mtime = mtime;
	new_file->depth = depth;
	new_file->check = 0;
	new_file->next = NULL;
	new_file->prev = NULL;
	
	return new_file;
}

void delete_file_list(file *root) {
	file *cur = root;
	file *rnode;

	while(cur != NULL) {
		rnode = cur;
		cur = cur->next;
		// cur->prev = NULL;
		rnode->next = NULL;
		free(rnode);
	}
}

void add_file(file **root, file *new_file) {
	file *cur = *root;
	
	if(*root == NULL) {
		*root = new_file;
		return;
	}

	while(cur->next !=  NULL) {
		cur = cur->next;
	}
	new_file->prev = cur;
	cur->next = new_file;
}

void create_fqueue(fqueue **q) {
	(*q) = (fqueue *)malloc(sizeof(fqueue));
	(*q)->head = NULL;
}

void get_path(file *target, char *path) {
	file *cur = target->prev;
	int depth = target->depth;
	char tmp[6000];

	sprintf(path, "%s", target->name);

	while(cur != NULL) {
		if(depth - 1 == cur->depth) {
			strcpy(tmp, path);
			sprintf(path, "%s/%s", cur->name, tmp);
			depth--;
		}
		cur = cur -> prev;
	}
	//printf("path : %s\n", path);
}

int has_next_file(file *target, int depth) {
	file *cur = target->next;

	while(cur != NULL && depth <= cur->depth) {
		if(cur->depth == depth) {
			return 1;
		}
		
		cur = cur->next;
	}
	return 0;
}

void print_file_list(file *root) {

	file *cur = root;
	char path[4096];
	while(cur != NULL) {
		for(int i = 1; i <= cur->depth; i++) {
			if(i == cur->depth) {
				if(has_next_file(cur, i)) {
					printf("├── ");
				}else {
					printf("└── ");
				}
			}
/*
			else if(i == 1) {
				if(has_next_file(cur, i)) {
					printf("│   ");
				}else{
					printf("└── ");
				}
			}
			*/
			else{
				if(has_next_file(cur, i)) {
					printf("│   ");
				}else {
					printf("    ");
				}
			}
		}
		printf("%s\n", cur->name);
		cur = cur->next;
	}
}


int is_log_file(file *cur) {
	if(cur->depth != 1 || strcmp("log.txt", cur->name)) {
		return 0;
	}
	return 1;
}

void make_file_list(char *filename, file **cur, int depth) {

	int count = 0, i;
	struct dirent **namelist;
	struct stat sb;
	char pwd[4096];
	char path[4096];
	file *node;
	file *new_file;

	getcwd(pwd, 4096);
	
	new_file = create_file(filename, 0l, depth);
	add_file(cur, new_file);
	depth++;

	if(chdir(filename) < 0) {
		fprintf(stderr, "chdir error for %s\n", filename);
		return ;
	}
	getcwd(path, 4096);

	if((count = scandir(path, &namelist, NULL, alphasort)) < 0) {
		fprintf(stderr, "scandir error for %s\n", path);
	}

	for(i = 0; i < count; i++) {
		if(!strcmp(namelist[i]->d_name, ".") || !strcmp(namelist[i]->d_name, "..") || (!strcmp(namelist[i]->d_name, "log.txt") && depth == 1)) {
			continue;
		}

		if(lstat(namelist[i]->d_name, &sb) < 0) {
			fprintf(stderr, "lstat error for %s\n", namelist[i]->d_name);
			continue;
		}

		if(S_ISDIR(sb.st_mode)) {
			make_file_list(namelist[i]->d_name, cur, depth);
		}else if(S_ISREG(sb.st_mode)) {
			add_file(cur, create_file(namelist[i]->d_name, sb.st_mtime, depth));
		}
	}

	//(*cur)->depth = *depth;
	if(count > 0) {
		while(count--) {
			free(namelist[count]);
		}
		free(namelist);
	}

	if(chdir(pwd) < 0) {
		fprintf(stderr, "chdir error for %s\n", pwd);
	}
	
}
