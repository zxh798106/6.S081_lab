#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int is_prime(int n) {
	if (n <= 1) return 0;
	for (int i = 2; i * i <= n; ++i) {
		if (n % i == 0) return 0;
	}
	return 1;
}

void recur(int num) {
	if (num >= 35) return;

	int p[2];
	int buf[1];
	int buf_len = sizeof(buf);
	
	if (pipe(p) == -1) {
		printf("create pipe error, num is %d\n", num);
		exit(1);
	}
	
	int pid = fork();
	if (pid > 0) {
		close(p[0]);
		buf[0] = num;
		if (write(p[1], buf, buf_len) != buf_len) {
			printf("write error\n");
			exit(1);
		}
		close(p[1]); // close after close, otherwise, resources exceeed.
		wait(0);
		exit(0);
	}
	else if (pid == 0) {
		close(p[1]);
		if (read(p[0], buf, buf_len) != buf_len) {
			printf("read error\n");
			exit(1);
		}
		close(p[0]); // close after close, otherwise, resources exceeed.
		//printf("buf[0] is %d\n", buf[0]);
		if (is_prime(buf[0])) printf("prime %d\n", buf[0]);
		if (is_prime(buf[0] + 1)) printf("prime %d\n", buf[0] + 1);
		
		recur(buf[0] + 2);
		exit(0);
	}
	else {
		printf("fork error\n");
		exit(1);
	}
}

int main() {
	recur(2);
	exit(0);
	return 0;
}
