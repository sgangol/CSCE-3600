/***********************************************************************
     CSCE 3600:     System Programming                                **
     ASSIGNMENT:    Major Assignment 1                                **
     DESCRIPTION:   Major 1                                           **
                    This program implements client/server model using **
                    Linux sockets for a “messaging system”            **
                                                                      **
					Client.c                                          **
					- Client side of the Program                      **
					The Progrram can:                                 **
                    - Connect up to 4 client/server                   **
                    - Acquire the client ID via a simple handshake    **
                        i.e> hello <username>                         **
                    - Broadcast each client message or file           **
					                                                  **
     DEPENDENCIES:  common.h                                          **
     AUTHORS:       Rawdhah Alshaqaq | Saina Baidar | Srizan Gangol   **
                    Sundus Nasser Said Al Subhi                       **
     DATE:          05/06/2017                                        **
------Read ReadMe.txt & Screenshot for instructions & examples--------**
 ***********************************************************************/
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/sendfile.h>
#include <fcntl.h>

#include "common.h"

static int skt = -1;	//sever connection socket

//find server IP and connect to it
static int connect_to_server(const char * server, const unsigned short port){
	struct addrinfo hints;
    struct addrinfo *result, *rp;

	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family 	= AF_INET;    	/* Allow IPv4 or IPv6 */
	hints.ai_socktype	= SOCK_STREAM; 	/* Datagram socket */
	hints.ai_flags		= 0;
	hints.ai_protocol	= 0;          	/* Any protocol */

	char buf[10];
    snprintf(buf, sizeof(buf), "%u", port);
	int s = getaddrinfo(server, buf, &hints, &result);
	if (s != 0) {
		//fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
		return -1;
	}

	int sfd;
	for (rp = result; rp != NULL; rp = rp->ai_next) {
		sfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
		if (sfd == -1)
			continue;

		if (connect(sfd, rp->ai_addr, rp->ai_addrlen) != -1)
			break;                  /* Success */

		close(sfd);
	}

	if (rp == NULL) {               /* No address succeeded */
		fprintf(stderr, "Could not connect\n");
		return -1;
	}
	freeaddrinfo(result);

	return sfd;
}

//send a file to server
int send_file(const char * filename, const int srv_skt){
	int fd = open(filename, O_RDONLY);	//open file
	if(fd == -1){
		perror("open");
		return -1;
	}

	//find file size
	lseek(fd, 0, SEEK_END);
	long len = lseek(fd, 0, SEEK_CUR);
	lseek(fd, 0, SEEK_SET);

	char header[LINE_LEN+1];

	//make the header
	bzero(header, LINE_LEN);
	int hlen = snprintf(header, LINE_LEN, "%i:%s:%li\n", MSG_FILE, filename, len);

	//first send length and filename
	send(srv_skt, header, hlen, 0);

	//then send data
	off_t offset = 0;
	while(offset < len){
		ssize_t copied = sendfile(srv_skt, fd, (void*)&offset, len-offset);
		if(copied == -1){
			perror("sendfile");
			break;
		}
		offset += copied;
	}

	close(fd);
	return 0;
}

//Function for getting user input, to be sent to server
static void * char_input(void * param){
	char line[LINE_LEN+1];	//buffer for input

	printf("> ");
	while(fgets(line, LINE_LEN, stdin) != NULL){	//get input

		int len = strlen(line);
		if(len > 1){
			if(send(skt, line, len, 0) != len)	//send it to server
				perror("send");

			line[--len] = '\0';	//remove the newline
			if(strncmp(line, "put",  3) == 0){
				send_file(&line[4], skt);
			}
			if(strncmp(line, "quit", 4) == 0)	//if command is exit
				break;
		}

		printf("> ");
	}
	pthread_exit(NULL);
}

//receive a file from server and save it
static int receive_file(char * header, const int skt){
	//get header variables
	char * filename = strtok(header, ":");
	char * sflen = strtok(NULL, ":");
	int flen = atoi(sflen);

	//open file
	int fd = open(filename, O_CREAT | O_WRONLY, 0666);
	if(fd == -1){
		return 1;
	}

	//receive file contents
	char buf[100];
	int offset = 0;
	while(offset < flen){
		int bytes = read(skt, buf, sizeof(buf));
		switch(bytes){
			case -1:
				perror("read");
				offset = flen;
				break;
			case 0:
				offset = flen;
				break;
			default:
				write(fd, buf, bytes);
				offset += bytes;
				break;
		}
	}
	close(fd);
	printf("Received file %s\n", filename);

	return 0;
}

//called by the output thread
static void * chat_output(void * param){
	char line[LINE_LEN+1];
	char *msg;

	//get a line from server
	while(skt_readline(skt, line) > 0){	//get a line from server
		int code = line[0] - '0';	//find line code
		msg = &line[1];

		//check type of message
		switch(code){
			case MSG_FILE:
				receive_file(msg, skt);
				break;
			case MSG_SRV:
			case MSG_STRING:
				printf("%s\n", msg);			//show what server sent
				if(strncmp(msg, "Bye", 3) == 0)	//if bye, stop waiting for messages
					pthread_exit(NULL);
				break;
			default:
				fprintf(stderr, "Error: Unknown message code %i\n", code);
				break;
		}
	}
	pthread_exit(NULL);
}

int main(const int argc, char * const argv[]){
	// Check Validity
	if(argc <4)
	{
		printf("$ Usage: ./client -a <ip-address> -p <valid-port#> \n");
		exit(-1);
	}
	char * server = NULL;
	unsigned short port = DEFAULT_PORT;
	
	//get input arguments
	int opt;
	while((opt = getopt(argc, argv, "a:p:h")) > 0){
		switch(opt){
			case 'a':
				server = strdup(optarg);
				break;
			case 'p':
				port = atoi(optarg);
				break;
			case 'h':
				printf("-s \t Server ( default: localhost)\n");
				printf("-p \t Port ( default: 8888)\n");
				printf("-m \t Message ( default: Hello World!)\n");
				printf("-h \t Show this help message\n");
			default :
				printf("\n Invalid prompter!\n");
		}
	}

	if(server == NULL)
		server = strdup("localhost");


	skt = connect_to_server(server, port);
	if(skt == -1){
		return 1;
	}

	pthread_t thr_input, thr_output;	//thread ids
	if( (pthread_create(&thr_input, NULL, char_input,  NULL) != 0) ||
		(pthread_create(&thr_output,NULL, chat_output, NULL) != 0)		){
		perror("pthread_create");
		return 1;
	}

	pthread_join(thr_output, NULL);
	pthread_cancel(thr_input);			//kill the input
	pthread_join(thr_input,  NULL);

	close(skt);

	return 0;
}

