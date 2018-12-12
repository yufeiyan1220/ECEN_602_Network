all: proxy client proxy_bonus

proxy: proxy.c
	mkdir -p server
	g++ proxy.c -o ./server/proxy
	
proxy_bonus: proxy_bonus.c
	mkdir -p server_bonus_test
	g++ proxy_bonus.c -o ./server_bonus_test/proxy_bonus
	
client: Client.c 
	mkdir -p client_test
	gcc Client.c -o ./client_test/client
	
clean:
	-rm -rf server 
	-rm -rf client_test
	-rm -rf server_bonus_test
	
	
	





