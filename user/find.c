#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

char *getname(char *path) {
	char *p;
	for (p = path + strlen(path); p >= path && *p != '/'; --p);
	++p;
	return p;
}

void find(char *path, char *dst) {
	char buf[512], *p;
	int fd;
	struct dirent de;
	struct stat st;
	
	if ((fd = open(path, 0)) < 0) {
		fprintf(2, "find: cannot open %s\n", path);
		return;
	}
	
	if (fstat(fd, &st) < 0) {
		fprintf(2, "find: cannot stat %s\n", path);
   		close(fd);
    	return;
	}
	
	switch(st.type) {
		case T_FILE:
			if (strcmp(getname(path), dst) == 0) {
				printf("%s\n", path);
				//close(fd); // close after using file. Otherwize, file descriptor resources exceed.
			}
			//else close(fd); // close after using file. Otherwize, file descriptor resources exceed.
			break;
		case T_DIR:
			if(strlen(path) + 1 + DIRSIZ + 1 > sizeof buf){
			  printf("find: path too long\n");
			  break;
			}
			//if(*path == '.') strcpy(buf, path + 1);
			strcpy(buf, path);
			p = buf + strlen(buf);
			*p++ = '/';
			//printf("cur path is %s\n", buf);
			while (read(fd, &de, sizeof(de)) == sizeof(de)) {
				if (de.inum == 0) {
					//printf("buf %s, inum is 0\n", buf);
					//close(fd);
					continue;
				}
				memmove(p, de.name, strlen(de.name));
				p[strlen(de.name)] = 0;
				if(stat(buf, &st) < 0){
					printf("find: cannot stat buf is : %s  p is : %s\n", buf, p);
					continue;
			 	}
				if (strcmp(p, ".") != 0 && strcmp(p, "..") != 0) {
					//printf("next path is buf : %s  p : %s\n", buf, p);
					find(buf, dst);
				}
			}
			break;
	}
	close(fd); // close after using file. Otherwize, file descriptor resources exceed.
}

int main(int argc, char *argv[]) {
	
	if (argc < 2) {
		printf("input error\n");
		exit(0);
	}
	//printf("agrc is %d\n", argc);
	char *cur_dir = argv[1];
	//printf("cur_dir : %s\n", cur_dir);
	char *dst = argv[argc - 1];
	//printf("dst : %s\n", dst);
	find(cur_dir, dst);
	/*for (int i = 0; i < argc - 1; ++i) {
		find(".", dst);
	}*/
	exit(0);
}
