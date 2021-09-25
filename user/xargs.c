#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/param.h"

/* csdn上其他人的参考代码 */
/*int main(int argc, char *argv[]){
    int j = 0;
    int k;
    int l,m = 0;
    char block[32];
    char buf[32];
    char *p = buf
    char *new_argv[32];
    for(int i = 1; i < argc; i++){
        new_argv[j++] = argv[i];
    }
    if ((k = read(0, block, sizeof(block))) > 0) {
        for(l = 0; l < k; l++){
            if(block[l] == '\n'){
                buf[m] = 0;
                m = 0;
                new_argv[j++] = p;
                p = buf;
                new_argv[j] = 0;
                j = argc - 1;
                if(fork() == 0){
                    exec(argv[1], new_argv);
                }                
                wait(0);
            }else if(block[l] == ' ') {
                buf[m++] = 0;
                new_argv[j++] = p;
                p = &buf[m];
            }else {
                buf[m++] = block[l];
            }
        }
    }
    exit(0);
}*/




int main(int argc, char *argv[]) {
	
	int pid = fork();
	if (pid > 0) {
		wait(0);
		exit(0);
	}
	else if (pid == 0) {
		char buf[512];
		char *new_argv[MAXARG];
		// 读入原参数
		int argc_idx = 0;
		for (; argc_idx + 1 < argc; ++argc_idx) {
			//printf("argv[%d] : %s \n", argc_idx + 1, argv[argc_idx + 1]);
			new_argv[argc_idx] = argv[argc_idx + 1];
		}
		int start_idx = argc_idx;
		// 从管道中读取数据
		int buf_size = read(0, buf, sizeof(buf));
		buf_size = strlen(buf);
		//printf("buf:%s, size is %d\n", buf, buf_size);
		// 解析管道数据
		char *p = buf;
		new_argv[argc_idx] = p;
		while (p != buf + buf_size) {
			// 遇到'\n'创建子进程输出，父进程下标回到起始值
			if (*p == '\n') {
				*p = 0;
				if (fork() == 0) {		
					++argc_idx;
					new_argv[argc_idx] = 0;
					/*printf("buf : %s, *p : %c, ad_buf : %d, ad_p : %d\n", buf, *p, buf, p);
					for (int i = 0; i < argc_idx; ++i) {
						printf("new_argv[%d] : %s \n", i, new_argv[i]);
					}*/
					exec(new_argv[0], new_argv);
				}
				else {
					wait(0);
					argc_idx = start_idx;
					++p;
					new_argv[argc_idx] = p;
				}
			}
			// 将管道数据以空格破开，装入至new_argv中
			else if (*p == ' ') {
				*p = 0;
				++p;
				++argc_idx;
				new_argv[argc_idx] = p;
			}
			else ++p;
		}
		/*++argc_idx;
		new_argv[argc_idx] = 0;*/
		
		/*for (int i = 0; i < argc_idx; ++i) {
			printf("new_argv[%d] : %s \n", i, new_argv[i]);
		}*/
		
		//exec(new_argv[0], new_argv);
		//printf("exec error\n");
	}
	else {
		printf("fork error\n");
		exit(1);
	}
	exit(0);
}
