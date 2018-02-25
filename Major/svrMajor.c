/***********************************************************************
     CSCE 3600:     System Programming                                **
     ASSIGNMENT:    Major Assignment 1                                **
     DESCRIPTION:   Major 1                                           **
                    This program implements client/server model using **
                    Linux sockets for a “messaging system”            **
                    The Progrram can:                                 **
                    - Connect up to 4 client/server                   **
                    - Acquire the client ID via a simple handshake    **
                        i.e> hello <username>                         **
                    - Broadcast each client message or file           **
     DEPENDENCIES:  common.h                                          **
     AUTHORS:       Rawdhah Alshaqaq | Saina Baidar | Srizan Gangol   **
                    Sundus Nasser Said Al Subhi                       **			
     DATE:          05/06/2017                                        **
------Read ReadMe.txt & Screenshot for instructions & examples--------**
 ***********************************************************************/
#include <arpa/inet.h>
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <strings.h>
#include <string.h>
#include <sys/types.h>          
#include <sys/socket.h>
#include <netdb.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "common.h"

#define MAX_CONNS 4
static pthread_t thread[MAX_CONNS];

static int shutting_down = 0;	//flag set to 1, if server is going down

//input arguments
static char *server_ip = NULL;
static unsigned short port = DEFAULT_PORT;


static int		conns[MAX_CONNS];	/* connection sockets */
static int 	conns_count = MAX_CONNS;	//count of active connections
static pthread_mutex_t lock;		//lock fo accessing the request pool
static pthread_cond_t  condAdded;	//signaled when we add a new request
static pthread_cond_t  condRemoved;	//signaled when we add a new request

//Called when server exits
static int die(const int rc){

	//wait for threads to finish
	int i;
	for(i=0; i < MAX_CONNS; i++){
		pthread_join(thread[i], NULL);
	}

	//close any open sockets
	for(i=0; i < MAX_CONNS; i++){
		if(conns[i] > 0){
			close(conns[i]);
		}
	}

	pthread_mutex_destroy(&lock);
	pthread_cond_destroy(&condAdded);
	pthread_cond_destroy(&condRemoved);

	free(server_ip);

	exit(rc);
}

//give new connection to a thrads
void server_handoff (int sockfd) {

	pthread_mutex_lock(&lock);
	while(conns_count == 0){	//if all threads are busy
		pthread_cond_wait(&condRemoved, &lock);	//wait
	}

	//find a free thread
	int i;
	for(i=0; i < MAX_CONNS; i++){
		if(conns[i] == -1)
			break;
	}

	if(i == MAX_CONNS){	//if we have no free threads
		fprintf(stderr, "Error: Dropping connection on socket fd %i\n", sockfd);
	}else{
		conns_count++;
		conns[i] = sockfd;	//save socket fd to thread
		pthread_cond_broadcast(&condAdded);	//signal thread we have a new connection
	}

	pthread_mutex_unlock(&lock);
}

//send a message to all threads
static int msg_broadcast(int from, char *msg, const int len){
	pthread_mutex_lock(&lock);
		int i;
		for(i=0; i < MAX_CONNS; i++){
			if((conns[i] != -1) && (conns[i] != from)){
				send(conns[i], (void*)msg, len, 0);
			}
		}
	pthread_mutex_unlock(&lock);

	return 0;
}

//send a file to all clients
static int file_broadcast(int from){

	char header[LINE_LEN+1];
	bzero(header, sizeof(header));

	pthread_mutex_lock(&lock);

	//get the header line from client sending file
	int len = skt_readline(from, header);
	if (len <= 0){
		pthread_mutex_unlock(&lock);
		return 1;
	}
	header[len++] = '\n';

	//send header to all others
	int i;
	for(i=0; i < MAX_CONNS; i++){
		if((conns[i] != -1) && (conns[i] != from)){
			send(conns[i], (void*)header, len, 0);
		}
	}

	//get header variables
	char * code = strtok(header, ":");
	if(code == NULL){
		pthread_mutex_unlock(&lock);
		return 1;
	}
	char * filename = strtok(NULL, ":");
	if(filename == NULL){
		pthread_mutex_unlock(&lock);
		return 1;
	}
	char * sflen = strtok(NULL, ":");
	int flen = atoi(sflen);

	//send file data to others
	char buf[100];
	int offset=0;
	while(offset < flen){
		//read data from sender
		int bytes = recv(from, buf, sizeof(buf), 0);
		switch(bytes){
			case -1:
				perror("read");
				offset = flen;
				break;
			case 0:
				offset = flen;
				break;
			default:
				//send to receivers
				for(i=0; i < MAX_CONNS; i++){
					if((conns[i] != -1) && (conns[i] != from)){
						send(conns[i], (void*)buf, bytes, 0);
					}
				}
				offset += bytes;
				break;
		}
	}

	pthread_mutex_unlock(&lock);

	return 0;
}

//Called by each thread on a new connection
void serve_connection (int sockfd) {

	char  line[LINE_LEN+1];
	char reply[LINE_LEN+1];
	char name[NAME_LEN+1];

	bzero(name, NAME_LEN+1);
	bzero(line,  LINE_LEN+1);
	bzero(reply, LINE_LEN+1);

	int quit = 0;
	while (!quit) {

		//get a line from socket
		int len = skt_readline(sockfd, line);
		if (len <= 0)
			break;

		printf("%s: %s\n", name, line);

		if(strncmp("message", line, 7) == 0){
			len = snprintf(reply, LINE_LEN, "%i%s: %s", MSG_STRING, name, &line[7]);

		}else if(strncmp("put", line, 3) == 0){
			file_broadcast(sockfd);
			continue;
		}else if(strncmp("quit", line, 4) == 0){
			len = snprintf(reply, LINE_LEN, "%i%s has logged off", MSG_SRV, name);
			quit = 1;

		}else if(strncmp("hello", line, 5) == 0){
			strncpy(name, &line[6], len - 6);
			len = snprintf(reply, LINE_LEN, "%i%s has logged in", MSG_SRV, name);

		}else{
			fprintf(stderr, "Error: Line is '%s'\n", line);
			quit = 1;
			len = snprintf(reply, LINE_LEN, "%i%s was kicked out", MSG_SRV, name);
		}

		reply[len++] = '\n';
		msg_broadcast(sockfd, reply, len);
	}
}

//main thread function
void * thr_func(void * arg){
	int idx = *(int*) arg;

	while(1){
		pthread_mutex_lock(&lock);
			if(shutting_down){	//if server is stopping
				pthread_mutex_unlock(&lock);
				break;
			}

			while(conns[idx] == -1){	//if we don't have a connection
				pthread_cond_wait(&condAdded, &lock);
			}
		pthread_mutex_unlock(&lock);

		//process the connection
		printf("Thread %i serving fd %i\n", idx, conns[idx]);
		serve_connection(conns[idx]);

		//release the connection
		pthread_mutex_lock(&lock);
			close(conns[idx]);
			conns[idx] = -1;
			conns_count--;
		pthread_mutex_unlock(&lock);
	}
	printf("Thread %i done\n", idx);
	pthread_exit(NULL);
}

/* set up socket to use in listening for connections */
static int setup_listen_socket (const char * server_ip, const int port) {

	struct sockaddr_in servaddr;

	bzero(&servaddr, sizeof(struct sockaddr_in));

	servaddr.sin_family = AF_INET;
	//servaddr.sin_addr.s_addr = htonl (INADDR_ANY);
	servaddr.sin_addr.s_addr = inet_addr(server_ip);
	servaddr.sin_port = htons (port);

	/* create the socket */
	int listenfd = socket(AF_INET, SOCK_STREAM, 0);
	if(listenfd == -1){
		perror("socket");
		return -1;
	}

	int on = 1;
	if(setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(int)) == -1){
		perror("setsockopt");
		close(listenfd);
		return -1;
	}

	if(bind(listenfd, (struct sockaddr *) &servaddr, sizeof (servaddr))){
		perror("bind");
		close(listenfd);
		return -1;
	}

	if(listen(listenfd, MAX_CONNS) == -1){
		perror("listen");
		return -1;
	}

	socklen_t namelen = sizeof (servaddr);;
	getsockname (listenfd, (struct sockaddr *) &servaddr, &namelen);

	char ip[INET_ADDRSTRLEN];
	inet_ntop(AF_INET, &servaddr.sin_addr, ip, sizeof(ip));
	printf("Server started on %s:%hu\n", ip, port);

	return listenfd;
}

//called when server receives SIGINT
static void sig_handler (int sig) {

	pthread_mutex_lock(&lock);
	shutting_down = 1;	//raise shutdown flag
	int i;
	for(i=0; i < MAX_CONNS; i++){
		if(conns[i] == -1)
			conns[i] = -2;	//if conns[] is -1, thread will block again, make it -2
	}
	pthread_cond_broadcast(&condAdded);
	pthread_mutex_unlock(&lock);

	die(0);
}

int main (int argc, char **argv){
	
	// Check Validity
	if(argc <4)
	{
		printf("$ Usage: ./server -a <ip-address> -p <valid-port#> \n");
		exit(-1);
	}
	
	//get the input arguments
	int opt;
	while((opt = getopt(argc, argv, "a:p:h")) > 0){
		switch(opt){
			case 'a':
				server_ip = strdup(optarg);
				break;
			case 'p':
				port = atoi(optarg);
				break;
			case 'h':
				printf("%s [-a 1.2.3.4] [-p 10000]\n\n", argv[0]);
				printf("-a \t IP ( default: localhost)\n");
				printf("-p \t Port ( default: %i)\n", port);
				printf("-h \t Show this help message\n");
		}
	}

	if(server_ip == NULL)
		server_ip = strdup("localhost");

	signal(SIGINT, sig_handler);	//make server respond to SIGINT

	/*initialize synchronization of threads*/
	if(	(pthread_mutex_init(&lock,		 NULL) != 0) ||
		(pthread_cond_init(&condAdded, 	 NULL) != 0) ||
		(pthread_cond_init(&condRemoved, NULL) != 0)	){
		die(1);
	}

	/*start the threads*/
	int i;
	int idx[MAX_CONNS];
	for(i=0; i < MAX_CONNS; i++){
		idx[i] = i;
		conns[i] = -1;
		if(pthread_create(&thread[i], NULL, thr_func, &idx[i]) != 0){
			perror("pthread_create");
			die(1);
		}
	}

	int listenfd = setup_listen_socket (server_ip, port);
	if(listenfd == -1){
		die(1);
	}

	/* allow up to 4 queued connection requests before refusing */
	while (!shutting_down) {
		errno = 0;

		struct sockaddr_in cliaddr;
		socklen_t len = sizeof (cliaddr); /* length of address can vary, by protocol */

		int connfd;
		if ((connfd = accept (listenfd, (struct sockaddr *) &cliaddr, &len)) < 0) {
			if (errno != EINTR){
				perror("accept");
				die(1);
			}
		} else {
			printf("New connection from %s, fd %d\n", inet_ntoa(cliaddr.sin_addr), connfd);
			server_handoff (connfd); /* process the connection */
		}
	}
	close (listenfd);

	die(0);
	return 0;
}
