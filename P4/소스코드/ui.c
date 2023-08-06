#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "page_replacement.h"

int tokenize(char* input, char* argv[]) {
	char* ptr = NULL;
	int argc = 0;
	ptr = strtok(input, " ");

	while (ptr != NULL) {
		argv[argc++] = ptr;
		ptr = strtok(NULL, " ");
	}

	return argc;
}	

int main() {
	char input[1024];
	char* selected_algorithm[3];
	int number_of_selected;
	int algorithms[3];
	int number_of_page_frame;
	int input_way;
	int argc;
	int number_of_streams;
	char input_file_name[1024];
	char output_full_name[1024];
	char* output_file_name = "output";
	char* ext = "txt";
	int same_file_count = 0;

	sprintf(output_full_name, "%s.%s", output_file_name, ext);
	while (access(output_full_name, F_OK) != -1) {
		same_file_count++;
		sprintf(output_full_name, "%s(%d).%s",output_file_name, same_file_count, ext);
	}

	srand(time(NULL));

	printf("A. Page Replacement 알고리즘 시뮬레이터를 선택하시오 (최대 3개)\n");
	printf(" (1) Optimal  (2) FIFO  (3) LIFO  (4) LRU  (5) LFU  (6) SC  (7) ESC  (8) ALL\n");
	fgets(input, sizeof(input), stdin);
	input[strlen(input) - 1] = '\0';

	number_of_selected = tokenize(input, selected_algorithm);
	if (number_of_selected > 3 || number_of_selected < 1) {
		printf("최대 3가지 알고리즘을 고를 수 있습니다.\n");
		exit(-1);
	}
	
	argc = 0;

	for (int i = 0; i < number_of_selected; i++) {
		algorithms[i] = atoi(selected_algorithm[i]);
		if (algorithms[i] > 8 || algorithms[i] < 1) {
			printf("1 ~ 8 사이의 값을 입력할 수 있습니다.\n");
			exit(-1);
		}
		argc++;
	}

	if (argc > 1) {
		for (int i = 0; i < argc; i++) {
			if (algorithms[i] == 8) {
				printf("8번은 단독으로 선택하세요.\n");
				exit(-1);
			}
		}
	}

	printf("B. 페이지 프레임의 개수를 입력하시오.(3~10)\n");
	
	fgets(input, sizeof(input), stdin);
	input[strlen(input) - 1] = '\0';

	number_of_page_frame = atoi(input);

	if (number_of_page_frame < 3 || number_of_page_frame > 10) {
		printf("페이지 프레임의 수는 3과 10 사이의 정수만 가능합니다.\n");
		exit(-1);
	}

	printf("C. 데이터의 입력 방식을 선택하시오. (1,2)\n");
	printf(" (1) 랜덤하게 생성  (2) 사용자 생성 파일 오픈\n");

	fgets(input, sizeof(input), stdin);
	input[strlen(input) - 1] = '\0';

	input_way = atoi(input);

	FILE* fp;
	int length = 0;
	int offset = 0;
	int sep = 0;
	int i = 0;
	char buf[1024];
	char input_stream[10];
	char string_page_number[10];
	char string_modify_bit[10];

	stream* streams;
	init_stream(&streams);
	//number_of_streams = (rand() % 4501) + 500;
	number_of_streams = (rand() % 10001) + 500;

	if (input_way == 1) {
		for (int i = 0; i < number_of_streams; i++) {
			add_esc_stream(streams, get_random_number(1, 30), get_random_number(0, 2));
		}
	}else if (input_way == 2) {
		printf("파일 이름을 입력해 주세요.\n");
		fgets(input_file_name, sizeof(input_file_name), stdin);
		input_file_name[strlen(input_file_name) - 1] = '\0';

		if (access(input_file_name, F_OK) == -1) {
			printf("존재하지 않는 파일입니다.\n");
			return -1;
		}

		if((fp = fopen(input_file_name, "r")) == NULL) {
		printf("ERROR: fail to open\n");
		exit(-1);
		}

		while ((length = fread(buf,1, 1024, fp)) > 0) {
			buf[length] = '\0'; 
		
			i = 0;
			sep = 0;
			int string_input_length = 0;
			for (i = 0; i < length; i++) {
				if (buf[i] == ' ')
					continue;

				if (buf[i] == '|') {
					offset = offset + i + 1;
					fseek(fp, offset, SEEK_SET);
					break;
				}
				input_stream[string_input_length] = buf[i];
				string_input_length++;
			}
			input_stream[i] = '\0';

			for (i = 0; i < 10; i++) {
				if (input_stream[i] == ',') {
					sep = i + 1;
					break;
				}
				string_page_number[i] = input_stream[i];
			}
			string_page_number[sep - 1] = '\0';

			int modify_bit_length = 0;
			for (i = sep; i < 10; i++) {
				if (input_stream[i] == '\0')
					break;

				string_modify_bit[modify_bit_length] = input_stream[i];
				modify_bit_length++;

			}
			string_modify_bit[modify_bit_length] = '\0';
	
			int a = atoi(string_page_number);
			int b = atoi(string_modify_bit);
			if (a != 0) {
				add_esc_stream(streams, a, b);		
			}
		}

	fclose(fp);

	stream* last = streams;
	while (last->next != NULL) {
		last = last->next;
	}
	number_of_streams = last->index + 1;

	}else {
		printf("1이나 2를 입력 하세요.\n");
		exit(-1);
	}

	// 실제 알고리즘 수행
	fp = fopen(output_full_name, "a");
	pte* pt1 = init_pagetable(number_of_page_frame);
	int optimal_page_fault_counter = 0;


	printf(" OPTIMAL\n\n");
	printf(" page fault | stream | page table\n");
	fprintf(fp, " OPTIMAL\n\n");
	for (int i = 0; i < number_of_streams; i++) {
		if(is_pagefault(pt1, get_stream(streams, i)->page_num)) {
			pt1 = replace_optimal(pt1, streams, get_stream(streams, i)->page_num, i);
			printf("      F     |");
			fprintf(fp, "      F     |");
			optimal_page_fault_counter++;
		}else {
			printf("            |");
			fprintf(fp, "            |");
		}
		
		printf("   %2d   | ", get_stream(streams, i)->page_num);
		fprintf(fp, "   %2d   | ", get_stream(streams, i)->page_num);
		print_pagetable(pt1, fp);
		printf("\n");
		fprintf(fp, "\n");
	}
	printf("Page Fault : %d\nRate : %.2f\n\n\n", optimal_page_fault_counter, (float)optimal_page_fault_counter / number_of_streams * 100);
	fprintf(fp, "Page Fault : %d\nRate : %.2f\n\n\n", optimal_page_fault_counter, (float)optimal_page_fault_counter / number_of_streams * 100);

	for (int i = 0; i < argc; i++) {
		if (algorithms[i] == 1) {
			;
		}else if (algorithms[i] == 2) {
			pte* pt2 = init_pagetable(number_of_page_frame);
			int number_of_page_fault = 0;
			printf(" FIFO\n\n page fault | stream | page table\n");
			fprintf(fp, " FIFO\n\n page fault | stream | page table\n");
			for (int i = 0; i < number_of_streams; i++) {
				if (is_pagefault(pt2, get_stream(streams, i)->page_num)) {
					pt2 = replace_fifo(pt2, get_stream(streams, i)->page_num);
					printf("      F     |");
					fprintf(fp, "      F     |");
					number_of_page_fault++;
				}else {
					printf("            |");
					fprintf(fp, "            |");
				}
				printf("   %2d   | ", get_stream(streams, i)->page_num);
				fprintf(fp, "   %2d   | ", get_stream(streams, i)->page_num);
				print_pagetable(pt2, fp);
				printf("\n");
				fprintf(fp, "\n");
			}

			printf("Page Fault : %d\nRate : %.2f\n\n\n", number_of_page_fault, (float)number_of_page_fault / number_of_streams * 100);
			fprintf(fp, "Page Fault : %d\nRate : %.2f\n\n\n", number_of_page_fault, (float)number_of_page_fault / number_of_streams * 100);

		}else if (algorithms[i] == 3) {
			pte* pt3 = init_pagetable(number_of_page_frame);
			int number_of_page_fault = 0;
			printf(" LIFO\n\n page fault | stream | page table\n");
			fprintf(fp, " LIFO\n\n page fault | stream | page table\n");
			for (int i = 0; i < number_of_streams; i++) {
				if (is_pagefault(pt3, get_stream(streams, i)->page_num)) {
					pt3 = replace_lifo(pt3, get_stream(streams, i)->page_num);
					printf("      F     |");
					fprintf(fp, "      F     |");
					number_of_page_fault++;
				}else {
					printf("            |");
					fprintf(fp, "            |");
				}
				printf("   %2d   | ", get_stream(streams, i)->page_num);
				fprintf(fp, "   %2d   | ", get_stream(streams, i)->page_num);
				print_pagetable(pt3, fp);
				printf("\n");
				fprintf(fp, "\n");
			}
			
			printf("Page Fault : %d\nRate : %.2f\n\n\n", number_of_page_fault, (float)number_of_page_fault / number_of_streams * 100);
			fprintf(fp, "Page Fault : %d\nRate : %.2f\n\n\n", number_of_page_fault, (float)number_of_page_fault / number_of_streams * 100);

		}else if (algorithms[i] == 4) {
			pte* pt4 = init_pagetable(number_of_page_frame);
			int number_of_page_fault = 0;
			printf(" LRU\n\n page fault | stream | page table\n");
			fprintf(fp, " LRU\n\n page fault | stream | page table\n");
			for (int i = 0; i < number_of_streams; i++) {
				pt4 = replace_lru(pt4, get_stream(streams, i)->page_num, &number_of_page_fault, fp);
				printf("   %2d  | ", get_stream(streams, i)->page_num);
				fprintf(fp, "   %2d  | ", get_stream(streams, i)->page_num);
				print_pagetable(pt4, fp);
				printf("\n");
				fprintf(fp, "\n");
			}
			
			printf("Page Fault : %d\nRate : %.2f\n\n\n", number_of_page_fault, (float)number_of_page_fault / number_of_streams * 100);
			fprintf(fp, "Page Fault : %d\nRate : %.2f\n\n\n", number_of_page_fault, (float)number_of_page_fault / number_of_streams * 100);

		}else if (algorithms[i] == 5) {
			pte* pt5 = init_pagetable(number_of_page_frame);
			int number_of_page_fault = 0;
			printf(" LFU\n\n page fault | stream | page table\n");
			fprintf(fp, " LFU\n\n page fault | stream | page table\n");
			for (int i = 0; i < number_of_streams; i++) {
				pt5 = replace_lfu(pt5, get_stream(streams, i)->page_num, &number_of_page_fault, fp);
				printf("   %2d   | ", get_stream(streams, i)->page_num);
				fprintf(fp, "   %2d   | ", get_stream(streams, i)->page_num);
				print_pagetable(pt5, fp);
				printf("\n");
				fprintf(fp, "\n");
			}
			
			printf("Page Fault : %d\nRate : %.2f\n\n\n", number_of_page_fault, (float)number_of_page_fault / number_of_streams * 100);
			fprintf(fp, "Page Fault : %d\nRate : %.2f\n\n\n", number_of_page_fault, (float)number_of_page_fault / number_of_streams * 100);

		}else if (algorithms[i] == 6) {
			pte* pt6 = init_circular_table(number_of_page_frame);
			pte* clock1 = pt6;
			int number_of_page_fault = 0;
			printf(" SC\n\n page fault | stream | page table\n");
			fprintf(fp, " SC\n\n page fault | stream | page table\n");
			for (int i = 0; i < number_of_streams; i++) {
				clock1 = replace_sc(pt6, clock1, get_stream(streams, i)->page_num, &number_of_page_fault, fp);
				printf("   %2d   | ", get_stream(streams, i)->page_num);
				fprintf(fp, "   %2d   | ", get_stream(streams, i)->page_num);
				print_circular_table(pt6, fp);
				printf("\n");
				fprintf(fp, "\n");
			}
			
			printf("Page Fault : %d\nRate : %.2f\n\n\n", number_of_page_fault, (float)number_of_page_fault / number_of_streams * 100);
			fprintf(fp, "Page Fault : %d\nRate : %.2f\n\n\n", number_of_page_fault, (float)number_of_page_fault / number_of_streams * 100);

		}else if (algorithms[i] == 7) {
			pte* pt7 = init_circular_table(number_of_page_frame);
			pte* clock2 = pt7;
			int number_of_page_fault = 0;
			printf(" ESC\n\n page fault | stream | modify bit | page table\n");
			fprintf(fp, " ESC\n\n page fault | stream | modify bit | page table\n");
			for (int i = 0; i < number_of_streams; i++) {
				clock2 = replace_esc(pt7, clock2, get_stream(streams, i)->page_num, get_stream(streams, i)->modify_bit, &number_of_page_fault, fp);
				printf("   %2d   |      %d     | ", get_stream(streams, i)->page_num, get_stream(streams, i)->modify_bit);
				fprintf(fp, "   %2d   |      %d     | ", get_stream(streams, i)->page_num, get_stream(streams, i)->modify_bit);
				print_esc_table(pt7, fp);
				printf("\n");
				fprintf(fp, "\n");
			}
			
			printf("Page Fault : %d\nRate : %.2f\n\n\n", number_of_page_fault, (float)number_of_page_fault / number_of_streams * 100);
			fprintf(fp, "Page Fault : %d\nRate : %.2f\n\n\n", number_of_page_fault, (float)number_of_page_fault / number_of_streams * 100);

		}else if (algorithms[i] == 8) {
			
			pte* pt2 = init_pagetable(number_of_page_frame);
			int number_of_page_fault = 0;
			printf(" FIFO\n\n page fault | stream | page table\n");
			fprintf(fp, " FIFO\n\n page fault | stream | page table\n");
			for (int i = 0; i < number_of_streams; i++) {
				if (is_pagefault(pt2, get_stream(streams, i)->page_num)) {
					pt2 = replace_fifo(pt2, get_stream(streams, i)->page_num);
					printf("      F     |");
					fprintf(fp, "      F     |");
					number_of_page_fault++;
				}else {
					printf("            |");
					fprintf(fp, "            |");
				}
				printf("   %2d   | ", get_stream(streams, i)->page_num);
				fprintf(fp, "   %2d   | ", get_stream(streams, i)->page_num);
				print_pagetable(pt2, fp);
				printf("\n");
				fprintf(fp, "\n");
			}

			printf("Page Fault : %d\nRate : %.2f\n\n\n", number_of_page_fault, (float)number_of_page_fault / number_of_streams * 100);
			fprintf(fp, "Page Fault : %d\nRate : %.2f\n\n\n", number_of_page_fault, (float)number_of_page_fault / number_of_streams * 100);

			pte* pt3 = init_pagetable(number_of_page_frame);
			number_of_page_fault = 0;
			printf(" LIFO\n\n page fault | stream | page table\n");
			fprintf(fp, " LIFO\n\n page fault | stream | page table\n");
			for (int i = 0; i < number_of_streams; i++) {
				if (is_pagefault(pt3, get_stream(streams, i)->page_num)) {
					pt3 = replace_lifo(pt3, get_stream(streams, i)->page_num);
					printf("      F     |");
					fprintf(fp, "      F     |");
					number_of_page_fault++;
				}else {
					printf("            |");
					fprintf(fp, "            |");
				}
				printf("   %2d   | ", get_stream(streams, i)->page_num);
				fprintf(fp, "   %2d   | ", get_stream(streams, i)->page_num);
				print_pagetable(pt3, fp);
				printf("\n");
				fprintf(fp, "\n");
			}
			
			printf("Page Fault : %d\nRate : %.2f\n\n\n", number_of_page_fault, (float)number_of_page_fault / number_of_streams * 100);
			fprintf(fp, "Page Fault : %d\nRate : %.2f\n\n\n", number_of_page_fault, (float)number_of_page_fault / number_of_streams * 100);

			pte* pt4 = init_pagetable(number_of_page_frame);
			number_of_page_fault = 0;
			printf(" LRU\n\n page fault | stream | page table\n");
			fprintf(fp, " LRU\n\n page fault | stream | page table\n");
			for (int i = 0; i < number_of_streams; i++) {
				pt4 = replace_lru(pt4, get_stream(streams, i)->page_num, &number_of_page_fault, fp);
				printf("   %2d  | ", get_stream(streams, i)->page_num);
				fprintf(fp, "   %2d  | ", get_stream(streams, i)->page_num);
				print_pagetable(pt4, fp);
				printf("\n");
				fprintf(fp, "\n");
			}
			
			printf("Page Fault : %d\nRate : %.2f\n\n\n", number_of_page_fault, (float)number_of_page_fault / number_of_streams * 100);
			fprintf(fp, "Page Fault : %d\nRate : %.2f\n\n\n", number_of_page_fault, (float)number_of_page_fault / number_of_streams * 100);

			pte* pt5 = init_pagetable(number_of_page_frame);
			number_of_page_fault = 0;
			printf(" LFU\n\n page fault | stream | page table\n");
			fprintf(fp, " LFU\n\n page fault | stream | page table\n");
			for (int i = 0; i < number_of_streams; i++) {
				pt5 = replace_lfu(pt5, get_stream(streams, i)->page_num, &number_of_page_fault, fp);
				printf("   %2d   | ", get_stream(streams, i)->page_num);
				fprintf(fp, "   %2d   | ", get_stream(streams, i)->page_num);
				print_pagetable(pt5, fp);
				printf("\n");
				fprintf(fp, "\n");
			}
			
			printf("Page Fault : %d\nRate : %.2f\n\n\n", number_of_page_fault, (float)number_of_page_fault / number_of_streams * 100);
			fprintf(fp, "Page Fault : %d\nRate : %.2f\n\n\n", number_of_page_fault, (float)number_of_page_fault / number_of_streams * 100);

			pte* pt6 = init_circular_table(number_of_page_frame);
			pte* clock1 = pt6;
			number_of_page_fault = 0;
			printf(" SC\n\n page fault | stream | page table\n");
			fprintf(fp, " SC\n\n page fault | stream | page table\n");
			for (int i = 0; i < number_of_streams; i++) {
				clock1 = replace_sc(pt6, clock1, get_stream(streams, i)->page_num, &number_of_page_fault, fp);
				printf("   %2d   | ", get_stream(streams, i)->page_num);
				fprintf(fp, "   %2d   | ", get_stream(streams, i)->page_num);
				print_circular_table(pt6, fp);
				printf("\n");
				fprintf(fp, "\n");
			}
			
			printf("Page Fault : %d\nRate : %.2f\n\n\n", number_of_page_fault, (float)number_of_page_fault / number_of_streams * 100);
			fprintf(fp, "Page Fault : %d\nRate : %.2f\n\n\n", number_of_page_fault, (float)number_of_page_fault / number_of_streams * 100);

			pte* pt7 = init_circular_table(number_of_page_frame);
			pte* clock2 = pt7;
			number_of_page_fault = 0;
			printf(" ESC\n\n page fault | stream | modify bit | page table\n");
			fprintf(fp, " ESC\n\n page fault | stream | modify bit | page table\n");
			for (int i = 0; i < number_of_streams; i++) {
				clock2 = replace_esc(pt7, clock2, get_stream(streams, i)->page_num, get_stream(streams, i)->modify_bit, &number_of_page_fault, fp);
				printf("   %2d   |      %d     | ", get_stream(streams, i)->page_num, get_stream(streams, i)->modify_bit);
				fprintf(fp, "   %2d   |      %d     | ", get_stream(streams, i)->page_num, get_stream(streams, i)->modify_bit);
				print_esc_table(pt7, fp);
				printf("\n");
				fprintf(fp, "\n");
			}
			
			printf("Page Fault : %d\nRate : %.2f\n\n\n", number_of_page_fault, (float)number_of_page_fault / number_of_streams * 100);
			fprintf(fp, "Page Fault : %d\nRate : %.2f\n\n\n", number_of_page_fault, (float)number_of_page_fault / number_of_streams * 100);


		}else {
			printf("1번 문항 문제\n");
			exit(-1);
		}
	}

	fclose(fp);

	printf("D. 종료\n");
}
