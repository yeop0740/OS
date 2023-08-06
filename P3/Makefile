all: ssu_monitor add delete

ssu_monitor: ssu_monitor.o
	gcc ssu_monitor.o -o ssu_monitor

add: add.o
	gcc add.o -o add

delete: delete.o
	gcc delete.o -o delete

ssu_monitor.o: ssu_monitor.c
	gcc -c ssu_monitor.c

add.o: add.c
	gcc -c add.c

delete.o: delete.c
	gcc -c delete.c

clean:
	rm ssu_monitor.o
	rm add.o
	rm delete.o
	ssu_monitor
