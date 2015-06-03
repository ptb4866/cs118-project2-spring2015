
/*Project 2 


*/ 

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

#include <arpa/inet.h>

using namespace std; 

#define BUFSIZE 2048

#define DV_FILE_OPEN_FAILED -1000

pthread_mutex_t mutexRecv; 
struct neighbors { 

	char name;	
	int port; 
	int cost; 
	char ipAddress[48]; 

}; 
vector<struct neighbors> n; 

typedef struct {
	int sockfd; 
	char nodeName; 
	
}recvArg; 

struct routingTable { 
	char src; 
	char dest; 
	int  srcPort; 
	int destPort; 	
	int destCost; 
}; 
vector<struct routingTable> routeList;  


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


					//Generate an Ip address for neighbor a
					std::string ipstr; 
					ipstr = "127.0.0.1"; 
					strcpy(neighbor.ipAddress,ipstr.c_str()); 
					sizeof(neighbor.ipAddress); 
					neighbor.ipAddress[sizeof(neighbor.ipAddress) - 1] = 0; 

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

void updateRoute(char recvMessage[], char nodeName) {


	//Extract The message 
	std::string msg(recvMessage); 
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
	//	if (found == false) {
			//Not in routing table? Check for its source first
//			if (rt.dest != nodeName) {
				routeList.push_back(rt);  
//			}
	//	} 
	}


	for (int i = 0; i < routeList.size(); i++) {
		
		cout << routeList.at(i).dest <<":"<<routeList.at(i).destCost <<" " ; 	
	} 	
	cout <<endl; 

}
void *recvData(void *args) { 
	
	char recvMessage[BUFSIZE];  
	
	recvArg arg = *((recvArg *)args); 
	int recvlen;		/* # bytes in acknowledgement message */
	struct sockaddr_in remaddr;
	socklen_t addrlen = sizeof(remaddr);  

	//int sockfd = (int)socket; 
	while(1) {  


		recvlen = recvfrom(arg.sockfd, recvMessage, BUFSIZE, MSG_DONTWAIT, (struct sockaddr *)&remaddr, &addrlen);

		cout << "Received Result: " <<recvlen << " "<< string(recvMessage)<<endl; 

		if (recvlen > 0) {
			recvMessage[recvlen] = 0;
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


		}


		sleep(1); 
	
	}

} 



int main (int arc, char **argv) {

	struct sockaddr_in myaddr;
	int fd, i;
	char buf[BUFSIZE];	/* message buffer */
	char server[48] ;	/* change this to use a different server */

	/* create a socket */

	if ((fd=socket(AF_INET, SOCK_DGRAM, 0))==-1)
		printf("socket created\n");

	memset((char *)&myaddr, 0, sizeof(myaddr));
	myaddr.sin_family = AF_INET;

	char nodeName; 
	getRoutesFromFile(n,nodeName); 
	int serverPortNumber = getNodePortNumber(nodeName); 
	
	std::string ipstr; 
	ipstr = "127.0.0.1"; 
	strcpy(server,ipstr.c_str()); 
	sizeof(server); 
	server[sizeof(server) - 1] = 0; 

	/* bind it to all local addresses and pick any port number */
	myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	myaddr.sin_port = htons(serverPortNumber);

	if (bind(fd, (struct sockaddr *)&myaddr, sizeof(myaddr)) < 0) {
		perror("bind failed");
		return 0;
	}       

	struct sockaddr_in remaddr[n.size()]; 
	struct sockaddr_in dummy; 
	int slen=sizeof(dummy); 
	
	routingTable rt; 
	for (int i = 0; i < n.size(); i++ ) {
		/* now define remaddr, the address to whom we want to send messages */
		/* For convenience, the host address is expressed as a numeric IP address */
		/* that we will convert to a binary format via inet_aton */

		memset((char *) &remaddr[i], 0, sizeof(remaddr[i]));
		remaddr[i].sin_family = AF_INET;
		long port = n.at(i).port; 
		remaddr[i].sin_port = htons(port);
		if (inet_aton(server, &remaddr[i].sin_addr)==0) {
			fprintf(stderr, "inet_aton() failed\n");
			exit(1);
			}

		//Also initialize routing table with neighbors info
		rt.dest =  n.at(i).name; 
		rt.destPort = n.at(i).port; 
		rt.destCost = n.at(i).cost; 
		routeList.push_back(rt); 


	} 

	//mutex
	pthread_mutex_init(&mutexRecv, NULL);  

	//Thread attr
	pthread_attr_t attr; 
	pthread_attr_init(&attr);  

	//Receive Thread
	pthread_t recvThread ; 
	recvArg *rArgs = new recvArg; 
	rArgs->sockfd = fd; 
	rArgs->nodeName = nodeName; 
	pthread_create(&recvThread, &attr, recvData,rArgs);  


	char dest;
	int destPort; 
	int destCost;

	char src = nodeName; 
	int srcPort = serverPortNumber;
	
	std::string controlData;
	std::stringstream ss; 
	while (1) {
		
		for (int i = 0; i < n.size(); i++ ) {

			for (int j = 0; j < routeList.size(); j++) {

				dest = routeList.at(i).dest;
				destPort = routeList.at(i).destPort;
				destCost = routeList.at(i).destCost; 
				ss << "ALOHA#" << "CNTL#" << src << "#" << srcPort << "#"; 
				//copy data into buffer 
				ss << dest << "#" <<destPort <<"#" << destCost <<".";  
				ss >> controlData;
		
				for (int k = 0 ; k < controlData.length(); k++ ) {

						buf[k] = controlData[k];  
				} 

				if (sendto(fd, buf, strlen(buf), 0, (struct sockaddr *)&remaddr[i], slen)==-1) {
					perror("sendto");
					exit(1);
				}
				ss.str(""); 
				controlData = ""; 
			
			}
		}

		sleep(1); 

			
	} 
	
	pthread_mutex_destroy(&mutexRecv); 
	pthread_cancel(recvThread); 
	pthread_join(recvThread,NULL); 

	return 0; 
}
