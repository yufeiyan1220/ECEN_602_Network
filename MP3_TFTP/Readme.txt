Member 1 # Ningze Wang UIN: 827006879
Member 2 #Boquan Tao UIN: 627006553 
Member 3# Feiyan Yu UIN:927007279
Oct.29 2018

Contribution :
Member 1:I write the first version of TFTP server, with only RRQ. take part in debugging the final
version of TFTP protocol.
Member 2: I wrote the WRQ for the bonus and did test case with ascii mode.
Member 3: I wrote the final version of RRQ and integrate it with WRQ in main function and I wrote test.c
for test cases.
Experience :
This assignment provides a good way for us to understand the process TFTP protocol.
Usage :
we fix the new port number : 4000 and client need to connect to IP 0.0.0.0.
server: ./server
Client: tftp 0.0.0.0 4000
And for testing, test.c is used to generate test cases. The generated file is txt_out.txt
./test text2018 —test case 1
./test test2017 —test case 2
./test crlf —test case 3
Architecture :
1.initialize the socket and get ready to connect the first client send the request.
2.receive the mode, request type, file name.
3.generate the child process and ephemeral