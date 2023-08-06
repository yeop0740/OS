CC = gcc

UI = ui

all : $(UI)

$(UI) : ui.c
		$(CC) -o $@ $^ -g

clean :
		rm -rf *.o
		rm -rf $(UI)
