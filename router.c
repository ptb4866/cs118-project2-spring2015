
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

#define BUFSIZE 4096

#define DV_FILE_OPEN_FAILED -1000
#define UPDATE_INTERVAL 1 	

struct timeval timeout;

string formatBroadCast(); 
struct neighbors { 

	char name;	
	int port; 
	int cost; 
	char ipAddress[48]; 

}; 
vector<struct neighbors> n; 


struct Node { 
	char node; 
	int16_t nodePort;
	char nextHop; 
	int16_t nextHopPort; 
	int16_t cost; 
	bool active;  
}; 
vector<struct Node> dv;  

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
//This fucntion prints the node neighbors 
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

void updateRoute(char *recvMessage, char nodeName) {


}


bool isActiveNeighbors(char nodeName, int nodePort, int neighborPort) {

	int tmpfd = socket(AF_INET, SOCK_DGRAM, 0); 
	struct sockaddr_in myaddr;
	memset((char *)&myaddr, 0, sizeof(myaddr));
	myaddr.sin_family = AF_INET;
	myaddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	myaddr.sin_port = htons(neighborPort);
	int status;
	if (status = bind(tmpfd, (struct sockaddr *)&myaddr, sizeof(myaddr)) >= 0) { 
		close(tmpfd); 
		int option = -1; 
		setsockopt(tmpfd,SOL_SOCKET,(SO_REUSEPORT | SO_REUSEADDR),(char*)&option,sizeof(option)); 
		return true; 
	}

	return false; 
} 

void checkForActiveNodes(char nodeName, int nodePort) {

	//Check for active neighbors 
	//if neighbor is active add to nodeList
	//Todo!! Remove from list if not active 
	for (int i = 0; i <  n.size(); i++) {
		Node n_nei; 
		n_nei.node = nodeName; 
		n_nei.nodePort = nodePort; 
		n_nei.nextHop = n.at(i).name;
		n_nei.nextHopPort = n.at(i).port; 
		n_nei.cost = n.at(i).cost;

		if(!isActiveNeighbors(nodeName,nodePort, n_nei.nextHopPort)) {

			if (dv.empty()) { 
				dv.push_back(n_nei);  

			} else { 
				bool found = false; 
				for (int k = 0; k < dv.size(); k++) {
					if (n_nei.nextHopPort == dv.at(k).nextHopPort) {

						found = true; 
						break; 
					} 

				} 
				if (found == false) {
					dv.push_back(n_nei);  

				} 
			

			} 
		} 
		

	} 


} 

/*
	Create BroadCast Message
*/
string formatBroadCast() { 

	std::string str = ""; 
	int numBytes = 0; 
	stringstream ss; 
	//pass route to char array and append delimer at the end 
	for (int k = 0; k < dv.size(); k++) {

		ss << dv.at(k).node << dv.at(k).nodePort; 
		ss << dv.at(k).nextHop; 
		ss << dv.at(k).nextHopPort << dv.at(k).cost; 	
		

	} 
	ss >> str; 


}  

void broadCastDVToNeighbors(char nodeName, char serverPortNumber, char *server) {

	struct sockaddr_in remaddr;  
	char msg[BUFSIZE]; 
	
	for (int i = 0; i < n.size(); i++) {

		if(!isActiveNeighbors(nodeName,serverPortNumber, n.at(i).port)) { 

			/*string str = "ALOHA#CNTL#" + formatBroadCast(); 	
			
			for (int j = 0; j < str.length(); j++ ) {
				msg[j] = str[j];  
			} */

					
			memset((char *) &remaddr, 0, sizeof(remaddr));
			remaddr.sin_family = AF_INET;
			long port = n.at(i).port; 
			remaddr.sin_port = htons(port);
			if (inet_aton(server, &remaddr.sin_addr)==0) {
				fprintf(stderr, "inet_aton() failed\n");
				exit(1);
			}

			int sock = socket(AF_INET, SOCK_DGRAM, 0);  
			int slen = sizeof(remaddr);  
			int retV = sendto(sock,msg, strlen(msg), 0, (struct sockaddr *)&remaddr,slen);  
		}
	}
} 


int main (int arc, char **argv) {

	struct sockaddr_in myaddr;
	int fd, i;
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
	myaddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	myaddr.sin_port = htons(serverPortNumber);

	if (bind(fd, (struct sockaddr *)&myaddr, sizeof(myaddr)) < 0) {
		perror("bind failed");
		return 0;
	}       

	
	struct sockaddr_in dummy;
	socklen_t addrlen = sizeof(dummy);  



	fd_set observedSockets; 
	int fdmax; 
	FD_ZERO(&observedSockets); 

	timeout.tv_sec= UPDATE_INTERVAL; 
	timeout.tv_usec=0;

	char recvMessage[BUFSIZE];  

	while (1) { 

		FD_SET(fd, &observedSockets);
		FD_SET(0,&observedSockets);

		/* keep track of the biggest file descriptor */
		fdmax = fd ; /* so far, it's this one*/

		int activity=select(fdmax+1,&observedSockets,NULL,NULL,&timeout); // blocking all until there is some activity on any of the sockets

		if(activity<0)
		{
			printf("error in select");
			return 0;
		}
		else if(activity==0)
		{
			 broadCastDVToNeighbors(nodeName,serverPortNumber, server) ; 
			//broadcast neighbor 
			timeout.tv_sec= UPDATE_INTERVAL;
			timeout.tv_usec=0;
		
		}
		else  { 

		       if(FD_ISSET(fd,&observedSockets)) // .. there has been activity on the mainSocket. thus there s a new connection that needs to be added
			{
               	
				int byteReceived =  recv(fd, &recvMessage, BUFSIZE - 1, 0);  
				//updateRoute(recvMessage, nodeName);  
			 
			} else {

				cout << "Display " <<endl; 
			}

		} 

	} 
	


	
	return 0; 
}
