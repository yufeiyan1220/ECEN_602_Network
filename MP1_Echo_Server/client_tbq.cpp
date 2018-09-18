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

#define maxlen 20

//describe errors
void err_sys(const char*x){          
	perror(x);
	exit(1);
}

//write from buf to sockfd
int writen(int sockfd,char* buf,int N){   
	int NumLeft=N;
	int Numcount;
	char* ptr=buf;
	while(NumLeft){
		Numcount=write(sockfd,ptr,NumLeft);          
		if(Numcount<0){
			if(errno==EINTR)
				continue;
			else 
				return -1;              //errors occur
		}
		NumLeft-=Numcount;              //number left to write to sockfd
		ptr+=Numcount;                  //pointer ptr pointing to left characters
	}
	return N;                           
}

//read from sockfd to buf
int readline(int sockfd,char* buf,int N){        
	int i;
	int len=0;                         
	int readf;
	char* ptr=buf;
	char c;
	if(N<0||buf==NULL){
		printf("input N or buf error!\n"); //invalid input
	}
	
    //read each character 	
	for(i=1;i<N;i++){
		readf=read(sockfd,&c,1);
		if(readf<0){
			if(errno==EINTR){
				i--;
				continue;
			}
			else 
				return -1;        //errors occur
		}
		else if(readf==0)         //break when '\0' is found
			break;
		else {
			if (len< N - 1) {     
                *ptr=c;				
                len++;				
                ptr++;
            }
            if (c == '\n') {
				break;
			}    
	    }
	}
	*ptr='\0';
	return len;
}
   

int main(int argc,char** argv){
	int sock_client;
	int pton;
	int writeto;
	int readfrom;
	struct sockaddr_in server_addr;
	unsigned short int portnum=0;
	char* port=argv[2];
	char sendbuf[maxlen+1];
	char recvbuf[maxlen+1];
	if(argc!=3){
		printf("command line error!\n");
		exit(1);
	}
	
	pton=inet_pton(AF_INET,argv[1],&server_addr.sin_addr);
	if(pton==0){
		printf("IP address invalid!\n");
		exit(1);
	}
	
	while(*port!='\0'){                     
		if(*port>='0'&&*port<='9'){
			portnum=portnum*10+(*port-'0');
			port++;
		}
		else{
			printf("portnumber error!\n");
			exit(1);
		}
	}
	printf("connecting to %s ; %d\n", inet_ntoa(server_addr.sin_addr), portnum);
	
	sock_client=socket(AF_INET,SOCK_STREAM,0);
	if(sock_client<0){
		printf("socket not created!\n");
		printf("error number is :%d\n",errno);
		err_sys("client:socket");
		exit(1);
	}
	
	memset(&server_addr,0,sizeof(server_addr));                
	server_addr.sin_family=AF_INET;
	server_addr.sin_port=htons(portnum);
	socklen_t server_addr_size = sizeof(server_addr);
	
	if(connect(sock_client,(struct sockaddr*)&server_addr,server_addr_size)==-1){  
		printf("connection failed!\n");
		printf("error number is :%d\n",errno);
		err_sys("client:connect");
		exit(1);
	}
	
	printf("connected to server!\n");
	
	while(fgets(sendbuf,sizeof(sendbuf),stdin)){      
		printf("input a text\n");
		if(feof(stdin)){                                     
		    printf("terminated text transmission!\n");
		    break;
		}
		sendbuf[maxlen]='\0';     
		if(strlen(sendbuf)==sizeof(sendbuf)-1&&sendbuf[maxlen-1]!='\n'){     
		    printf("Exceed limit!\n");
			continue;
		}
		else{ 
		    writeto=writen(sock_client,sendbuf,strlen(sendbuf));
		    if(writeto<0){
			    printf("no sending text!\n");
				break;
			}
		    readfrom=readline(sock_client,recvbuf,maxlen);
			if(readfrom<0){
				printf("read error!\n");
				exit(1);
			}
			printf("message received from server :%s\n",recvbuf);
			
        }
	}
	
	close(sock_client);
	printf("connection is closed!\n");
}
		
		
	
	
		
	
	
		
	