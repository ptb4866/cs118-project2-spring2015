#include <sys/socket.h> 
#include <netinet/in.h> 
#include <stdio.h> 
#include <sys/types.h>  
#include <stdlib.h>
#include <strings.h>
#include <string.h> 
#include <sys/wait.h>   /* for the waitpid() system call */
#include <signal.h>     /* signal name macros, and the kill() prototype */
#include <string>
#include <unistd.h>
#include <iostream>
#include <limits.h>
#include <sstream>
#include <sys/time.h>  
#include <netdb.h> 
#include <stdlib.h>  

#include <pthread.h> 

#define NUM_THREADS 1 
#define BUFFER_SIZE 4096  

void _setupTimer() {
	sleep(1); 
}


void _udpOpenSocket(int &fd ) {

	//Create a Socket 
	//_param 
	// AF_INET: domain/IPv4
	//SOCK_DRAM: type of service/UDP
	//0  PROTOCOL 
	if ((fd = socket(AF_INET, SOCK_DGRAM,0)) < 0 ) {
		perror ("Error creating socket!!");  
		exit(1); 
	}
}

void _udpServerSetup(int &fd, int portno) {

	struct sockaddr_in serv_addr; 
	//bind to arbituary address
	bzero((char *) &serv_addr, sizeof(serv_addr));
     	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(portno);

	if (bind(fd, (struct sockaddr *) &serv_addr,
              sizeof(serv_addr)) < 0)
              perror("ERROR on binding");
}


int  _udpRecv(int &fd, char *msg) {

	int recvLen;  
	struct sockaddr_in remaddr;
	socklen_t len = sizeof(remaddr);  


	//int recvfrom(int socket, void *restrict buffer,
	// size_t length, int flags, struct sockaddr *restrict src_addr,
        // socklen_t *restrict *src_len)
	recvLen = recvfrom(fd,msg, BUFFER_SIZE,MSG_DONTWAIT,(struct sockaddr *)\
&remaddr,&len);   


	return recvLen;  
}

void _udpSend(int &fd, char *msg,std::string remoteServerHostName, int remoteServerPort) {
	struct hostent *hp;
        int i;
	struct sockaddr_in remaddr;  
	//Wait for 1000 milliseconds 
	//_setupTimer();   

	char *host;  
	host = new char[remoteServerHostName.length() + 1];  
	strcpy(host, remoteServerHostName.c_str()); 
	
	hp =gethostbyname(host);  	

	if (!hp) {
		perror("Could not obtain host  address"); 
		exit(1);  
        }

       
	memset((char*)&remaddr, 0, sizeof(remaddr));
	remaddr.sin_family = AF_INET;
	remaddr.sin_port = htons(remoteServerPort);

	/* put the host's address into the server address structure */
	memcpy((void *)&remaddr.sin_addr, hp->h_addr_list[0], hp->h_length);


	/* send a message to the server */
	if (sendto(fd, msg,strlen(msg) , 0, (struct sockaddr *)&remaddr, sizeof(remaddr)) < 0) {
	perror("sendto failed");
		exit(1);  
	} 




} 


int main(int argc, char *argv[]) {

	int sockfd = -1; 
	struct sockaddr_in remaddr; 
	char msg[] = {'h','e','l','l','o'}; 
	char rcvMsg[BUFFER_SIZE];  

	

	std::cout << "Example input: \n" << "./router <serverportnumber> <remoteportnumber>" << std::endl; 
	if (argc < 3 ) {
		fprintf (stderr, "ERROR, no port provided\n\r"); 
		exit(1); 
	} 

	 _udpOpenSocket(sockfd);  

	int serverPortNumber = atoi(argv[1]);  
	_udpServerSetup(sockfd,serverPortNumber);


	//remote server port number 
	int remPortNumber = atoi(argv[2]);  

/*	//Convert remote hostname to char pointer 
	char *remoteServer; 
	std::string hostName = "127.0.0.1"; 
	remoteServer = new char[hostName.length() + 1];   
	strcpy (remoteServer, hostName.c_str()); */  


	char recvMessage[5];  
	std::string remoteServer = "127.0.0.1";  

	for (;;) { 

		_udpSend(sockfd, msg,remoteServer,remPortNumber ) ;
		
		if (_udpRecv(sockfd, recvMessage) > 0) {


			std::cout << recvMessage[0] << recvMessage[1] <<recvMessage[2] << recvMessage[3] << recvMessage[4] << recvMessage[5] << std::endl; 
		}

	
	 	 


	}  



	return 0;  
}


