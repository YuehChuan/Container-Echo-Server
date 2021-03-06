//mnt_echo_client.c
#include <sys/inotify.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>


#define BUF_LEN (10 * (sizeof(struct inotify_event) + NAME_MAX + 1))

int main(int argc, char *argv[])
{
	int inotifyFd, wd, j;
	char buf[BUF_LEN] __attribute__ ((aligned(8)));
	ssize_t numRead;
	char *p;
	struct inotify_event *event;

	system("rm -f client_message");
	system("rm -f bridge_message");


	inotifyFd = inotify_init();                 
	if (inotifyFd == -1) {
		perror(strerror(errno));
		printf("inotifyFd\n");
		return 1;
	}

	//wahtch the change of file, fire a event if it changes then  getcwd get the path
	wd = inotify_add_watch(inotifyFd, getcwd(NULL, 0), IN_CLOSE_WRITE);
	if (wd == -1) {
		perror(strerror(errno));
		printf("inotify_add_watch\n");
		return 1;
	}



	while(1){

		FILE *fp_c = fopen("client_message", "w");
		char input[4096], ch;
		while((ch = getchar()) != '\n'){
			fputc(ch, fp_c);
		}
		fputc('\n', fp_c);
		fclose(fp_c);

		// 1. send the message through writing to the client_message file
		// 2. client_message file will be read  and  deleted by bridge
		// 3. bridge will then send the message to server and receive the msg from server
		// 4. bridge will create the bridge_message file and write the message into it.
		// 5. after detecting the closing of the bridge_message file then open, read and delete the file 
		int flag = 0;


		while(1){
			
			numRead = read(inotifyFd, buf, BUF_LEN);
			if (numRead <= 0) {
				perror(strerror(errno));
				printf("read() from inotify fd returned %d!", numRead);
				return 1;
			}

			for (p = buf; p < buf + numRead; ) {
				event = (struct inotify_event *) p;
				if((event->mask|IN_CLOSE_WRITE) && !strcmp(event->name, "bridge_message")){

					char ch;
					FILE *fp_b = fopen("bridge_message", "r");
	

					printf("Recv:");
					while((ch = fgetc(fp_b)) != '\n')
						putchar(ch);
					printf("\n");
					fclose(fp_b);
					system("rm -f bridge_message");

					flag=1;
					break;

				}

				p += sizeof(struct inotify_event) + event->len;
			}
			if(flag==1)
				break;
		}
	}
	return 0;
}