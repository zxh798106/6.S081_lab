#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"


int main() {
	int pipe_p2c[2]; // parent write, child read
	int pipe_c2p[2]; // child write, parent read
	
	char buf[1] = {0};
	int buf_len = sizeof(buf);
	
	// create pipe
	if (pipe(pipe_p2c) == -1) {
		printf("error: create pipe_p2c");
		exit(1);
	}
	if (pipe(pipe_c2p) == -1) {
		printf("error: create pipe_c2p");
		exit(1);
	}
	
	int pid = fork();
	if (pid > 0) {
		close(pipe_p2c[0]);
		close(pipe_c2p[1]);
		if (write(pipe_p2c[1], buf, buf_len) != buf_len) {
			printf("parent write error\n");
			exit(1);
		}
		wait(0);
		if (read(pipe_c2p[0], buf, buf_len) != buf_len) {
			printf("parent read error\n");
			exit(1);
		}
		printf("%d: received pong\n", getpid());
		exit(0);
	} 
	else if (pid == 0) {
		close(pipe_p2c[1]);
		close(pipe_c2p[0]);
		if (read(pipe_p2c[0], buf, buf_len) != buf_len) {
			printf("child read error\n");
			exit(1);
		}
		printf("%d: received ping\n", getpid());
		if (write(pipe_c2p[1], buf, buf_len) != buf_len) {
			printf("child write error\n");
			exit(1);
		}
		exit(0);
	}
	else {
		printf("fork error\n");
		exit(1);
	}
}
