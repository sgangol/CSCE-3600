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
     DEPENDENCIES:  NONE                                              **
     AUTHORS:       Rawdhah Alshaqaq | Saina Baidar | Srizan Gangol   **
                    Sundus Nasser Said Al Subhi                       **
     DATE:          05/06/2017                                        **					
------Read ReadMe.txt & Screenshot for instructions & examples--------**
 ***********************************************************************/
 
#ifndef COMMON_H
#define COMMON_H

#define LINE_LEN 1024
#define NAME_LEN 20
#define DEFAULT_PORT 10000

int skt_readline(int skt, char * buf);
int skt_writeline(int skt, char * buf, int len);

enum msg_type{
	MSG_SRV=0,		//server message (quit, join, etc)
	MSG_STRING,		//chat message
	MSG_FILE,		//file transfer
	MSG_TYPES
};

#endif
