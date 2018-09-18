#include <stdio.h>
#include <string>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h> 
#include <sys/un.h>
#include <signal.h>
#include <sys/wait.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>

//STR_SIZE is the maxsize of message, MAXLINE is to give a buffer
#define MAXLINE 100
#define STR_SIZE 50

int readline (int sockfd, char* buffer, int n);
int writen (int sockfd, char* buffer, int n);
void sig_handling(int sig);
int echo_new(int sockfd);
char* trim(char *str);

int main (int argc, char **argv){
   
	int sockfd;
	char recvline[MAXLINE+1];
	struct sockaddr_in servaddr;
	int port_num;
	
	//Command is echo port
	if (argc != 2){
		printf( "Command error\n");
		exit(0);
	}
	//create a socket
    if ((sockfd = socket(AF_INET,SOCK_STREAM,0)) < 0){
		printf("Socket error\n");
		exit(0);
	}
	//Get port number.
	for (char *p = argv[1]; *p != '\0'; p++) {
		if (*p >= '0' && *p <= '9') {
			port_num = port_num * 10 + (*p - '0');
		}
		else {
			printf("Invaild port number error\n");
			exit(0);
		}	
	}
	printf("Port number is:%d\n", port_num);
	
	//Socket initialization
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(port_num);
	
	//inet_aton("10.230.169.95", &servaddr.sin_addr);
	servaddr.sin_addr.s_addr = INADDR_ANY;
	
	//printf("%d\n", servaddr.sin_addr.s_addr);
	socklen_t servaddr_size = sizeof(servaddr);
	
    //binding (sockaddr and sockaddr_in is at same size, so just cast)
	if (bind (sockfd, (struct sockaddr*)&servaddr, servaddr_size) < 0){
        printf("Binding socket error.");
        printf("Error number:%d\n",(int) errno);
        exit(0);
    }
	else {
		printf("Socket binded successfully.\n");
	}
	//listen to the client
	//backblog: max connected number.
	if(listen(sockfd, 10) < 0){
        printf("Unable to find client.\n");
        printf("Error number%d\n ",(int) errno);
        exit(0);
    }
	else {
		//listen to socket and waiting for client
		printf("Listening to the cilent.\n");
	}
	
	//Prepared for echo.
	char received_str[MAXLINE];
	//Process the zombie using sigaction
	struct sigaction act;
	act.sa_handler = sig_handling;
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;
	sigaction(SIGCHLD,&act,0);	
	
	
	while(true){    
        struct sockaddr_in clientaddr;
        socklen_t clientaddr_size = sizeof(clientaddr);
        int client_socket = accept(sockfd, (struct sockaddr*)&clientaddr, &clientaddr_size);
        
        if(client_socket < 0){
			if(errno == EINTR){ 
			//EINTR, try it again.
                continue;
            }
			else {
				printf("Unable to accept client\n");
				printf("Error number: %d\n", (int) errno);
				exit(0);
			}
        }
        //create a child process
		
        int pid = fork();
        
        if (pid < 0){
			perror("Error of fork a child\n");
			exit(1);
		}
		else if (pid == 0){
			//child process		
			//echo(client_socket);
			close(sockfd);
			echo_new(client_socket);
			close(client_socket);
			exit(0);
		}
		//main process do nothing but loop.
		else {
				
			//sleep(0.2);
		}
        
    }
    return 0;
}


int readline (int sockfd, char* buffer, int n){
	char str[MAXLINE + 1];
    int len;
    int letter;
    char *b;
    char character;
    
    if (n <= 0 || buffer == NULL) {
        printf("Invaild input.\n");
        return -1;
    }
	
    //Initialization
    b = buffer;
    len = 0;
    
    for(int i = 0; i < MAXLINE; i++){
		//read one letter
        letter = (int)read(sockfd, &character, 1);
		//printf("%d\n",letter);
		if(letter == -1) {
			if(errno == EINTR){ 
			//EINTR, try it again.
                continue;
            }
			else {
			//Some errors occur
                return -1;
            }
		}
		else if(letter == 0) {
			//Some letter is read, end with '\0', read is over                 
			break;
		}
		else {
			// only read <= n-1 letters
			if (len < n - 1) {      
                len++;				
                *b = character;
                b++;
            }
			// The line is over.
            if (character == '\n') {
				break;
			}               
		}
    }
	//Mark the end of the string(line)
    *b = '\0';
    return len;
}

int writen (int sockfd, char* buffer, int n){
    int len_written;
    char* b = buffer;
	/*
	write(fp, p1+len, (strlen(p1)-len)
	*/
	for (int i = n; i > 0; ){
		len_written = write(sockfd, b, i);
		if (len_written <= 0){
            if (len_written < 0 && errno == EINTR){
				//Try it again.
                continue;
            }
            else {
				//Some errors occur.
                return -1;
            }
        }
		else {
			i -= len_written;
			//point to the next location for write
			b += len_written;
		}
	}
	return n;
}

//Handler for process the SIGCHLD when child is terminated 
void sig_handling(int sig) {
	int status;
	pid_t pid;
	if(sig == SIGCHLD)
	{
		//waitpid to kill zombie
		pid = waitpid(-1,&status,WNOHANG);
		if(WIFEXITED(status))
		{
			printf("process %d exited,return value=%d\n",pid,WEXITSTATUS(status));
		}
	}
}

int echo_new(int sockfd) {
	int len;
    char str[MAXLINE + 1];
	memset(str, 0, STR_SIZE);
	
    //Receive message from client.
	
    while(1){
		//len = read(sockfd, str, STR_SIZE);		
		len = readline(sockfd, str, STR_SIZE);
		//printf("len: %d\n",len);
		//str[STR_SIZE]='\0';
		if(len < 0){
			printf("Unable to receive message from client.\n");
			break;
		}
		else if(len == 0) {
			printf("Client type EOF, service terminate.");
			break;
		}
		else{
			char *temp = (char *)malloc((MAXLINE + 1) * sizeof(char));
			strcpy(temp, str);
			temp = trim(temp);
			//printf("temp: %s\n", temp);
			//printf("is quit: %d\n", strcmp(temp, "quit"));
			if (strcmp(temp, "\0") != 0){
				//printf("len: %d\n",len);
				printf("Message from client: %s\n", temp);
				writen(sockfd, str, len);
			}
			if (strcmp(temp, "quit") == 0 ) {
				break;
			}
		}
	}
	
	
	return 0;
}
//Trim a string. remove the head and tail of '\0' '\n' '\t' '\r'
char* trim(char *str) {
	int first = -1; 
	int last = -1;
	for (int i = 0; str[i] != '\0'; i++) {
		if (str[i] != ' ' && str[i] != '\t' && str[i] != '\n' && str[i] != '\r'){ 
			first = i;
			break;
		}
	}
	if (first == -1){ 
		str[0] = '\0';
		return str;
	}
 
	for (int i = first; str[i] != '\0'; i++){
		if (str[i] != ' ' && str[i] != '\t' && str[i] != '\n' && str[i] != '\r') {
			last = i;
		}
	}

	str[last + 1] = '\0';
	return str + first;
}