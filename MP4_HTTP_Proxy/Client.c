#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>
#include <dirent.h>
#include <pthread.h>
#include <signal.h>
#include <sys/wait.h>


typedef unsigned int uint;

int parse_URL(char* URL, char *hostname, int *port, char *path) {
	char *token;
	char *host_temp, *path_temp;
	char *tmp1, *tmp2;
	int num = 0;
	char s[16];
	if (strstr(URL, "http") != NULL) {//get rid of protocol
		token = strtok(URL, ":");
		tmp1 = token + 7;
	}
	else {
		tmp1 = URL;
	}
	tmp2 = malloc(64);
	memcpy(tmp2, tmp1, 64);
	if (strstr(tmp1, ":") != NULL) {//to test if there is a port
		host_temp = strtok(tmp1, ":");
		*port = atoi(tmp1 + strlen(host_temp) + 1);
		sprintf(s, "%d", *port);
		path_temp = tmp1 + strlen(host_temp) + strlen(s) + 1;
	}
	else {
		host_temp = strtok(tmp1, "/");
		*port = 80;
		path_temp = tmp2 + strlen(host_temp);
	}
	if (strcmp(path_temp, "") == 0)
		strcpy(path_temp, "/");
	memcpy(hostname, host_temp, 64);
	memcpy(path, path_temp, 256);
	return(0);
}




int err_sys(const char* x)    // Error display source code
{
	perror(x);
	exit(1);
}

int main(int argc,char *argv[])
{
struct sockaddr_in  servaddr;
int sockfd, inet, conn;
char buff[512] = {0};//buffer
int sendret = 0;//send request flag
int recvret = 0;//receive response flag
uint port_number ;
char req[100];
char  *ptr;
char path[256] = {0};
char hostname[64] = {0};
int port = 80;//default port in URL
char URL[256] = {0};

  if (argc != 4)
  {
    err_sys ("Please input as following format: ./client <Server_IP_Address> <Port_Number> <URL>");
    exit(1);
  }
  port_number = atoi(argv[2]);
  bzero(&servaddr,sizeof servaddr);
  servaddr.sin_family=AF_INET;
  servaddr.sin_port=htons(port_number);
  inet = inet_aton(argv[1], (struct in_addr *)&servaddr.sin_addr.s_addr);      
  if (inet <= 0)
    err_sys ("Inet_aton error");

  sockfd=socket(AF_INET,SOCK_STREAM,0);  //create a socket
  if (sockfd < 0)
    err_sys ("Fails to create a socket");

  conn = connect(sockfd,(struct sockaddr *)&servaddr,sizeof(servaddr));
  if (conn < 0)
    err_sys ("Connect Error");
  memset(req, 0, 100);
  sprintf(req, "GET %s HTTP/1.0\r\n", argv[3]);
  printf("Request sent to proxy server: \n%s\n", req);  
  printf("Request sent to proxy server: \n%s\n", req);
  sendret = send(sockfd, req, strlen(req), 0);
  if (sendret == -1) {
      err_sys("Error: Send");
      exit(2);
  }
  memset(buff, 0, 512);
  parse_URL(argv[3], hostname, &port, path);//e.g. HOSTNAME-www.tamu.edu.com  
  FILE *fp;
  fp=fopen(hostname, "w");
  printf("Waiting for response\n");
  recvret = recv(sockfd, buff, 512, 0);
  if((strstr(buff, "200")) != NULL)//status code 200 implies that server responses the page
	printf("'200 OK' received. Saving to file: %s\n", hostname);
  else if ((strstr(buff, "400") != NULL))// status code 400 implies that server dont understand the syntax of request
	printf("'400 Bad Request' received. Saving to file: %s\n", hostname);
  else if ((strstr(buff, "404") != NULL))//status code 404 implies that server cant find the page
	printf("'404 Page Not Found' received. Saving to file: %s\n", hostname);
  //ptr = strstr(buff, "\r\n\r\n");
  fwrite(buff, 1, 512, fp);
  
  while(1) {
	  recvret = recv(sockfd, buff, 512, 0);
	  if (recvret < 0) {
		err_sys("Error: Recv");
		fclose(fp);
		close(sockfd);
		return 1;
	  }
	  else if(recvret == 0){
		  break;
	  }
	  fwrite(buff, 1, recvret, fp);
	  memset(buff, 0, 512);
  }
  
  fclose(fp);
  close(sockfd);

  return 0;

}
