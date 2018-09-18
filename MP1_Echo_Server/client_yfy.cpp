#include <stdio.h>
#include <string>
#include <sys/un.h>
#include <sstream>
#include <stdlib.h>
#include <stdio_ext.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>

//STR_SIZE is the maxsize of message, MAXLINE is to give a buffer
#define MAXLINE 100
#define STR_SIZE 50

int readline (int sockfd, char* buffer, int n);
int writen (int sockfd, char* buffer, int n);
char* trim(char *str);

int main(int argc, char** argv){
	
	int sockfd;
	char str[MAXLINE+1];
	struct sockaddr_in servaddr;
	int port_num = 0;
	unsigned int ip;
	
	//command is echo server_address port
	if(argc != 3) {
		printf("command error\n");
		exit(0);
	}
	
	//create a socket
	if ((sockfd = socket(AF_INET,SOCK_STREAM,0)) < 0){
		printf("Socket error\n");
		exit(0);
	}
	
	//Get a unsigned int32 IPv4 address(aton is ok).
	ip = inet_aton(argv[1], &servaddr.sin_addr);
	if (ip == 0){
		printf("IPv4 format invalid!\n");
		exit(0);
	}
	
	//Get port number.
	for (char *p = argv[2]; *p != '\0'; p++) {
		if (*p > '0' || *p < '9') {
			port_num = port_num * 10 + (*p - '0');
		}
		else {
			printf("Invaild port number error\n");
			exit(0);
		}	
	}
	printf("Connecting to %s:%d.", inet_ntoa(servaddr.sin_addr), port_num);
	
	//Socket initialization
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(port_num);
	socklen_t server_addr_size = sizeof(servaddr);
	
	//Connect to server
	int connection_status = connect(sockfd,(struct sockaddr*)&servaddr,server_addr_size);
	if(connection_status < 0){
		printf("Error when connecting to server.\n");
		printf("Error number: %d\n", (int) errno);
		exit(0);
	}
	printf("Server connected.\n");
	char buffer[MAXLINE+1];
	char buffer_recv[MAXLINE+1];
	while(1) {
		
		//Send message to server
		printf("Send Message: ");
		if (fgets(buffer, sizeof(buffer), stdin) == NULL && feof(stdin)) {
			printf("EOF detected, the client will be terminated.");
            break;
		}
		//buffer[strlen(buffer)] = '\0';
		/*
		if(feof(stdin)){
            printf("EOF detected, the client will be terminated.");
            break;
        }*/
		if(strlen(buffer) >= STR_SIZE){     
		    printf("Exceed limit, try again.(50)\n");
			continue;
		}
		int write_to_server = writen(sockfd, buffer, strlen(buffer));
		
		if(write_to_server < 0){
			printf("Error occur when writing.");
			break;
		}
		
		//trim temp and compared to "quit"
		char *temp = (char *)malloc((MAXLINE + 1) * sizeof(char));
		strcpy(temp, buffer);
		temp = trim(temp);
		if(!strcmp(temp,"quit")){
            printf("Quit detected, the client will be terminated.");
            break;
        }
		
		readline(sockfd, buffer_recv, STR_SIZE);
		printf("Echoed message from server: %s\n", buffer_recv);
		
		
	}
	close(sockfd);
	printf("You shut off the connection.\n");
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
		//printf("%d",letter);
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
			//Some letter is read, end with '\0'       
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