#include <sys/types.h>  
#include <sys/stat.h>  
#include <sys/queue.h>  
#include <sys/time.h>  
#include <fcntl.h>  
#include <stdlib.h>  
#include <stdio.h>  
#include <string.h>  
#include <unistd.h>  
#include <errno.h>  


int main(int argc, char **argv)
{
	char input[] = "hello world!";
	int fd;
	fd = open("event.fifo", O_WRONLY);
	if (fd == -1){
		perror("open error");
		exit(EXIT_FAILURE);
	}


	write(fd, input, strlen(input));
	close(fd);
	printf("write success\n");
	return 0;
}