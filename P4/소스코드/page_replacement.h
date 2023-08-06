#include <stdio.h>
#include <stdlib.h>
#include <time.h>

struct pte {
	int page_num;
	int count;
	int reference_bit;
	int modify_bit;
	struct pte* next;
};

struct stream {
	int index;
	int page_num;
	int modify_bit;
	struct stream* next;
};

typedef struct pte pte;
typedef struct stream stream;

void init_stream(stream** list) {
	stream* new_stream = (stream*) malloc(sizeof(stream));

	new_stream->index = -1;
	new_stream->page_num = -1;
	new_stream->next = NULL;

	*list = new_stream;
}

void add_stream(stream* list, int page_num) {

	if (list->index == -1) {
		list->index = 0;
		list->page_num = page_num;
		list->next = NULL;

		return ;
	}
	stream* new_stream = (stream*) malloc(sizeof(stream));
	
	stream* cur = list;
	while (cur->next != NULL) {
		cur = cur->next;
	}

	new_stream->index = cur->index + 1;
	new_stream->page_num = page_num;
	new_stream->next = NULL;
	
	cur->next = new_stream;
}

void print_streamlist(stream* list) {
	stream* cur = list;
	printf("stream list : \n");
	while (cur != NULL) {
		printf("index : %d page_num : %d\n", cur->index, cur->page_num);
		cur = cur->next;
	}
	printf("\n");
}

pte* init_pagetable(int num) {
	pte* head = (pte*)malloc(sizeof(pte));

	for (int i = 0; i < num - 1; i++) {
		pte* cur = head;
	
		while(cur->next != NULL) {
			cur = cur->next;
		}
		cur->next = (pte*)malloc(sizeof(pte));
	}
	
	return head;
}

int is_pagefault(pte* head, int stream_num) {
	pte* cur = head;
	// stream_num must not be 0
	while (cur != NULL) {
		if(cur->page_num == stream_num) {
			//printf("page is in page table\n");
			return 0;
		}
		cur = cur->next;
	}

	//printf("page fault\n");
	return 1;
}

void print_pagetable(pte* head, FILE* fp) {
	pte* cur = head;
	while (cur != NULL) {
		if (cur->page_num != 0) {
			printf("%2d ", cur->page_num);
			fprintf(fp, "%2d ", cur->page_num);
		}
		cur = cur->next;
	}
}

pte* replace_fifo(pte* head, int stream_num) {
	pte* cur = head;
	// if pagetable is not full
	while (cur != NULL) {
		if (cur->page_num == 0) {
			cur->page_num = stream_num;
			//printf("page_num : 0 -> stream_num : %d\n", stream_num);
			return head;
		}
		cur = cur->next;
	}
	// if pagetable is full
	pte* replace = head;
	//printf("page_num : %d -> stream_num : %d\n", replace->page_num, stream_num);
	replace->page_num = stream_num;
	
	pte* new_head = head->next;
	cur = new_head;

	while (cur->next != NULL) {
		cur = cur->next;
	}

	cur->next = replace;
	//printf("replace : %d\n", cur->next->page_num);
	cur->next->next = NULL;

	return new_head;
}

pte* replace_lifo(pte* head, int stream_num) {
	pte* cur = head;
	// if pagetable is not full
	while (cur != NULL) {
		if (cur->page_num == 0) {
			cur->page_num = stream_num;
			return head;
		}
		cur = cur->next;
	}

	cur = head;
	// if pagetable is full
	while (cur->next != NULL) {
		cur = cur->next;
	}
	pte* replace = cur;
	replace->page_num = stream_num;
	
	return head;
}

int get_random_number(int min, int max) {
	int result;
	result = rand() % max + min;
	return result;
}

stream* get_stream(stream* list, int index) {
	stream* cur = list;
	if (cur->index == -1)
		return NULL;

	for(int i = 0; i < index; i++) {
		cur = cur->next;
	}

	return cur;
}	

int get_index_gap(stream* list, int curIndex, int cur_pageNum) {
	stream* cur = get_stream(list, curIndex + 1);

	while (cur != NULL && cur->page_num != cur_pageNum) {
		cur = cur->next;
	}

	if (cur == NULL)
		return 999; // MAX_INT

	return cur->index - curIndex;
}

// 한가지 우려되는 사항은 이 함수가 리스트의 끝까지 순회하는 것이 아닌 끝 바로 이전의 pte까지만 확일할 것같은 느낌이 든다.
pte* get_change_pte(pte* head, stream* list, int curIndex) {
	pte* cur = head;
	pte* max = head;
	while (cur->next != NULL) {
		if (get_index_gap(list, curIndex, max->page_num) < get_index_gap(list, curIndex, cur->next->page_num)) {
			max = cur->next;
		}
		cur = cur->next;
	}

	return max;
}

pte* get_prev_pte(pte* head, pte* change_pte) {
	pte* cur = head;

	if (cur == change_pte)
		return NULL;

	while (cur->next != change_pte && cur->next != NULL) {
		cur = cur->next;
	}

	return cur;
}

pte* replace_optimal(pte* head, stream* list, int stream_num, int curIndex) {
	pte* cur = head;
	// if pagetable is not full
	while (cur != NULL) {
		if (cur->page_num == 0) {
			cur->page_num = stream_num;
			return head;
		}
		cur = cur->next;
	}

	cur = head;
	// if pagetable is full
	//	pte* change_pte = get_change_pte(head, list, curIndex);

	pte* change_pte = get_change_pte(head, list, curIndex);
	pte* prev_pte;
	if (change_pte == head) {
		pte* new_head = change_pte->next;
		while (cur->next != NULL) {
			cur = cur->next;
		}
		change_pte->page_num = stream_num;
		change_pte->next = NULL;
		cur->next = change_pte;
		return new_head;
	}

	prev_pte = get_prev_pte(head, change_pte);
	prev_pte->next = change_pte->next;
	change_pte->page_num = stream_num;
	change_pte->next = NULL;
	while (cur->next != NULL) {
		cur = cur->next;
	}

	cur->next = change_pte;
	return head;
}

pte* get_referred_pte(pte* head, int stream_num) {
	pte* cur = head;
	while (cur != NULL) {
		if(cur->page_num == stream_num) {
			return cur;
		}
		cur = cur->next;
	}

	return NULL;
}

pte* replace_lru(pte* head, int stream_num, int* number_of_page_fault, FILE* fp) {

	if (is_pagefault(head, stream_num)) {
		*number_of_page_fault = *number_of_page_fault + 1;
		pte* cur = head;
		while(cur->next != NULL) {
			cur = cur->next;
		}
		pte* tail = get_prev_pte(head, cur);
		cur->page_num = stream_num;
		cur->next = head;
		//head = cur;
		tail->next = NULL;
		printf("      F     |");
		fprintf(fp, "      F     |");
		return cur;
	}
	pte* change = get_referred_pte(head, stream_num);
	if (change == NULL) {
		printf("get_referred_pte가 잘못되었습니다.\n");
		return NULL;
	}

	if (change == head) {
		change->page_num = stream_num;
		printf("            |");
		fprintf(fp, "            |");
		return change;
	}

	pte* prev = get_prev_pte(head, change);
	if (prev == NULL) {
		printf("get_prev_pte가 잘못되었습니다.\n");
		return NULL;
	}
	prev->next = change->next;
	change->page_num = stream_num;
	change->next = head;
	printf("            |");
	fprintf(fp, "            |");
	return change;
}

pte* get_least_used_pte(pte* head) {
	pte* min = head;
	pte* cur = head;
	while (cur->next != NULL) {
		if (min->count > cur->next->count) {
			min = cur->next;
		}
		cur = cur->next;
	}
	return min;
}

void print_pagetable_count(pte* head) {
	pte* cur = head;
	while (cur != NULL) {
		printf("%d ", cur->count);
		cur = cur->next;
	}
	printf("\n");
}

void increase_pagetable_count(pte* head, int page_num) {
	pte* cur = head;
	while (cur != NULL) {
		if (cur->page_num == page_num) {
			cur->count = cur->count + 1;
		}
		cur = cur->next;
	}
}

pte* replace_lfu(pte* head, int stream_num, int* number_of_page_fault, FILE* fp) {
	if (is_pagefault(head, stream_num)) {
		*number_of_page_fault = *number_of_page_fault + 1;
		pte* min = get_least_used_pte(head);
		if (min == head) {
			pte* new_head = min->next;
			min->page_num = stream_num;
			min->next = NULL;
			pte* cur = new_head;
			while (cur->next != NULL) {
				cur = cur->next;
			}
			cur->next = min;
			printf("      F     |");
			fprintf(fp, "      F     |");
			return new_head;
		}
		pte* prev = get_prev_pte(head, min);
		prev->next = min->next;
		min->page_num = stream_num;
		min->next = NULL;
		pte* cur = prev;
		while (cur->next != NULL) {
			cur = cur->next;
		}
		cur->next = min;
		printf("      F     |");
		fprintf(fp, "      F     |");
		return head;
	}
	increase_pagetable_count(head, stream_num);
	printf("            |");
	fprintf(fp, "            |");
	return head;
}

pte* init_circular_table(int num) {
	pte* head = (pte*)malloc(sizeof(pte));
	head->next = head;

	for (int i = 0; i < num - 1; i++) {
		pte* cur = head;
	
		while(cur->next != head) {
			cur = cur->next;
		}
		cur->next = (pte*)malloc(sizeof(pte));
		cur->next->next = head;
	}
	
	return head;
}

void print_circular_table(pte* head, FILE* fp) {
	pte* cur = head;
	while (cur->next != head) {
		if (cur->page_num != 0) {
			printf("%2d", cur->page_num);
			fprintf(fp, "%2d", cur->page_num);
			if (cur->reference_bit == 1) {
				printf("* ");
				fprintf(fp, "* ");
			}else {
				printf("  ");
				fprintf(fp, "  ");
			}
		}
		cur = cur->next;
	}

	if (cur->page_num != 0) {
		printf("%2d", cur->page_num); //통일성을 위해 나중에 \n은 뺀다.
		fprintf(fp, "%2d", cur->page_num);
		if (cur->reference_bit == 1) {
			printf("* ");
			fprintf(fp, "* ");
		}else {
			printf("  ");
			fprintf(fp, "  ");
		}
	}
}



void print_esc_table(pte* head, FILE* fp) {
	pte* cur = head;
	while (cur->next != head) {
		if (cur->page_num != 0) {
		printf("%2d(%d, %d) ", cur->page_num, cur->reference_bit, cur->modify_bit);
		fprintf(fp, "%2d(%d, %d) ", cur->page_num, cur->reference_bit, cur->modify_bit);
		}
		cur = cur->next;
	}

	if (cur->page_num != 0) {
		printf("%2d(%d, %d) ", cur->page_num, cur->reference_bit, cur->modify_bit);
		fprintf(fp, "%2d(%d, %d) ", cur->page_num, cur->reference_bit, cur->modify_bit);
	}
}

int is_circular_fault(pte* head, int stream_num) {
	pte* cur = head;
	// stream_num must not be 0
	while (cur->next != head) {
		if (cur->page_num == stream_num) {
			return 0;
		}
		cur = cur->next;
	}
	// last pte
	if (cur->page_num == stream_num) {
		//printf("last pte : %d\n", cur->page_num);
		return 0;
	}

	return 1;
}

pte* get_referring_pte(pte* head, int stream_num) {
	pte* cur = head;
	while (cur->next != head) {
		if (cur->page_num == stream_num) {
			return cur;
		}
		cur = cur->next;
	}

	if (cur->page_num == stream_num) {
		return cur;
	}

	return NULL;
}

pte* replace_sc(pte* head, pte* clock, int stream_num, int* number_of_page_fault, FILE* fp) {
	if (is_circular_fault(head, stream_num)) {
		*number_of_page_fault = *number_of_page_fault + 1;
		pte* cur = clock;
		while (cur->reference_bit != 0) {
			cur->reference_bit = 0;
			cur = cur->next;
		}
		cur->page_num = stream_num;
		cur->reference_bit = 1;
		printf("      F     |");
		fprintf(fp, "      F     |");
		return cur->next;
	}

	get_referring_pte(head, stream_num)->reference_bit = 1;
	printf("            |");
	fprintf(fp, "            |");
	return clock;
}

void add_esc_stream(stream* list, int page_num, int modify_bit) {

	if (list->index == -1) {
		list->index = 0;
		list->page_num = page_num;
		list->next = NULL;
		list->modify_bit = modify_bit;

		return ;
	}
	stream* new_stream = (stream*) malloc(sizeof(stream));
	
	stream* cur = list;
	while (cur->next != NULL) {
		cur = cur->next;
	}

	new_stream->index = cur->index + 1;
	new_stream->page_num = page_num;
	new_stream->next = NULL;
	new_stream->modify_bit = modify_bit;

	cur->next = new_stream;
}

void print_stream_modify_bits(stream* list) {
	stream* cur = list;
	printf("modify_bits : ");
	while (cur != NULL) {
		printf("%d ", cur->modify_bit);
		cur = cur->next;
	}
	printf("\n");
}

void print_pagetable_reference_bits(pte* head) {
	pte* cur = head;
	while (cur != NULL) {
		printf("%d ", cur->reference_bit);
		cur = cur->next;
	}
	printf("\n");
}

void print_circular_table_reference_bits(pte* head) {
	pte* cur = head;
	printf("reference_bits : ");
	while (cur->next != head) {
		printf("%2d ", cur->reference_bit);
		cur = cur->next;
	}
	printf("%2d\n", cur->reference_bit);
}

void print_circular_table_modify_bits(pte* head) {
	pte* cur = head;
	printf("modify_bits    : ");
	while (cur->next != head) {
		printf("%2d ", cur->modify_bit);
		cur = cur->next;
	}
	printf("%2d\n", cur->modify_bit);
}

void set_esc_pte(pte* change_pte, int stream_num, int modify_bit) {
		change_pte->page_num = stream_num;
		change_pte->reference_bit = 1;
		change_pte->modify_bit = modify_bit;
}

pte* replace_esc(pte* head, pte* clock, int stream_num, int modify_bit, int* number_of_page_fault, FILE* fp) {
	if (is_circular_fault(head, stream_num)) {
		*number_of_page_fault = *number_of_page_fault + 1;
		pte* cur = clock;
		while (1) {
			// find (0, 0)
			while (cur->next != clock) {
				if (cur->reference_bit == 0 && cur->modify_bit == 0) {
					set_esc_pte(cur, stream_num, modify_bit);
					printf("      F     |");
					fprintf(fp, "      F     |");
					return cur->next;
				}
				cur = cur->next;
			}

			if (cur->reference_bit == 0 && cur->modify_bit == 0) {
				set_esc_pte(cur, stream_num, modify_bit);
				printf("      F     |");
				fprintf(fp, "      F     |");
				return cur->next;
			}
			// or cur = cur->next;
			cur = clock; // 해당 문장이 필요한지 모르겠으나 혹 모르는 상황을 위해 넣음
			
			// find (0, 1)
			while (cur->next != clock) {
				if (cur->reference_bit == 0 && cur->modify_bit == 1) {
					set_esc_pte(cur, stream_num, modify_bit);
					printf("      F     |");
					fprintf(fp, "      F     |");
					return cur->next;
				}
				cur->reference_bit = 0;
				cur = cur->next;
			}

			if (cur->reference_bit == 0 && cur->modify_bit == 1) {
				set_esc_pte(cur, stream_num, modify_bit);
				printf("      F     |");
				fprintf(fp, "      F     |");
				return cur->next;
			}
			cur->reference_bit = 0;
			cur = clock; // or cur = cur->next;
		}
	}

	get_referring_pte(head, stream_num)->reference_bit = 1;
	if (modify_bit == 1) {
		get_referring_pte(head, stream_num)->modify_bit = 1;
	}
	printf("            |");
	fprintf(fp, "            |");
	return clock;
}

void free_pagetable(pte* head) {
	pte* cur = head;
	while(cur != NULL) {
		pte* del = cur;
		cur = del->next;
		free(del);
	}
}

void free_streams(stream* streams) {
	stream* cur = streams;
	while (cur != NULL) {
		stream* del = cur;
		cur = del->next;
		free(del);
	}
}