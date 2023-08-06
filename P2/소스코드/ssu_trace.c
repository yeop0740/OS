#include "types.h"
#include "stat.h"
#include "user.h"


int main(int argc, char **argv)
{
	int n;
	if((n= atoi(argv[1])) < 1) {
		// error
		printf(2, "usage: ssu_trace [mask] [file ...]\n");
		exit();
	}// input mask
	//printf(1, "%s %s %s\n", argv[2], argv[3], argv[4]);

	if(argc < 3) {
		printf(2, "usage: ssu_trace [mask] [file ...]\n");
		exit();
	}
	int wpid;
	int pid = fork();
	if(pid < 0) {
		printf(1, "fork error\n");
		exit();
	}else if(pid == 0) {
		trace(n);
		exec(argv[2], argv + 2);
		printf(1, "exec error\n");
		exit();
	}
	
	while((wpid=wait()) > 0 && wpid != pid)
		printf(1, "zombie!\n");
	exit();
}
