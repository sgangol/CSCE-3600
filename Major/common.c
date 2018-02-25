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

#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include "common.h"

int skt_readline(int skt, char * buf){
	int len=0;
	while((recv(skt, &buf[len], 1, 0) == 1) && (len < LINE_LEN)){
		if(buf[len++] == '\n')			//if this is end of message
			break;
	}
	buf[--len] = '\0';
	return len;
}

int skt_writeline(int skt, char * buf, int len){
	buf[len++] = '\n';					//put a newline at end of message
	if(send(skt, buf, len, 0) != len){	//send to user
		perror("write");
		len = 0;
	}
	return len;
};
