# Multi-threaded-Web-Server

1. Prepare executables of **load_generator** and **server** by running respective makefile in respective directory (simply execute *make* command). <br />
2. start the web server by executing *server* executable with port number as argument <br />
3. generate load on the web server by executing *load_generator* executable with hostname, server port number, number of concurrent users, think time (in seconds) and test duration (in seconds) as argument <br />

For example: <br />
./server \<port number\> <br />
followed by <br />
./load_generator \<hostname\> \<server port\> \<number of concurrent users\> \<think time (in s)\> \<test duration (in s)\>
