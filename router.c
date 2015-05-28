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

#include <fstream> 
#include <algorithm> 
#include <vector> 
#include <cstring> 
#include <sstream> 


#include <signal.h> 
#include <stdlib.h> 

#include <pthread.h> 

#define NUM_THREADS 1 
#define BUFFER_SIZE 1024 
#define MAX_NODE 1024 


#define DV_FILE_OPEN_FAILED -1000 


using namespace std; 

pthread_mutex_t mutexRecv;
//Get Neighbors from file and NodeName 

struct neighbors { 

	char name;	
	int port; 
	int cost; 

}; 
vector<struct neighbors> n; 

typedef struct {
	int sockfd; 
	std::string remoteServer;  
	char nodeName; 
	
}sendArg; 

typedef struct {
	int sockfd; 
	char nodeName; 
	
}recvArg; 


struct forwardingTable { 
	char dest; 
	int cost; 
	char nextHop; 
}; 
forwardingTable ft; 

vector<struct forwardingTable> table; 

struct routingTable { 
	char src; 
	char dest; 
	int  srcPort; 
	int destPort; 	
	int destCost; 
}; 
vector<struct routingTable> routeList;  

void _setupTimer(int seconds) {
	sleep(seconds); 
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

int  getRoutesFromFile(vector<struct neighbors> &n, char &nodeName) {

	//first initialize node name and port 

	nodeName = ' '; 	
	vector<string> routes; 	
	char ifopened; 
	std::string line;  

	std::fstream  file(".route.dat",std::fstream::in | std::fstream::out | std::fstream::binary | std::fstream::app);  
	if(!file.is_open()) return DV_FILE_OPEN_FAILED; 

	file.seekp(0, ios::end);  
	int fileLength = file.tellp(); 

	if (fileLength == 0) 
	{
		ifstream inFile; 
		inFile.open("routes.dat",  ios::in | ios::binary);  
		//Read line from routes.dat 
		while (getline(inFile,line)) { 
			file << line << endl; 	
		//			routes.push_back(line);  	
		} 
		inFile.close(); 
	} 

	file.seekp(0,ios::beg);  
	while (getline(file,line)) { 
		routes.push_back(line);  	
	} 
	std::reverse(routes.begin(),routes.end());
	file.close(); 
	file.open(".route.dat",std::fstream::in | std::fstream::out | std::fstream::binary);  

	bool nodeNotRetrieved = true; 
	while (!routes.empty()) {
		std::string str = routes.back(); 
		routes.pop_back(); 
		if (str[str.length() - 1] == '.') {
			//Do nothing! file has been read
			//Just output the string back to file 
			file  << str <<endl; 
		} else { 

			//If node has not been selected
			//do the following otherwise get neighbor information 
			if (nodeNotRetrieved) {
				str.append(".");  
				//get node name 
				nodeName = str[0];  	
				//get first neighbor 
				std::string tmp; 
				struct neighbors neighbor;  
				//Neighbors Name
				tmp = str.substr(2,1);  
				neighbor.name = tmp[0]; 
				
				//Neighbors Port Number
				tmp = str.substr(4,5); 
				stringstream ss(tmp); 				
				ss >> neighbor.port ;  
				ss.str("");  

				//Cost to Neighbor 
				char tmpChar = str[str.length() - 2];  
				neighbor.cost = abs(tmpChar - '0'); 
				
				//save neighbor 
				n.push_back(neighbor); 

				nodeNotRetrieved = false;  
			} else {

				if (str[0] == nodeName) {
					//Get Neighbor's info	
					std::string tmp; 
					struct neighbors neighbor;  
					//Neighbors Name
					tmp = str.substr(2,1);  
					neighbor.name = tmp[0]; 
					
					//Neighbors Port Number
					tmp = str.substr(4,5); 
					stringstream ss(tmp); 				
					ss >> neighbor.port ;  
					ss.str("");  

					//Cost to Neighbor 
					char tmpChar = str[str.length() - 2];  
					neighbor.cost = abs(tmpChar - '0'); 
	
					//save neighbor 
					n.push_back(neighbor); 
			
					
					str.append(".");  

				}

			} 
			//Write to file 
			file << str <<endl; 

		}
	}

	file.close(); 
} 

//Print Neighbors should be called after 
//  getRoutesFromFile(vector<struct neighbors> &n, char &nodeName) 
//This fucntion prints the nodes neighbors 
void printNeighbors(vector<struct neighbors> &n ) {

	struct neighbors temp;  
	cout <<"size of neighbors" << n.size() <<endl; 
	for (int i = 0; i < n.size(); i++) {		
		temp = n.at(i);  
		cout  << "Neighbor : " <<endl; 
		cout << "Name: " << temp.name <<" "; 
		cout << "Port: " << temp.port <<" " ; 
		cout << "Cost: " << temp.cost <<endl; 

	}
} 



int getNodePortNumber(char nodeName) {

	std::string line,tmp; 
	std::fstream  file(".route.dat",std::fstream::in | std::fstream::binary);  
	if(!file.is_open()) return DV_FILE_OPEN_FAILED;  
	
	while (getline(file,line)) { 
		
		tmp = line.substr(2,1);  
		char node = tmp[0]; 
		if (node == nodeName) 
			break; 

	} 

	file.close(); 

	int port; 	
	tmp = line.substr(4,5); 
	
	stringstream ss(tmp); 				
	ss >> port ;  
	ss.str(""); 

	return port; 

}


static void *TimerRoutine(void *args) {

	//send ALOHA to neighbors every 5 seconds 
	//For simplicity the payload/header is also included 
	//in the packet. The packet is the routing table 

	//PACKET STRUCTURE : 
	/*
	ALOHA#CNTL#NODESID#NODESPORT#NEIGHBORID#NEIGHBORPORT#COSTTNEIGHBOR. 
			OR 
	ALOHA#DATA#NODESID#NODESPORT#NEIGHBORID#NEIGHBORPORT#COSTTNEIGHBOR. 
	*/

	sendArg arg = *((sendArg *)args); 

	char msg[BUFFER_SIZE];  

	std::string controlData; 
	//Route List 
	routingTable rt; 
	for (int i = 0; i < n.size(); i++) {
		rt.dest =  n.at(i).name; 
		rt.destPort = n.at(i).port; 
		rt.destCost = n.at(i).cost; 
		routeList.push_back(rt); 

	}

	while(1) {
		std::stringstream ss; 
	 	char src = arg.nodeName;
		int srcPort =getNodePortNumber(src);
		
			char dest;
		int destPort; 
		int destCost;

		for (int k = 0; k < n.size(); k++) { 

			for (int i = 0; i < routeList.size(); i++) {
				
					
				dest = routeList.at(i).dest;
				destPort = routeList.at(i).destPort;
				destCost = routeList.at(i).destCost; 
				cout <<dest <<" "; 	
				ss << "ALOHA#" << "CNTL#" << src << "#" << srcPort << "#"; 
				//copy data into buffer 
				ss << dest << "#" <<destPort <<"#" << destCost <<".";  

				ss >> controlData;  
	
				for (int j = 0 ; j < controlData.length(); j++ ) {

					msg[j] = controlData[j];  
				} 
			

				_udpSend(arg.sockfd,msg,arg.remoteServer,n.at(k).port) ; 

				ss.str("");  
				controlData = ""; 

			} 
		
		}
		_setupTimer(1); 
		cout <<endl; 
	} 


} 

//Used to detect Control C 
void my_handler(int s){
           printf("Caught signal %d\n",s);
           exit(1); 

}

void *controlC(void *threadId) { 
	//Used to detect if Ctrl-C is pressed 
	struct sigaction sigIntHandler;
	sigIntHandler.sa_handler = my_handler;
	sigemptyset(&sigIntHandler.sa_mask);
	sigIntHandler.sa_flags = 0;
	sigaction(SIGINT, &sigIntHandler, NULL); 


}
void updateRoute(char recvMessage[], char nodeName) {


	//Extract The message 
	std::string msg(recvMessage); 
//	cout <<msg <<endl; 
	routingTable rt; 

	std::string tmp; 
	if (msg.substr(6,4) == "CNTL") {
		//Extract Header 
		tmp = msg.substr(11, 1);  
		rt.src = tmp[0]; 
		tmp = msg.substr(13,5); 
		stringstream ss(tmp); 
		ss >> rt.srcPort; 
		ss.str(""); 
		tmp = msg.substr(19,1);
		rt.dest = tmp[0]; 
		tmp = msg.substr(21,5);  
		stringstream s(tmp);  
		s >> rt.destPort;
		s.str("");  
		char tmpChar = msg[27]; 
		rt.destCost = abs(tmpChar - '0');  

		//Check if Route is not in route list, if not add it 
		bool found = false; 
		int k = 0; 	
		for (int i = 0; i < routeList.size(); i++ ) {
			
			if (rt.destPort == routeList.at(i).destPort) {
				found = true; 
				k = i; 
				break; 
			}

		}
		if (found == false) {
			//Not in routing table? Check for its source first
			if (rt.dest != nodeName) {
				routeList.push_back(rt);  
			}
		} else { 

			bool check = false; 
			for (int i = 0; i < routeList.size(); i++ ) {
				
				if (routeList.at(i).destPort == rt.srcPort) {

					int getCost = routeList.at(i).destCost; 
					int sum ; 
					if (nodeName == routeList.at(i).dest) {
						sum = rt.destCost; 
					} else { 
					    sum = getCost + rt.destCost;  
					}

					if (sum < routeList.at(k).destCost) {
						rt.destCost = sum; 
						routeList.erase(routeList.begin() + k); 
						routeList.push_back(rt); 

					}
						
					break; 		
					check = true; 
				}

			} 
			if (check == false) {

				//Route most not exist, remove routes with rt.srcPort in routeList...
			}	
			

		} 

	}

	for (int i = 0; i < routeList.size(); i++) {
		
		cout << routeList.at(i).dest <<":"<<routeList.at(i).destCost <<" " ; 	
	} 	
	cout <<endl; 


	// check the forwarind table if it has the neighbors of this node
	// if there is neighbors --- good, if not , 
	// then remove dea.- check .route.data - remove the .


}


void *recvData(void *args) { 
	
	char recvMessage[BUFFER_SIZE];  
	
	recvArg arg = *((recvArg *)args); 

	//int sockfd = (int)socket; 
	while(1) { 
		_udpRecv(arg.sockfd, recvMessage); 

		if (recvMessage[0] == 'A' &&
			recvMessage[1] == 'L' &&
			recvMessage[2] == 'O' &&
			recvMessage[3] == 'H' && 
			recvMessage[4] == 'A') {

			pthread_mutex_lock(&mutexRecv);  
			updateRoute(recvMessage, arg.nodeName);
			pthread_mutex_unlock(&mutexRecv);  

			memset(recvMessage,0,strlen(recvMessage));  

		}  

		_setupTimer(1); 

	
	}

} 


void run(void) {

	int sockfd = -1; 
	struct sockaddr_in remaddr; 
	char rcvMsg[BUFFER_SIZE];  

	char nodeName; 
	 getRoutesFromFile(n,nodeName); 


	 _udpOpenSocket(sockfd);  

	int serverPortNumber = getNodePortNumber(nodeName); 
	_udpServerSetup(sockfd,serverPortNumber);



	char recvMessage[5];  
	std::string remoteServer = "127.0.0.1";  

	pthread_mutex_init(&mutexRecv, NULL);  

	//Send Thread
	pthread_t sendThread ; 
	pthread_attr_t attr; 
	pthread_attr_init(&attr);  
	//Argument 
	sendArg *args = new sendArg;  
	args->sockfd = sockfd; 
	args->remoteServer = remoteServer;  
	args->nodeName = nodeName; 
	//Periodically send ALOHA and Routing Updates to 
	//neighbors
	pthread_create(&sendThread, &attr, TimerRoutine,args);  

	//Receive Thread 	
	pthread_t recvThread ; 
	recvArg *rArgs = new recvArg; 
	rArgs->sockfd = sockfd; 
	rArgs->nodeName = nodeName; 
	pthread_create(&recvThread, &attr, recvData,rArgs);  


	//Ctr-C Trhread
	pthread_t controlCThread ; 
	pthread_attr_t cntAttr; 
	pthread_attr_init(&cntAttr); 
	int i = 0; 
	pthread_create(&controlCThread, &cntAttr, controlC,(void*)i);  


	while(1) {;} 

	pthread_mutex_destroy(&mutexRecv); 
	pthread_cancel(sendThread); 
	pthread_join(sendThread,NULL); 
	pthread_cancel(recvThread); 
	pthread_join(recvThread,NULL); 
	pthread_cancel(controlCThread); 
	pthread_join(controlCThread,NULL); 


}




int main(int argc, char *argv[]) {
	

	run();  

	return 0;  
}


