
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <signal.h>
#include <fcntl.h>
#define MYPORT "4000" 
// the port users all users know

//define area;
#define RRQ 1
#define WRQ 2
#define DATA 3
#define ACK 4
#define ERR 5

#define MAX_BUFFER_LEN 517
#define MAX_DATA_LEN 512
#define MAX_RETRY 10
#define MAX_CLIENTS 1024
static char* OCTET = "octet";
static char* NETASCII = "netascii";

struct parse_data
{
	uint16_t RQ_type;
	char* mode;
	char filename[MAX_DATA_LEN];
};
/*
struct packet
{
	uint16_t blockid;
	uint16_t seqNum;
	char filename[MAX_DATA_LEN];
};*/
int generateERR(char buffer[], char* errorMessage, int errorCode) {
   buffer[0]=(char)0;
   buffer[1]=(char)ERR;
   buffer[2]=(char)errorCode >> 8;
   buffer[3] = (char)(errorCode & 0xFF);
   for(int i=0; i < strlen(errorMessage); i++) {
        buffer[4+i] = errorMessage[i];
   }
   return 0;

}

int parseRequest(struct parse_data* parse_type, char buffer[], int dataSize)
{	
	memset(parse_type, 0, sizeof(struct parse_data));
	parse_type->RQ_type = ntohs(*((uint16_t *)buffer));
	if(parse_type->RQ_type != 1 && parse_type->RQ_type != 2) {
		char* errorMessage = "Unacceptable RRQ/WRQ packet.";
		generateERR(errorMessage, strlen(errorMessage) + sizeof(uint16_t)*2 + 1, 4);
		return -1;
	}
    if(dataSize > 9) {
		parse_type->mode = malloc(6 * sizeof(char));
		strncpy(parse_type->mode, buffer + dataSize -6 , 6);
		printf("mode_temp==%s\n",parse_type->mode);
		if(strcmp(parse_type->mode, OCTET) == 0) {
            // mode is correct
            //parse_type->mode = malloc((dataSize - 8) * sizeof(char));
			
            strncpy(parse_type->filename, buffer + 2, dataSize - 8);
        }
		else {
			parse_type->mode = malloc(9*sizeof(char));
            strncpy(parse_type->mode, buffer + dataSize - 9, 9);
            if(strcmp(parse_type->mode, NETASCII) == 0) {
                // mode is correct
				strncpy(parse_type->filename, buffer + 2, dataSize - 11);
            } else {
                return -1;
            }
        }
		return 0;
	}
    return -1;
}


int generateDATA(char dataMsg[], const char fileBuffer[], uint16_t seqNum, ssize_t file_read_count)
{
    dataMsg[0] = (char)0;
    dataMsg[1] = (uint8_t)DATA;
    dataMsg[2] = (uint8_t)(seqNum >> 8);
    dataMsg[3] = (uint8_t)(seqNum & 0xFF);
    for(int i=0; i<file_read_count; i++) {
        dataMsg[4+i] = fileBuffer[i];
    }
    return 0;
}

int parseDATA(char** message, uint16_t* seqNum, const char buffer[], ssize_t dataSize)
{
    if(dataSize >= 4 && buffer[0] == (uint8_t)0 && buffer[1] == (uint8_t)DATA) {
        *message = malloc((dataSize-4)*sizeof(char));
        //printf("buffer[2]:\n %s \n", byte_to_binary(buffer[2]));
        //printf("buffer[3]:\n %s \n", byte_to_binary(buffer[3]));
        *seqNum = (uint16_t) (((uint16_t)buffer[2] * 256) + (uint8_t)buffer[3]);
        // TODO: HERE must manually copy all the bytes, strncpy will not copy eacatly same characters
        // strncpy(*message, buffer+4, dataSize-4);
        for(int i=4;i<dataSize;i++) {
            *(*message+i-4) = buffer[i];
        }
        return 0;
    } else {
        return -1;
    }
}

int generateACK(char ackMsg[], uint16_t seqNum)
{
    ackMsg[0] = (uint8_t)0;
    ackMsg[1] = (uint8_t)ACK;
    ackMsg[2] = (uint8_t)(seqNum >> 8);
    ackMsg[3] = (uint8_t)(seqNum & 0xFF);
    return 0;
}

int parseACK(uint16_t* seqNum, const char buffer[], ssize_t dataSize)
{
    if(dataSize == 4 && buffer[0] == (uint8_t)0 && buffer[1] == (uint8_t)ACK ) {
        *seqNum = (uint16_t) (((uint16_t)buffer[2] * 256) + (uint8_t)buffer[3]);
//        *seqNum = ((uint16_t)buffer[2] << 8) + (uint16_t)buffer[3];
        return 0;
    } else {
        return -1;
    }
}

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}


int readable_timeo(int fd, int sec) {
    fd_set rset;
    struct timeval tv;
    FD_ZERO(&rset);
    FD_SET(fd, &rset);
    tv.tv_sec = sec;
    tv.tv_usec = 0;

    return select(fd+1, &rset, NULL,NULL, &tv);
}

//Global nextchar
char nextchar;

int readNetascii(FILE *fp, char *ptr, int maxnbytes)
{
    for(int count=0; count < maxnbytes; count++) {
        if(nextchar >= 0) {
            *ptr++ = nextchar;
            nextchar = -1;
            continue;
        }

        int c = getc(fp);
        if(c == EOF) {
            if(ferror(fp)) {
                perror("read err on getc");
            }
            return count;
        } else if (c == '\n') {
            c = '\r';
            nextchar = '\n';
        } else if (c == '\r') {
            nextchar = '\0';
        } else {
            nextchar = -1;
        }
        *ptr++ = (char) c;
    }
    return maxnbytes;
}


void sendFile(const char *filename, int client_new_fd, struct sockaddr_storage client_addr, socklen_t addr_len, const char *mode)
{

    // Initialize file descriptors
	//fd binary;
    int fd = -1;
	//fp text file
    FILE* fp = NULL;
    nextchar = -1;
    if(strcmp(mode, OCTET) == 0) {
        fd = open(filename, O_RDONLY);
        if(fd < 0) {
            char* errorMessage = "Error open the file one the server"; 
            int length = sizeof(errorMessage) + sizeof(uint16_t) * 2;
            char buffer[length];                               
            generateERR(buffer, errorMessage, 1);
            sendto(client_new_fd, buffer, length, 0, (struct sockaddr *)&client_addr, addr_len);
            return;
        }
    } else if(strcmp(mode, NETASCII) == 0) {
        fp = fopen(filename, "r");
        if(fp==NULL) {
            char* errorMessage = "No such file one the server"; 
            int length = strlen(errorMessage) + sizeof(uint16_t)*2 + 1 ;
            char buffer[length];                               
            generateERR(buffer, errorMessage, 2);
            sendto(client_new_fd, buffer, length, 0, (struct sockaddr *)&client_addr, addr_len);
            return;
        }
    }
	//total packet = size/512byte
    uint64_t totalPackets = 0;
	//first is from 1 to 65535, other is from 0 to 65535
	int64_t firstRound = 1;
    // Read the First Packet
    char fileBuffer[MAX_DATA_LEN + 1];
    uint16_t seqNum = 1;
    int64_t file_read_count;
    if(strcmp(mode, OCTET) == 0) {
        file_read_count	= read(fd, fileBuffer, MAX_DATA_LEN);
    } else {
        file_read_count = readNetascii(fp, fileBuffer, MAX_DATA_LEN);
    }
	//printf("dataMsg: %d\n", file_read_count);
    // Loop Over All Left
    while(file_read_count >= 0 ) {

        // Send A full DATA packet
        char dataMsg[file_read_count+4];
        if(generateDATA(dataMsg, fileBuffer, seqNum, file_read_count) == 0) {
            int retries = 0;
			//printf("client_new_fd: %d\n", dataMsg);
            sendto(client_new_fd, dataMsg, sizeof dataMsg, 0,
                   (struct sockaddr *)&client_addr, addr_len);
				   
            // Retry
            while(retries <= MAX_RETRY ) {
                if(readable_timeo(client_new_fd, 1) <= 0) {
                    // Timeout
                    if(retries >= MAX_RETRY) {
                        printf("Retry Times for this packet exceed 10. Will close connection\n");
                        char* errorMessage = "Error during get file from the server"; 
                        int length = sizeof(errorMessage) + sizeof(uint16_t) * 2;
                        char buffer[length];                               
                        generateERR(buffer, errorMessage, 3);
                        sendto(client_new_fd, buffer, length, 0, (struct sockaddr *)&client_addr, addr_len);
                        close(client_new_fd);
                        exit(0);
                    }
                    retries ++;
                    printf("Retry Times: %d\n", retries);
					
                    sendto(client_new_fd, dataMsg, sizeof dataMsg, 0,
                           (struct sockaddr *)&client_addr, addr_len);
                } 
				
				else {
                    // We might receive an ACK
					retries = 0;
                    ssize_t received_count;
                    uint16_t received_seq_num;
                    char ackBuf[5];
                    if ((received_count = recvfrom(client_new_fd, ackBuf, 4 , 0, (struct sockaddr *)&client_addr, &addr_len)) == -1) {
                        perror("recvfrom:ACK");
                        exit(1);
                    }

                    if(received_count == 4 && parseACK(&received_seq_num, ackBuf, 4) == 0) {
						//printf("ReceivedSeq: %d\n", received_seq_num);
						//printf("SeqNum: %d\n", seqNum);
                        if(received_seq_num == seqNum) {
                            break;
                        }
                    }
                }
            }
			
			if(seqNum == 65535) {
                // Wrap is needed
                seqNum = 0;
                if(firstRound == 1) {
                    totalPackets += 65535;
                    firstRound = 0;
                } else {
                    totalPackets += 65536;
                }
            } else {
                seqNum ++;
            }
        }
        memset(&fileBuffer, 0, sizeof fileBuffer);
        if(file_read_count == MAX_DATA_LEN) {
            // Keep Sending
            if(strcmp(mode, OCTET) == 0) {
                file_read_count	= read(fd, fileBuffer, MAX_DATA_LEN);
            } else {
                file_read_count = readNetascii(fp, fileBuffer, MAX_DATA_LEN);
            }
        } else {
            // Last Frame
			totalPackets +=seqNum - 1;
            printf("Succeeded sending %llu packets DATA.\n", totalPackets);
            break;
        }

    }
    printf("File sent.\n");
    // Reset this global helper Character after sending each file
    nextchar = -1;
}

int64_t receiveFile(const char *filename, int client_fd, struct sockaddr_storage client_addr, socklen_t addr_len)
{
	//write
    int read_fd = open(filename, O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR);
    int totalBytesRead = 0;
    if(read_fd < 0) {
        // No such file Should Later ERROR packet
        return -1;
    }
    char receiveBuf[MAX_DATA_LEN + 4 + 1];
    uint16_t desiredSeq = 1;
    uint16_t seqNum = 0;
    ssize_t receiveCount;

    // send the first ACK:
    char ackFirst[4];
    if(generateACK(ackFirst, seqNum) == 0) {
        sendto(client_fd, ackFirst, (size_t) 4, 0,
               (struct sockaddr *)&client_addr, addr_len);
    }

    while(1) {
        if ((receiveCount = recvfrom(client_fd, receiveBuf, MAX_DATA_LEN + 4 + 1, 0, (struct sockaddr *)&client_addr, &addr_len)) == -1) {
            perror("recvfrom");
            return -2;
        }

        if(receiveCount > MAX_DATA_LEN+4 || receiveCount < 4) {
            printf("Discard Wrong Packet.\n");
            continue;
        }

        char* data_msg;
        char ackMsg[4];

        if(parseDATA(&data_msg, &seqNum, receiveBuf, receiveCount) == 0) {
            //printf("DesiredSeq: %d\n", desiredSeq);
            //printf("SeqNum: %d\n", seqNum);
            if(desiredSeq == seqNum) {
                // Success Print on screen & send ACK back
                write(read_fd, data_msg, (size_t) (receiveCount - 4));

                if(generateACK(ackMsg, seqNum) == 0) {
                    // Send to Server
                    sendto(client_fd, ackMsg, (size_t) 4, 0,
                           (struct sockaddr *)&client_addr, addr_len);
                }

                totalBytesRead += receiveCount-4;

                if(receiveCount-4 != MAX_DATA_LEN) {
                    // Last frame
                    break;
                } else {
                    if(desiredSeq == 65535) {
                        printf("Current Read Bytes: %d\n", totalBytesRead);
                        desiredSeq = 0;
                    } else {
                        desiredSeq ++;
                    }
                }

            }
        }
        free(data_msg);
        memset(&receiveBuf, 0, sizeof receiveBuf);
    }
    close(read_fd);
    return totalBytesRead;

}

char* concat(const char *s1, const char *s2)
{
    char *result = malloc(strlen(s1)+strlen(s2)+1);//+1 for the null-terminator
    //in real code you would check for errors in malloc here
    strcpy(result, s1);
    strcat(result, s2);
    return result;
}

//Handler for process the SIGCHLD when child is terminated 
void sig_handling(int sig) {
	int status;
	pid_t pid;
	if (sig == SIGCHLD)
	{
		//waitpid to kill zombie
		pid = waitpid(-1, &status, WNOHANG);
		if (WIFEXITED(status))
		{
			printf("process %d exited,return value=%d\n", pid, WEXITSTATUS(status));
		}
	}
}



int clientport[MAX_CLIENTS];
int static Numberofport = 0;
int main(void)
{

    int sockfd=-1;
    int client_new_fd;
	
	for(int i = 0; i < MAX_CLIENTS; i++) {
		clientport[i] = 0;
	}
	
    struct addrinfo hints, *servinfo, *p;
    int rv;
    ssize_t numbytes;
    struct sockaddr_storage client_addr;
    char buf[MAX_BUFFER_LEN];
    socklen_t addr_len;
    char s[INET6_ADDRSTRLEN];
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET; // set to AF_INET to force IPv4
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE; // use my IP
    if ((rv = getaddrinfo(NULL, MYPORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }
    // loop through all the results and bind to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                             p->ai_protocol)) == -1) {
            perror("listener: socket");
            continue;
        }
        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("listener: bind");
            continue;
        }
        break;
    }
    if (p == NULL) {
        fprintf(stderr, "listener: failed to bind socket\n");
        return 2;
    }
    if(sockfd == -1) {
        return 3;
    }
    freeaddrinfo(servinfo);
	printf("server: started.\nlistening on port: %s\nwaiting for connections...\n\n", MYPORT);
    addr_len = sizeof client_addr;

    // Get the pwd
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) != NULL)
        fprintf(stdout, "Current working dir: %s\n", cwd);
    else
        perror("getcwd() error");

	
	//kill the zombies
	/*
	struct sigaction act;
	act.sa_handler = sig_handling;
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;
	sigaction(SIGCHLD, &act, 0);
	*/

    // Server starts here
    while(1) {
        if ((numbytes = recvfrom(sockfd, buf, MAX_BUFFER_LEN - 1, 0, (struct sockaddr *)&client_addr, &addr_len)) == -1) {
            perror("recvfrom");
            exit(1);
        }
        printf("server: got packet from %s\n",
               inet_ntop(client_addr.ss_family,
                         get_in_addr((struct sockaddr *)&client_addr),
                         s, sizeof s));
		
        if(numbytes >= MAX_BUFFER_LEN) {
            continue;
        }
        //buf[numbytes] = '\0';
		
		struct parse_data parse_data_type;
		memset(&parse_data_type, 0, sizeof(struct parse_data));
		//printf("0Successfully get the request!\n");
		int request_type = parseRequest(&parse_data_type, buf, numbytes);
		//printf("1Successfully get the request!\n");
		if(numbytes > 4 && (request_type == 0)) {
			printf("Successfully get the request!\n");
		}
		else {
			perror("Invalid request!\n");
			continue;
		}
		
		char* full_filename;
		
		//printf(full_filename);
        full_filename = concat(concat(cwd, "/"), parse_data_type.filename);
		
		strcpy(parse_data_type.filename,full_filename);
		printf("Filename: %s\n", parse_data_type.filename);
		printf("Mode: %s\n", parse_data_type.mode);
		printf("Type: %d\n", parse_data_type.RQ_type);

        client_new_fd = socket(AF_INET, SOCK_DGRAM, 0);
		Numberofport++;
		int pid  = fork();
        if (pid == 0) { // this is the child process
            // Child process
			
            close(sockfd); // child doesn't need the listener
			int client_no = 0;;
			int i = 0;
			for(; i < Numberofport; i++) {
				if(clientport[i] == 0) break;
			}
			if(i == Numberofport) {
				client_no = Numberofport;
			}
			else {
				client_no = i;
			}
			clientport[client_no] = 5000 + client_no + 1;
			//printf("New port num for client: %d\n", clientport[client_no]);
            struct sockaddr_in my_addr;
            my_addr.sin_family = AF_INET;
            my_addr.sin_port = htons(clientport[client_no]); // short, network byte order
            my_addr.sin_addr.s_addr = inet_addr("0.0.0.0");
            memset(my_addr.sin_zero, 0, sizeof my_addr.sin_zero);
            bind(client_new_fd, (struct sockaddr *)&my_addr, sizeof my_addr);
			
            // Two Modes
            if(parse_data_type.RQ_type == 1) {
				//RRQ
				printf("parse_data_type: RRQ\n");
				//printf("parse_data_type mode: %s\n", parse_data_type.mode);
				sendFile(full_filename, client_new_fd, client_addr, addr_len, parse_data_type.mode);
			
            } else {
				printf("parse_data_type: WRQ");
                int64_t fileSize = receiveFile(full_filename, client_new_fd, client_addr, addr_len);
                if (fileSize == -1) {
                    // Send "No Such File to Client"
					char* errorMessage = "No Such File to Client";
					printf("%s\n", errorMessage);
                                      
                } else {
                    printf("File received. Total file size is %lli bytes. \n", fileSize);
                }
            }
            printf("Closing Connection...\n");
            close(client_new_fd);
			clientport[client_no] = 0;
            exit(0);
        }
        // Parent Process does not need this
		
        close(client_new_fd);
		memset(&buf, 0, sizeof(buf));
		//sleep(1);
    }

    close(sockfd);
    return 0;

}
