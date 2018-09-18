ECEN 602 Feiyan Yu

Contribution:  
For groupwork, I coded the client2 and server. I made a lot of tries to create socket server and socket client. 

My experience: 
This assignment provides a good way for us to have a real understanding of the socket programming. I trained my C++ programming ability and got familiar with Linux Operating. I also learned how to use GDB to debug.

My first version is to echo a single line of message by a child process. The client need to reconnect the server after it finishes sending a line. This actually works, but not a good method because the server need to create a child for a single line. 

Usage

Server: 
./server	any port number bigger than 1023

Client: 
./client1	IPv4 address of the server	the port number you put the server listen to
then  input string from keyboard.
./client2  IPv4 address of the server	the port number you put the server listen to
then  input string from keyboard.
./client3  IPv4 address of the server	the port number you put the server listen to
then  input string from keyboard.

Architecture

Client:
1.create a socket and initialize it
2.get the Ipv4 address and port number
3.connect server
4.input string from keyboard to the socket
5.ready to receive the character from the socket
6.show the echo on screen


Server:
1.create a socket and initialize it
2.get the port number 
3.bind, listen and ready to accept and fork.
4.readline and writen contribute the echo.
5.check for EOF and when that happens kill the childprocess

Details:

Client:
1. int readline (int sockfd, char* buffer, int n): 
Read a line from 'sockfd', and put the string in buffer. Using 'read' function to get 1 character a time until '\n', and add a '\0' as mark of termination for a string.
Return the length of buffer.

2. int writen (int sockfd, char* buffer, int n):
Write n character to buffer from 'sockfd', terminated when have read n character or read a '\0'.
Return n if writen successfully(actually no use). 

3.char* trim(char *str):
Remove the blank part of head a tail of a string. 
It is used to process the situation when user type something like "  quit	" to close the client. This function could return "quit". 


Server:
1. int readline (int sockfd, char* buffer, int n):
Same as client.

2. int writen (int sockfd, char* buffer, int n):
Same as client.

3. void sig_handling(int sig):
To process the zombie process, using 'waitpid' function. This function is set to a sigaction struct.

4. int echo_new(int sockfd);
The name is 'echo_new' because this is the second version of 'echo' function.
Using 'readline' to read the message from client, save in buffer. And if there is no 'quit' in buffer, use 'writen' function to send message in buffer back to client.
Return 0 if exit successfully.

5. char* trim(char *str);
Same as client.

Errta: 
In first version, I could not using a loop in echo. I could just reconnect when the same client send message. By debugging, I found the close(client_socket) is the reason. So echo_new is created.
(So many bugs I met, so I did not present other details. Some of the bugs are easy or stupid problems.)