#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <dirent.h>
#include <pthread.h>
#include <time.h>
#include <fcntl.h>

#define CacheNum 10
#define ONEDAY 86400  // 86400 seconds in one day
#define ONEMONTH 2678400  // 2678400 seconds in one month
#define BUFFERSIZE 512
#define PORT "80"

struct Cache{
	char filename[50];   //refer to the filename
	struct tm* Date;
	struct tm* Expires;
	struct tm* Last_Modified;
};

struct Cache CacheTable[CacheNum];

int parseHTTPheader(char buffer[],char* host,char* path,char* URL){
	char method[10];
	char protocol[10];
    int num;
    

    num = sscanf(buffer,"%s %s %s",method,URL,protocol);
    if (strcmp(method, "GET")!=0) return -1;
    if (strcmp(protocol, "HTTP/1.0")!=0) return -1;
    int hostStartIndex;
    int hostEndIndex;
    if (memcmp(URL,"http://",7) == 0) {
    	hostStartIndex = 7;
    }else if (memcmp(URL,"https://",8) == 0){
    	hostStartIndex = 8;
    }else{
    	printf("invalid HTTP request: URL error.\n");
    	return -1;
    }
    printf("received URL is : %s\n",URL);
    int i = hostStartIndex;
    while (URL[i] != '/'){
    	i++;
    }
    hostEndIndex = i;

    for (i = hostStartIndex;i < hostEndIndex; i++){
    	host[i-hostStartIndex] = URL[i];
    }
    host[i-hostStartIndex] = '\0';

    int length = strlen(URL);
    for (i = hostEndIndex+1;i<length;i++){
    	path[i-hostEndIndex-1] = URL[i];
    }
    path[i-hostEndIndex-1] = '\0';
    return 0;
}

void buildHTTPheaderBonus(char buffer[],char* host,char* path,int index) {
	if(index == -1) {
		sprintf(buffer,"GET /%s HTTP/1.0\r\nHost: %s\r\n\r\n", path,host);
		return;
	}
	char time_string[100];
	struct tm* query_time;
	if(CacheTable[index].Expires != NULL) {
		query_time = CacheTable[index].Expires;
	}
	else if(CacheTable[index].Last_Modified != NULL){
		query_time = CacheTable[index].Last_Modified;
	}
	else if(CacheTable[index].Date != NULL){
		query_time = CacheTable[index].Date;
	}
	else {
		sprintf(buffer,"GET /%s HTTP/1.0\r\nHost: %s\r\n\r\n", path,host);
		return;
	}
    time_t gmt_time = mktime(query_time);
    strftime(time_string, 100, "%a, %d %b %Y %H:%M:%S GMT", gmtime(&gmt_time));
    
    sprintf(buffer,"GET /%s HTTP/1.0\r\nHost: %s\r\nIf-Modified-Since: %s\r\n\r\n", path, host, time_string);
	printf("The request contains LMS:\n%s\n", buffer);
}

int CheckCache(struct Cache CacheTable[],char* file,int* CurrentCacheNum){
	for (int i =0;i<*CurrentCacheNum;i++){
		if (strcmp(file,CacheTable[i].filename)==0){
			return i;
		}
	}
	return -1;
}

void updateCache(struct Cache CacheTable[],struct Cache Replace,int index,int* CurrentCacheNum){
    for(int i=index; i< *CurrentCacheNum-1;i++){
    	CacheTable[i] = CacheTable[i+1];
    }
    CacheTable[*CurrentCacheNum-1] = Replace;
}

char* createfile(char* host,char* path){
	char *result =(char*)malloc(strlen(host)+strlen(path)+2);//+1 for the null-terminator
    //in real code you would check for errors in malloc here
 
    strcpy(result, host);
    strcat(result, "_");
    strcat(result, path);
    *(result+strlen(host)+strlen(path)+1) = '\0';
    for(int i=0; i<strlen(result); i++) {
        if(*(result+i) == '/') {
            *(result+i) = '_';
        }
    }
    return result;
}

int processing(int clientsocket,struct Cache CacheTable[],int* CurrentCacheNum,char* host,char* path){
	char* file = createfile(host,path);
	int index = CheckCache(CacheTable,file,CurrentCacheNum);
	int rv;
	ssize_t received_count;
	int sockfd;
	char recv_buffer[BUFFERSIZE];
	char request[100];
	struct addrinfo hints, *servinfo, *p;
	struct Cache newCache;
	memset(&newCache, 0, sizeof(struct Cache));
	memset(&hints, 0, sizeof hints);	// Reset to zeros
	hints.ai_family = AF_UNSPEC;			
	hints.ai_socktype = SOCK_STREAM;	// TCP
	if((rv = getaddrinfo(host,PORT,&hints,&servinfo))==-1){
		fprintf(stderr,"getaddrinfo: %s\n", gai_strerror(rv));
		return 0;
	}
	for(p = servinfo;p!=NULL;p = p->ai_next){
		if ((sockfd = socket(p->ai_family,p->ai_socktype,p->ai_protocol))==-1){
			perror("server: Socket");
			continue;
		}

		if (connect(sockfd,p->ai_addr,p->ai_addrlen) == -1){
			close(sockfd);
			perror("server:connect");
			continue;
		}
		break;
	}
	freeaddrinfo(servinfo);
	if( p == NULL) {
		fprintf(stderr,"server: failed to connect\n");
		return 0;
	}
	
	//int flag = 0;    //falg is 1 if the URL needs to be cached, 0 if both expiry and last-modified is missed
	ssize_t send_count = 0;
	buildHTTPheaderBonus(request,host,path,index);
	//printf("hello123\n");
	if ((send_count = send(sockfd,request,strlen(request),0)) == -1){
		perror("send request:");
		close(sockfd);
		return 0;
	}
	printf("request sent length: %zd\n",send_count);

	
	received_count = recv(sockfd,recv_buffer,BUFFERSIZE,0);
	printf("%s\n", recv_buffer);
	if (recv_buffer[9] == '4' && recv_buffer[10] == '0' && recv_buffer[11] == '4'){
		printf("file not found.\n");
		close(sockfd);
		return 1;
	}
	else if (recv_buffer[9] == '3' && recv_buffer[10] == '0' && recv_buffer[11] == '4'){
		printf("file not stale, just use it");
		char fileBuffer[BUFFERSIZE];
		int cached_file_fd = open(file, O_RDONLY);
		ssize_t file_read_count	= read(cached_file_fd, fileBuffer, BUFFERSIZE);
		if(file_read_count < 0) {
			perror("file read error");
			return 0;
		}         
		while(file_read_count > 0 ) {
			ssize_t send_count = send(clientsocket, fileBuffer, file_read_count,0);
			if(send_count < 0) {
				perror("send to client error");
				return 0;
			}
			memset(&fileBuffer, 0, sizeof (fileBuffer));
			file_read_count	= read(cached_file_fd, fileBuffer, BUFFERSIZE);        
		}
			// Close the cached file
		close(cached_file_fd);
		//update the cache table;
		updateCache(CacheTable,CacheTable[index],index,CurrentCacheNum);
		return 1;
	}
	
	FILE* fp;
	if (index != -1){
		remove(file);
	}

	fp = fopen(file,"w");
	fwrite(recv_buffer,1,received_count,fp);
	
	const char* ptr = recv_buffer;
	while (ptr[0]) {
		char buffer[100];
		int n;
		sscanf(ptr, " %99[^\r\n]%n", buffer, &n); // note space, to skip white space
		ptr += n; // advance to next \n or \r, or to end (nul), or to 100th char
					// ... process buffer
		if ( memcmp(buffer, "Date: ", 6) == 0) {
						// Parse date first
			size_t date_len = strlen(buffer) + 1 - 6;
			char* date =(char*) malloc((date_len)*sizeof(char));
			*(date+date_len-1) = '\0';
			strncpy(date, buffer+6, date_len-1);
			newCache.Date = (struct tm*)malloc(sizeof(struct tm));
			strptime(date, "%a, %d %b %Y %X %Z", newCache.Date);
		} else if ( memcmp(buffer, "Expires: ", 9) == 0) {
			size_t date_len = strlen(buffer) + 1 - 9;
			char* date = (char*)malloc((date_len)*sizeof(char));
			*(date+date_len-1) = '\0';
			strncpy(date, buffer+9, date_len-1);
			newCache.Expires = (struct tm*)malloc(sizeof(struct tm));
			strptime(date, "%a, %d %b %Y %X %Z", newCache.Expires);
		} else if ( memcmp(buffer, "Last-Modified: ", 15) == 0) {
			size_t date_len = strlen(buffer) + 1 - 15;
			char* date = (char*)malloc((date_len)*sizeof(char));
			*(date+date_len-1) = '\0';
			strncpy(date, buffer+15, date_len-1);
			newCache.Last_Modified =(struct tm*) malloc(sizeof(struct tm));
			strptime(date, "%a, %d %b %Y %X %Z", newCache.Last_Modified);
		}
	}

	for(int k = 0;k<strlen(file);k++){
		newCache.filename[k] = file[k];
	}
	printf("new filename: %s\n", newCache.filename);
	memset(recv_buffer,0,BUFFERSIZE);

	while((received_count = recv(sockfd,recv_buffer,BUFFERSIZE,0)) > 0){
		fwrite(recv_buffer,1,received_count,fp);
		memset(recv_buffer,0,BUFFERSIZE);
	}
	fclose(fp);
	printf("receive successfully from Web server.\n");
	int Sendfd = open(file, O_RDONLY);

	while (1){
		int send_size = read(Sendfd, recv_buffer, BUFFERSIZE);
		
		//printf("%d\n", send_file_size);
		if(send_size < 0){
			perror("Send to client:"); 
			return 0;       
		}
		else if(send_size == 0) break;
		
		send(clientsocket, recv_buffer, send_size, 0);
		memset(recv_buffer, 0, BUFFERSIZE);
	}
	
	if (*CurrentCacheNum < CacheNum && index == -1){
		printf("the new website is not in Cache and Cache Number is below 10, so add it\n");
		CacheTable[*CurrentCacheNum] = newCache;
		*CurrentCacheNum = *CurrentCacheNum + 1;
	}
	else if (*CurrentCacheNum == CacheNum && index == -1) {
		printf("The cahe is full, update it.\n");
		updateCache(CacheTable,newCache,0,CurrentCacheNum);
	}
	else{
		printf("replace staled cache entry.\n");
		updateCache(CacheTable,newCache,index,CurrentCacheNum);
	}
	
	printf("CurrentNum: %d\n", *CurrentCacheNum);
	for(int i = 0; i < *CurrentCacheNum; i++) {
		printf("filename of %d :%s\n", i, CacheTable[i].filename);
	}
	free(file);
	close(Sendfd);
	close(sockfd);
    return 1;	
}

int main(int argc,char** argv){
	fd_set master;
	fd_set read_fds;
	int fdmax;
	int listener;
	int clientsocket;
	struct sockaddr_storage remoteaddr; // client address
    socklen_t addrlen;
    char request_buf[100];// buffer for client data
    memset(&request_buf,0,sizeof(request_buf));
    char remoteIP[INET6_ADDRSTRLEN];
    ssize_t received_count;

    int yes=1;
    int i,rv;

    int* CurrentCacheNum = (int*)malloc(sizeof(int));
    *CurrentCacheNum = 0;
    struct addrinfo hints, *servinfo, *p;

    FD_ZERO(&master); // clear the master and temp sets
    FD_ZERO(&read_fds);

    /*	This part checks the input format and record the assigned port number		*/
    if(argc != 3) {
        fprintf(stderr, "Number of arguments error, expected: 3, got: %d\n", argc);
        exit(1);
    }

    /* This part do the IP address look-up, creating socket, and bind the port process */

    memset(&hints, 0, sizeof (hints));	// Reset to zeros
    hints.ai_family = AF_UNSPEC;			// use IPv4 for HW1
    hints.ai_socktype = SOCK_STREAM;	// TCP
    hints.ai_flags = AI_PASSIVE; 		// use my IP
    if ((rv = getaddrinfo(argv[1], argv[2], &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all the results and bind to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
        listener = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (listener < 0) {
            continue;
        }

        // lose the pesky "address already in use" error message
        if (setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes,sizeof(int)) == -1) {
            perror("setsockopt");
            exit(1);
        }
        if (bind(listener, p->ai_addr, p->ai_addrlen) < 0) {
            close(listener);
            perror("server: bind");
            continue;
        }
        break;
    }

    freeaddrinfo(servinfo); // all done with this structure
    if (p == NULL) {
        fprintf(stderr, "server: failed to bind\n");
        exit(1);
    }

    /* This part starts the listening process 	*/
    if (listen(listener,CacheNum) == -1) {
        perror("listen");
        exit(2);
    }
    printf(" the proxy server is now ready to receive!\n");
    // add the listener to the master set
    FD_SET(listener, &master);

    // keep track of the biggest file descriptor
    fdmax = listener; // so far, it's this one
    // Main loop

    while(1) {
        read_fds = master; // copy it

        if (select(fdmax+1, &read_fds, NULL, NULL, NULL) == -1) {
            perror("select");
            exit(4);
        }

        if (FD_ISSET(listener, &read_fds)) {
            // handle new connections
            addrlen = sizeof (remoteaddr);
            clientsocket = accept(listener, (struct sockaddr *) &remoteaddr, &addrlen);
            if (clientsocket == -1) {
                perror("accept");
            } else {
                FD_SET(clientsocket, &master); // add to master set
                if (clientsocket > fdmax) { // keep track of the max
                    fdmax = clientsocket;
                }
                continue;
                
            }
        }
        
        // run through the existing connections looking for data to read
        // Version 1: Try to recv all server results on one trigger event   =>  Success
        for(i = 0; i<= fdmax; i++) {
        	if (FD_ISSET(i, &read_fds) && i != listener) {          
                if(clientsocket == i){
                    received_count = recv(clientsocket, request_buf, sizeof request_buf, 0);
                    if(received_count <= 0) {
                    // got error or connection closed by client clean up resources
                        if(received_count < 0) {
                            perror("Proxy: recvFromClient");
                        }
                    }     
                    // Version 1: Send GET request & receive data
                    char* host = (char*)malloc(20*sizeof(char));
                    char* path = (char*)malloc(20*sizeof(char));
                    char* URL = (char*)malloc(40*sizeof(char));
                    
                    if(parseHTTPheader(request_buf,host,path,URL) == -1) {
                        perror("Proxy: parseHTTP");
                    }
                    // Print Out Host & Resource
                    printf("Host:%s\n", host);
                    printf("Path:%s\n", path);
                    // Check Cache 1st, then send GET
                    if(processing(clientsocket,CacheTable,CurrentCacheNum,host,path) == 1){
                    	printf("Succesfully transmit data to client\n");              
                    }
                    close(clientsocket);
                    FD_CLR(clientsocket,&master);
                    clientsocket = 0;
                    memset(&request_buf,0,sizeof(request_buf));
                    free(host);
                    free(path);
                    free(URL);
                    
                }
                
            }
        }           
    }
    close(listener);        	
}

