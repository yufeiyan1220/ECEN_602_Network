//SBCP Message
//Header(Version+Type+Length(whole SBCP messsage))+Attribute[2]
//Attribute includes type+length(attribute)+payload(512bytes)

#include <iostream> 
#include <netinet/in.h> 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <netdb.h> 
#include <stdlib.h> 
#include <string.h> 
#include <unistd.h> 
#include <sys/time.h> 
#include <error.h>
using namespace std;

typedef unsigned int uint;
 
//SBCP Header 
struct HeaderSBCP{ 
    uint vrsn : 9; 
    uint type : 7; 
    int length; 
}; 
 
//SBCP Attribute 
struct AttributeSBCP 
{ 
    int type; 
    int length; 
    char payload[512]; 
}; 
 
//SBCP Message 
struct MsgSBCP 
{ 
    struct HeaderSBCP header; 
    struct AttributeSBCP attribute[2]; 
}; 
 
//read message from the server 
int getServerMsg(int sockfd){ 
 
    struct MsgSBCP serverMsg; 
    int status = 0; //to mark the message which needs to be care
 
    int k=(read(sockfd, (struct MsgSBCP *) &serverMsg, sizeof(serverMsg))); 
    if(k==-1)
    {
cout<<"error"<<endl;}
else{ 
    if (serverMsg.header.type == 3) //display the client message from other users
    { 
     if ((serverMsg.attribute[0].payload != NULL || serverMsg.attribute[0].payload != '\0') 
      && (serverMsg.attribute[1].payload != NULL || serverMsg.attribute[1].payload != '\0') 
      &&  serverMsg.attribute[0].type == 4 && serverMsg.attribute[1].type == 2) 
 {       
cout<<"public chat---"<<serverMsg.attribute[1].payload<<":"<<serverMsg.attribute[0].payload<<endl;
 } 
        status=0; 
    } 
 
    if (serverMsg.header.type == 5) //displya the NAK
    { 
     if ((serverMsg.attribute[0].payload != NULL || serverMsg.attribute[0].payload != '\0') 
      && serverMsg.attribute[0].type == 1) 
       { 
                cout<<"NAK message from server"<<serverMsg.attribute[0].payload<<endl; 
       } 
       status=1; 
    } 
 
    if (serverMsg.header.type == 6) //display the offline message
    { 
 if ((serverMsg.attribute[0].payload != NULL || serverMsg.attribute[0].payload != '\0') 
  && serverMsg.attribute[0].type == 2) 
       { 
        cout<<serverMsg.attribute[0].payload<<" is offline"<<endl;                
       } 
       status=0; 
    } 
 
    if (serverMsg.header.type == 7) //display the ACK message
    {      
 if ((serverMsg.attribute[0].payload != NULL || serverMsg.attribute[0].payload != '\0') 
  && serverMsg.attribute[0].type == 4) 
       { 
cout<< "ACK message from server"<<serverMsg.attribute[0].payload<<endl;                
       } 
       status=0; 
    } 
 
    if (serverMsg.header.type == 8) //display the online message of other users
    { 
 if ((serverMsg.attribute[0].payload != NULL || serverMsg.attribute[0].payload != '\0') 
  && serverMsg.attribute[0].type == 2) 
       { 
cout<<serverMsg.attribute[0].payload<<" is online"<<endl;                
       } 
       status=0; 
    } 
 
    if (serverMsg.header.type == 9) //display the other user is idle
    { 
cout<<serverMsg.attribute[0].payload<<" is Idle"<<endl;
    } 
 }
    return status; 
} 
 
//  send join message to server
void Join(int sockfd, const char *argv[]){ 
 
    struct HeaderSBCP header; 
    struct AttributeSBCP attr; 
    struct MsgSBCP msg; 
    int status = 0; 
 
    header.vrsn = '3'; 
    header.type = '2';
    attr.type = 2; 
    attr.length = strlen(argv[1]) + 1; 
    strcpy(attr.payload,argv[1]); 
    msg.header = header; 
    msg.attribute[0] = attr; 
    write(sockfd,(void *) &msg, sizeof(msg)); 
 
    // wait for the server_reply 
    sleep(1); 
    status = getServerMsg(sockfd); 
    if (status == 1) { 
            close(sockfd); 
    } 
} 
 
 
// keyboard input and send it to the server to wait for the broadcasting 
void chat(int connectionDesc)
{ 
 
    struct MsgSBCP msg; 
    struct AttributeSBCP clientAttr; 
 
    int nread = 0; 
    char temp[512]; 
 
    struct timeval tv; 
    fd_set readfds; 
    tv.tv_sec = 2; 
    tv.tv_usec = 0; 
    FD_ZERO(&readfds); 
    FD_SET(STDIN_FILENO, &readfds); 
 
    select(STDIN_FILENO+1, &readfds, NULL, NULL, &tv); 
    if (FD_ISSET(STDIN_FILENO, &readfds)) 
    { 
 nread = read(STDIN_FILENO, temp, sizeof(temp)); 
        if (nread > 0) 
        { 
         temp[nread] = '\0'; 
        } 
     
     clientAttr.type = 4; 
     strcpy(clientAttr.payload, temp); 
     msg.attribute[0] = clientAttr; 
     write(connectionDesc, (void *) &msg, sizeof(msg)); 
    } 
    else 
    { 
 cout<<"timed out!"<<endl; 
    } 
} 
 
int main(int argc, char const *argv[]) 
{ 
    if (argc == 4) 
    { 
     int sockfd; 
     struct sockaddr_in servaddr; 
     struct hostent* hostaddr; 
     fd_set m; 
     fd_set read_fds; 
     FD_ZERO(&read_fds); 
     FD_ZERO(&m); 
     sockfd = socket(AF_INET, SOCK_STREAM, 0); 
 
     if (sockfd == -1) 
     { 
         cout<<"socket creating failed"<<endl; 
         exit(0); 
     } 
 
     else 
     { 
cout<<"socket created"<<endl;         
     } 
     
     bzero(&servaddr, sizeof(servaddr)); 
     servaddr.sin_family = AF_INET; 
     hostaddr = gethostbyname(argv[2]); 
     memcpy(&servaddr.sin_addr.s_addr, hostaddr->h_addr, hostaddr->h_length); 
     servaddr.sin_port = htons(atoi(argv[3])); 
 
     if (connect(sockfd,(struct sockaddr *)&servaddr, sizeof(servaddr))!=0) 
     { 
cout<<"fail to connect to server"<<endl;         
exit(0); 
     } 
     else 
     { 
cout<<"connected to the server"<<endl;          
         Join(sockfd, argv); 
         FD_SET(sockfd, &m); 
         FD_SET(STDIN_FILENO, &m); 
         for(;;) 
         { 
            read_fds = m; 
            cout<<endl;
 
      if (select(sockfd+1, &read_fds, NULL, NULL, NULL) == -1) 
      { cout<<"error"<<endl;
                 exit(4); 
            } 
 
             if (FD_ISSET(sockfd, &read_fds)) 
             { 
                     getServerMsg(sockfd); 
                } 
  
             if (FD_ISSET(STDIN_FILENO, &read_fds)) 
             { 
 
                        chat(sockfd); 
                } 
 
            } 
 
        } 
    } 
cout<<"client left"<<endl;  
    return 0; 
} 
 
 
