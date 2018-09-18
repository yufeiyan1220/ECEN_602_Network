all: server client1 client2 client3
server: Socket_server_yfy.cpp  
	g++ -o server Socket_server_yfy.cpp 

client1: client_wnz.cpp 
	g++ -o client1 client_wnz.cpp
	
client2: client_yfy.cpp
	g++ -o client2 client_yfy.cpp
	
client3: client_tbq.cpp
	g++ -o client3 client_tbq.cpp


clean:
	rm -rf *.o