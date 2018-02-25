=======================================
-------------Read Me File--------------
=======================================

1. Compile
$ make
gcc -Wall -g -c common.c
gcc -Wall -g client.c common.o -o client -pthread
gcc -Wall -g server.c common.o  -o server -pthread

2. Run Server
$ ./server -a 129.120.151.9X -p XXXXX
<ip-address>
- CSE01 	129.120.151.94
- CSE02 	129.120.151.95
- CSE03 	129.120.151.96
- CSE04 	129.120.151.97
- CSE05 	129.120.151.98
- CSE06 	129.120.151.99

<port#> Any open port#

3. ./client -a 129.120.151.9X -p XXXXX
- same ip-address & port# as the server 

4. Handshake from client
> hello <username>
- This registers username 

5. Send Message
> message <message>

6. Send File
> put <filename>

7. Quit [Client]
> quit

8. Quit [Server]
> ^C