//SBCP Message
//Header(Version+Type+Length(whole SBCP messsage))+Attribute[2]
//Attribute includes type+length(attribute)+payload(512bytes)
//server memory has the recorded client list(username and socket number)

#include <iostream> 
#include <netinet/in.h> 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <netdb.h> 
#include <stdlib.h> 
#include <string.h> 
#include <unistd.h> 

using namespace std;

typedef unsigned int uint; 
struct SBCP_Header
{
    uint vrsn : 9; 
    uint type : 7; 
    int length; 
}; 
 
struct SBCP_Attribute
{
    int type; 
    int length; 
    char payload[512]; 
}; 
 
struct SBCP_Message
{
    struct SBCP_Header header; 
    struct SBCP_Attribute attribute[2]; 
}; 

 
struct client_info
{ 
    char username[16]; 
    int fd; 
}; 
 
int total_count = 0;//the total clients in server 
struct client_info *client_list;//the list of all clients with their information 

//checks if username exist already 
//return int  0----username available  1---username existed  2-----reach the maximum 
int check_username(char a[])
{
    int i = 0; 
    for(i = 0; i < total_count ; i++)
{
        if(!strcmp(a,client_list[i].username)){//compare the new username with list
            return 1;
             
        } 
        if(total_count == 3)
{ 
            return 2; 
        } 
    } 
    return 0;}
//send ACK message to the new client 
void sendACK(int accept_socket_fd)
{
 
    struct SBCP_Message ACK_Message; 
    struct SBCP_Header ACK_Message_header; 
    struct SBCP_Attribute ACK_Message_attribute; 
    int i = 0; 
    char temp[512]; 
    temp[0]= '\n';
    temp[1] = (char)(((int)'0')+ total_count);//the order of new client in the server 
    temp[2] = ' '; 
    temp[3] = '\0';// 
    for(i =0; i<total_count-1; i++) 
    { 
        strcat(temp,client_list[i].username);//put recorded username in buffer to fwd to the clients
        strcat(temp, ","); 
    } 
    ACK_Message_header.vrsn=3;//protocol version 
    ACK_Message_header.type=7; //ACK message
    ACK_Message_attribute.type = 4;//usernames list message
    ACK_Message_attribute.length = strlen(temp)+1; 
    strcpy(ACK_Message_attribute.payload, temp); 
    ACK_Message.header = ACK_Message_header; 
    ACK_Message.attribute[0] = ACK_Message_attribute; 
    write(accept_socket_fd,(void *) &ACK_Message,sizeof(ACK_Message));//send the ACK message(client count+usernames list) back to the new client 
} 
 
//send NAK message to the new client 
void sendNAK(int accept_socket_fd,int code)
{
    struct SBCP_Message NAK_Message; 
    struct SBCP_Header NAK_Message_header; 
    struct SBCP_Attribute NAK_Message_attribute; 
    char temp[128]; 
 
    NAK_Message_header.vrsn =3;
    NAK_Message_header.type =5;// NAK message 
    NAK_Message_attribute.type = 1;//failure reason
 
//code from function checkusername 1--name existed  2---reach the maximum 
   if(code == 1)
{ 
        strcpy(temp,"the username has already existed\n");// 
    } 
    if(code == 2){ 
        strcpy(temp,"the server has reached the maximum client number\n");// 
    } 
    NAK_Message_attribute.length = strlen(temp);
    strcpy(NAK_Message_attribute.payload, temp); 
    NAK_Message.header = NAK_Message_header; 
    NAK_Message.attribute[0] = NAK_Message_attribute; 
    write(accept_socket_fd,(void *) &NAK_Message,sizeof(NAK_Message));//send the NAK message to the new client 
    close(accept_socket_fd);//close this client socket 
 
} 
//return the socket number after init 
int socketinit(char const *argv[])
{ 
    struct sockaddr_in servAddr; 
    struct hostent* hostaddr; 
    int listening_sockfd; 
    listening_sockfd = socket(AF_INET,SOCK_STREAM,0);//create the server socket 
    cout<<"server socket created!"<<endl; 
    bzero(&servAddr,sizeof(servAddr)); 
    servAddr.sin_family = AF_INET; 
    hostaddr = gethostbyname(argv[1]); 
    memcpy(&servAddr.sin_addr.s_addr, hostaddr->h_addr,hostaddr->h_length); 
    servAddr.sin_port = htons(atoi(argv[2])); 
    bind(listening_sockfd, (struct sockaddr*)&servAddr, sizeof(servAddr)); 
    cout<<"Server socket binded!"<<endl; 
    listen(listening_sockfd, 99); 
    cout<<"Server is listening!"<<endl; 
    return listening_sockfd; 
} 
 //broadcast the message to all clients except one
void broadcast_all(struct SBCP_Message broadcast_message,fd_set recorded_client_fd_list,int listening_sockfd,int accept_socket_fd, int latest_fd)
{ 
    for(int j = 0; j <= latest_fd; j++) 
    { 
        if (FD_ISSET(j, &recorded_client_fd_list)) //check the broadcasting fd is valid or not 
        {  
            if (j != listening_sockfd && j != accept_socket_fd)//dont broadcast to the original sender and the server 
            { 
                if ((write(j,(void *) &broadcast_message,sizeof(broadcast_message))) == -1) 
                { 
                    cout<<"the server cannot broadcast message"<<endl; 
                } 
            } 
        } 
    } 
 
} 
int main(int argc, char const *argv[]) 
{ 
    struct SBCP_Message SEND_message; //message received from clients
    struct SBCP_Attribute SEND_message_attribute; //attribute
    struct SBCP_Message broadcast_message; //message used to broadcast
    int accept_socket_fd; //accept socket 
    int username_status = 0; //
    struct sockaddr_in client_channel_info; 
    fd_set recorded_client_fd_list;//set of file description of clients in the server 
    fd_set monitoring_fd_list;//fd being monitored 
    int latest_fd;//the latest client socket 
    int temp; 
    int x,y; 
    int current_fd; 
    int index_r; 
    int client_limit=0; 
    int listening_channel =  socketinit(argv);//socket init, the server is listening to the socket now. 
    client_limit=atoi(argv[3]);//the maximum number of clients 
    client_list= (struct client_info *)malloc(client_limit*sizeof(struct client_info));//set engough space for the client list 
    FD_SET(listening_channel, &recorded_client_fd_list);//put the server socket into our local recorded list 
    latest_fd = listening_channel;//the initial value of our latest socket is the server socket 
    while(1) 
    { 
        monitoring_fd_list = recorded_client_fd_list;//set the monitoring descriptors list as what we recorded in local 
        select(latest_fd+1, &monitoring_fd_list, NULL, NULL, NULL);
        for(current_fd=0 ; current_fd<=latest_fd ; current_fd++)//the server will scan all fd to see which fd has made changes in the monitoring fd list. 
            { 
                if(FD_ISSET(current_fd, &monitoring_fd_list))//if any change in our monitoring fd list happens 
                { 
                    if(current_fd == listening_channel)//if the change comes from our server socket, that means a new socket coming to connect, which is the new client want to JOIN 
                    { 
                        int client_channel_len = sizeof(client_channel_info); 
                        accept_socket_fd = accept(listening_channel,NULL,NULL);
                        if(accept_socket_fd < 0)
                            {cout<<"FAIL: server cannot accept"<<endl;} 
                        else 
                        { 
                            temp = latest_fd;//to record the latest_fd 
                            FD_SET(accept_socket_fd, &recorded_client_fd_list);//put the new client socket fd into our local recorded client fd list 
                            if(accept_socket_fd > latest_fd)
                            {//to make sure this is the largest fd we have now 
                                latest_fd = accept_socket_fd;
                            } 
                            struct SBCP_Message JOIN_Message; 
                            struct SBCP_Attribute JOIN_MessageAttribute; 
                            char index_msg[16]; 
                            read(accept_socket_fd,(struct SBCP_Message *) &JOIN_Message,sizeof(JOIN_Message));//read what's the message from the client 
                            JOIN_MessageAttribute = JOIN_Message.attribute[0];//get the first attribute in the message, which will include the username 
                            strcpy(index_msg, JOIN_MessageAttribute.payload);//only operate the message in the index_msg, so the message in the JOIN_Message would not change 
                             
                            //check the user name has existed? 
                            username_status = check_username(index_msg); 
                            if(username_status == 0)//there's no username existed, so this username is availeble 
                            { 
                                strcpy(client_list[total_count].username, index_msg);//update the new client into the client list 
                                client_list[total_count].fd = accept_socket_fd; 
                                total_count = total_count + 1;//our total count of the clients + 1 
                                 
                                //send ACK message to this client 
                                sendACK(accept_socket_fd); 
 
                                //broadcast ONLINE message to all clients except this client 
                                cout<<"New client in the chat room :"<<client_list[total_count-1].username<<endl; 
                                broadcast_message.header.vrsn=3;
                                broadcast_message.header.type=8;//Online message 
                                broadcast_message.attribute[0].type=2;//type 2 attribute means username in the payload 
                                strcpy(broadcast_message.attribute[0].payload,client_list[total_count-1].username); 
                                broadcast_all(broadcast_message,recorded_client_fd_list,listening_channel,accept_socket_fd, latest_fd); 
                            } 
                            //maximum number reached or existed username
                            else 
                            { 
                                if(username_status == 2){ 
                                    cout<<"Maximum reached, Close the client socket"<<endl; 
                                    sendNAK(accept_socket_fd,2); 
                                } 
                                else{ 
                                    cout<<"Username already exists. Close the client socket"<<endl; 
                                    sendNAK(accept_socket_fd, 1); // send NAK message to client, 1 is for the flag of username comflict 
                                } 
                                latest_fd = temp;//return the latest fd to the previous one 
                                FD_CLR(accept_socket_fd, &recorded_client_fd_list);//clear accept_socket_fd out of the recorded client list 
                                 
                            } 
                        } 
                    } 
                    //not a new client, the client in the server send some message or quit the chat
                    else 
                    { 
                        //client quit the chat
                        if ((index_r=read(current_fd,(struct SBCP_Message *) &SEND_message,sizeof(SEND_message))) <= 0) 
                        {//the error or 0 situation will cause the server close the socket 
                            if (index_r == 0) //once client close socket, the server will receive 0 from the client 
                            { 
                                //broadcast the OFFLINE message to all other clients 
                                for(y=0;y<total_count+1;y++)//check all the clients to see the username corresponding to this client. 
                                { 
                                    if(client_list[y].fd==current_fd) 
                                    { 
                                        broadcast_message.attribute[0].type=2;//means the message attribute payload is username 
                                        strcpy(broadcast_message.attribute[0].payload,client_list[y].username); 
                                        cout<<"client:" << client_list[y].username<<"left"<<endl; 
                                    } 
                                } 
                                 
                                broadcast_message.header.vrsn=3; 
                                broadcast_message.header.type=6;//offline message 
                                broadcast_all(broadcast_message,recorded_client_fd_list,listening_channel,current_fd, latest_fd); 
                                 
                            }
                           //wrong message format cause the socket close 
                           else if(index_r < 0)  
                            { 
                                cout<<" the server cannot read"<<endl; 
                            } 
                            close(current_fd); // close the client socket 
                            FD_CLR(current_fd, &recorded_client_fd_list); // remove from recorded_client_fd_list set 
                             
                            for(x=current_fd;x<total_count;x++)//left move the following fd after removing the current fd 
                            { 
                                client_list[x]=client_list[x+1]; 
                            } 
                            total_count--;//the total number of clients -1 
                        }
                         
                        //the message is coming from one client 
                        else 
                        { 
                            SEND_message_attribute = SEND_message.attribute[0]; 
                            broadcast_message=SEND_message;//copy the whole message which is going to FWD 
                            broadcast_message.header.type=3;//FWD
                            broadcast_message.attribute[1].type=2;//username
                            int payloadLength=0; 
                            char temp[16]; 
                            payloadLength=strlen(SEND_message_attribute.payload); 
                            strcpy(temp,SEND_message_attribute.payload); 
                            temp[payloadLength]='\0';//add an ending mark on the string 
 
                             
                            for(y=0;y<total_count;y++)//to find the corresponding username of this fd 
                            { 
                                if(client_list[y].fd==current_fd) 
                                { 
                                    strcpy(broadcast_message.attribute[1].payload,client_list[y].username); 
                                    cout<<"The message from client :"<<client_list[y].username<<" "<<temp<<endl; 
                                } 
                            } 
                            broadcast_all(broadcast_message,recorded_client_fd_list,listening_channel,current_fd, latest_fd); 
                             
                        } 
                    } 
                }
            }
        } 
     
    close(listening_channel); 
 
    return 0;
}
