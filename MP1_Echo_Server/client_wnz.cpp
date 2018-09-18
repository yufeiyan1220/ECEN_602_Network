#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <iostream>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdio_ext.h>
#include <stdlib.h>
using namespace std;

#define MAXLINE 50

//********readline function*******//
int readline(int fd, char* buf, int len)
{
	int bytes, bytes_get;
	char x;
	char *y;
	if (buf == NULL || len <= 0)
	{
		errno = EINVAL;
		return -1;
	}

	bytes_get = 0;
	y = buf;
	while (1)
	{
		bytes = read(fd, &x, 1);  // returns one character per function call 
		if (bytes == -1)
		{
			if (errno == EINTR)
				continue;
			else
				return -1;
		}
		else if (bytes == 0)
		{
			if (bytes_get == 0)  //read no character
				return 0;
			else   break;      //a line terminated with a newline
		}
		if (bytes_get<len - 1)
		{
			bytes_get++;
			*y = x;
			y++;
		}
		if (x == EOF) return -1;  //turn to close the socket
		if (x == '\n')
			break;
	}
	*y = '\0';
	return bytes_get;
}

//*************writen function**********//
int writen(int fd, char* buf, int len)
{
	char*  sen = buf;
	int bytes_left = len;    //total characters need to be writen
	int bytes_written;
	while (bytes_left>0)
	{
		bytes_written = (int)write(fd, sen, bytes_left);
		if (bytes_written <= 0)
		{
			if (bytes_written<=0 && errno == EINTR)
				bytes_written = 0;
			else     return -1;
		}
		sen += bytes_written;
		bytes_left -= bytes_written;
	}
	return bytes_written;
}

void str_cli(FILE* stdin, int sockfd)
{
	char sendline[MAXLINE], recvline[MAXLINE];
	while (fgets(sendline, MAXLINE, stdin) != NULL) //get the message from keyboard to sendline
	{
		if (feof(stdin))
		{
			cout << "\n" << "EOF detected, stop sending messages and exit" << endl;
			break;
		}

		sendline[MAXLINE - 1] = '\0';
		if (strlen(sendline) == MAXLINE - 1 && sendline[MAXLINE - 1] != '\n')
		{
			cout << "size of input string exceed the maximum" << endl;
			cout << "restart, the maximum is  " << MAXLINE - 1 << endl;
			__fpurge(stdin);
			continue;
		}
		writen(sockfd, sendline, strlen(sendline));   //write to the server
		if (readline(sockfd, recvline, MAXLINE) == 0)
			perror("str_cli:server shut off");
		fputs(recvline, stdout);//echo it on screen
	}
}




int main(int argc, char *argv[])
{
	if (argc != 3)
	{
		printf("command error");
		exit(0);
	}
	
	int c0,result,port_num;
	struct sockaddr_in servaddr;
	
	//****get port number*******//
	for (char *p = argv[2]; *p != '\0'; p++) {
		if (*p >= '0' && *p <= '9') {
			port_num = port_num * 10 + (*p - '0');
		}
		else {
			printf("Invaild port number error\n");
			exit(0);
		}
	}
	printf("Port number is:%d\n", port_num);
	
	c0 = socket(AF_INET, SOCK_STREAM, 0);
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(port_num);// server listen port
	result=inet_pton(AF_INET, argv[1], &servaddr.sin_addr);// IP address of server
	if (result<0)
	{
		perror("first parameter error:system doesn't support this protocol type");
		close(c0);
		exit(1);
	}
	else if (result == 0)
	{
		perror("second parameter is not valid format");
		exit(1);
	}
	if (connect(c0, (const struct sockaddr*)&servaddr, sizeof(struct sockaddr_in)) == -1)
	{
		perror("fail to connect");
		close(c0);
		exit(1);
	}
	cout << "Connected to server\n";

	cout << "Input a sentence(Ctrl+D means to stop connection)" << endl;
	cout << "The maximum length is 50" << endl;
	
	str_cli(stdin, c0);//echo
	exit(0);
}
