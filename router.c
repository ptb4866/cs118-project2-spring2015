
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


bool data; 
char sendTo; 
std::string msgLine; 
char thisNode; 
void printUpdatedDistance(char nodeName,bool print);

struct sockaddr_in remaddr[50]; 

void updateDistantVector(char nodeName);

// a structure to represent a weighted edge in graph
struct Edge
{
    int src, dest, weight;
};
 
// a structure to represent a connected, directed and weighted graph
struct Graph
{
    // V-> Number of vertices, E-> Number of edges
    int V, E;
 
    // graph is represented as an array of edges.
    struct Edge* edge;
};





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

typedef struct {
	int fd; 
	int port; 	
	int nodeName;
}sendArg; 


struct node { 
	char src; 
	int srcCost; 	
	char dest; 
	int  srcPort; 
	int destPort; 	
	int destCost; 
}; 
vector<struct node> nodeList;  

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

void updateRoute(char *recvMessage, char nodeName, bool &print) {


	//Extract The message 
	std::string msg(recvMessage); 
	node nInfo; 	

	//cout <<msg <<endl; 
	if (msg.substr(6,5) == "CNTL!") {
		int numBytes =11; 
		//parse data	
		while (numBytes < msg.length() ) { 

			try { 
				nInfo.src = msg[numBytes]; 
				numBytes++;

				std::string str; 
				str = msg.substr(numBytes, 5);  
				stringstream ss;
				ss << str;  
				ss >> nInfo.srcPort;  
				numBytes += 5;  
				nInfo.dest = msg[numBytes]; 
				numBytes++; 	
				
				stringstream ss2; 
				str = msg.substr(numBytes, 5);
				ss2 << str; 
				ss2 >> nInfo.destPort; 
				numBytes +=5;  

				nInfo.destCost = msg[numBytes];   
				nInfo.destCost = abs(nInfo.destCost - '0');  
				numBytes++; 

			} catch (int e) {

				cout << "Message Corrupted " <<endl; 
			}
			
		
		//	pthread_mutex_lock(&mutexRecv);  
			for (int i = 0; i < nodeList.size(); i++ ) {

				if (nInfo.destPort == nodeList.at(i).destPort) {
					if (nInfo.destCost != nodeList.at(i).destCost) {
						print = true; 
						break; 

					}
				} 


			}

			//Check if Route is not in route list, if not add it 
			bool found = false; 
			int k = 0; 	
			for (int i = 0; i < nodeList.size(); i++ ) {

				if (nInfo.destPort == nodeList.at(i).destPort) {
					found = true; 
					k = i; 
					break; 
				} 


			}
			if (found == false) {
				//Not in routing table? Check for its source first
				if (nInfo.dest == nodeName) 
					nInfo.destCost = 0;
			
				nodeList.push_back(nInfo);  
			}  

		//	pthread_mutex_unlock(&mutexRecv);  

		}	
	}
	 else { 


		if (msg.substr(6,5) == "DATA!") {
			int numBytes =11;  
			bool sent = false; 
			for (int x = 0 ; x < nodeList.size(); x++ ) {
				
				if (nodeList.at(x).src == msg[11]) {

					char tmp[msg.length()]; 
					for (int j = 12; j < msg.length(); j++) {
						tmp[j - 12] = msg[j]; 
					} 
					cout << string(tmp) <<endl; 
					sent = true; 
					break; 
				}
				
			}  

			if (sent == false ) { 
				for (int t = 0; t < nodeList.size(); t++ ) {
					
					if (nodeList.at(t).dest == msg[11]) {

						char server[48]; 
						std::string ipstr; 
						ipstr = "127.0.0.1"; 
						strcpy(server,ipstr.c_str()); 
						sizeof(server); 
						server[sizeof(server) - 1] = 0; 

						struct sockaddr_in address; 
						memset((char *) &address, 0, sizeof(address));
						address.sin_family = AF_INET; 

						long port = nodeList.at(t).srcPort ; 
						address.sin_port = htons(port);
						if (inet_aton(server, &address.sin_addr)==0) {
							fprintf(stderr, "inet_aton() failed\n");
							exit(1);
						} 
						struct sockaddr_in dummy; 
						int slen=sizeof(dummy); 


						int tmpfd = socket(AF_INET, SOCK_DGRAM, 0);  
						
						if (sendto(tmpfd,recvMessage, msg.length(), 0, (struct sockaddr *)&address, slen)==-1) {
							perror("sendto error");
							//exit(1);
						}

						close(tmpfd);  

					} 

				}  
			}
		} 

	} 


	for (int i = 0; i < nodeList.size(); i++) {
		
	cout << nodeList.at(i).dest <<":"<<nodeList.at(i).destCost <<" " ; 	

	} 	
	cout <<endl; 

}
void *recvData(void *args) { 
	
	char recvMessage[BUFSIZE];  
	
	recvArg arg = *((recvArg *)args); 
	int recvlen;		/// # bytes in acknowledgement message 
	struct sockaddr_in remAddr;
	socklen_t addrlen = sizeof(remAddr);  
	while(1) {  


	//	recvlen = recvfrom(arg.sockfd, recvMessage, BUFSIZE, 0, (struct sockaddr *)&remAddr, &addrlen);
	
		recvlen = recv(arg.sockfd, recvMessage, BUFSIZE - 1, 0); 	
		if (recvlen > 0) {
			recvMessage[recvlen] = 0;
			if (recvMessage[0] == 'A' &&
			recvMessage[1] == 'L' &&
			recvMessage[2] == 'O' &&
			recvMessage[3] == 'H' && 
			recvMessage[4] == 'A') {
				bool print = false; 
				//pthread_mutex_lock(&mutexRecv);  
				updateRoute(recvMessage, arg.nodeName, print);
				memset(recvMessage,0,strlen(recvMessage));  
				updateDistantVector(arg.nodeName);
				//printUpdatedDistance(arg.nodeName,print); 
				print = false; 
			//	pthread_mutex_unlock(&mutexRecv);  

			} 


		}

		sleep(1); 
	
	}

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
		node n_nei; 
		n_nei.src = nodeName; 
		n_nei.srcPort = nodePort; 
		n_nei.dest = n.at(i).name;
		n_nei.destPort = n.at(i).port; 
		n_nei.destCost = n.at(i).cost;

		if(!isActiveNeighbors(nodeName,nodePort, n_nei.destPort)) {

			if (nodeList.empty()) { 
				nodeList.push_back(n_nei);  

			} else { 
				bool found = false; 
				for (int k = 0; k < nodeList.size(); k++) {
					if (n_nei.destPort == nodeList.at(k).destPort) {

						found = true; 
						break; 
					} 

				} 
				if (found == false) {
					nodeList.push_back(n_nei);  

				} 
			

			} 
		} else {

			for (int x = 0; x < nodeList.size(); x++ ) {
				
				if (n_nei.destPort == nodeList.at(x).destPort) {
					nodeList.erase(nodeList.begin(), nodeList.begin() + x); 
					
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
	for (int k = 0; k < nodeList.size(); k++) {

		ss << nodeList.at(k).src <<  nodeList.at(k).srcPort; 
		ss << nodeList.at(k).dest; 
		ss << nodeList.at(k).destPort << nodeList.at(k).destCost; 	
		

	} 
	ss >> str; 

	return str; 

} 
void *sendData(void *args) { 

	sendArg arg = *((sendArg *)args); 

	char dest;
	int destPort; 
	int destCost;

	char src = arg.nodeName; 
	int srcPort = arg.port;

	std::string controlData;
	char buf[BUFSIZE];	/* message buffer */
	struct sockaddr_in dummy; 
	int slen=sizeof(dummy); 


	while (1) { 
		pthread_mutex_lock(&mutexRecv);  

		//std::stringstream ss; 
		checkForActiveNodes(src ,srcPort);

		string aloha = "ALOHA#CNTL!"; 
		std::string controlData = formatBroadCast(); 
		aloha +=controlData;
		controlData = aloha;  
		for (int k = 0 ; k < controlData.length(); k++ ) {

				buf[k] = controlData[k];  
		}


		for (int i = 0; i < n.size(); i++ ) {
			if (sendto(arg.fd, buf, strlen(buf), 0, (struct sockaddr *)&remaddr[i], slen)==-1) {
				perror("sendto");
				exit(1);
			}

		} 


		//Code to Send Data 
		if (data == true ) { 

			char server[48]; 
			std::string ipstr; 
			ipstr = "127.0.0.1"; 
			strcpy(server,ipstr.c_str()); 
			sizeof(server); 
			server[sizeof(server) - 1] = 0; 

			struct sockaddr_in address; 
			memset((char *) &address, 0, sizeof(address));
			address.sin_family = AF_INET;

			
			int foundDest = -1; 
			for (int k  = 0; k  < nodeList.size(); k++ ) {

				if (nodeList.at(k).dest == sendTo) {
					
					foundDest = k; 
					break; 
				} 
			} 

			if (foundDest >= 0) {
				long port = nodeList.at(foundDest).srcPort ; 
				address.sin_port = htons(port);
				if (inet_aton(server, &address.sin_addr)==0) {
					fprintf(stderr, "inet_aton() failed\n");
					exit(1);
				} 
			
				//data to send 
				string dummyString =  "ALOHA#DATA!"; 
				string dummy2;  
				dummy2[0] = sendTo; 
				dummyString += dummy2; 
				msgLine= dummyString + msgLine; 
				char dataD[msgLine.length()]; 
				for (int v = 0; v < msgLine.size(); v++ ) {
					dataD[v] =  msgLine[v]; 

				} 

				if (!isActiveNeighbors(src, srcPort, port)) {
								
					if (sendto(arg.fd,dataD, strlen(dataD), 0, (struct sockaddr *)&address, slen)==-1) {
						perror("sendto");
						exit(1);
					}
				} 
			}
			data = false; 
		} 	


		pthread_mutex_unlock(&mutexRecv);  



		sleep(1); 

	} 

} 

 
 
// Creates a graph with V vertices and E edges
struct Graph* createGraph(int V, int E)
{
    struct Graph* graph = (struct Graph*) malloc( sizeof(struct Graph) );
    graph->V = V;
    graph->E = E;
 
    graph->edge = (struct Edge*) malloc( graph->E * sizeof( struct Edge ) );
 
    return graph;
}
 
// A utility function used to print the solution
void printUpdatedDistance(char nodeName,bool print)
{ 

	if (print) {
	   printf("Vertex   Distance from Source\n");

	   for (int p = 0; p < nodeList.size(); p++ ) {
			//printf("%d \t\t %d\n", nodeList.at(p).dest, nodeList.at(p).destCost);
			if(nodeList.at(p).dest != nodeName) {
				printf("%c\t", nodeList.at(p).dest);
			}
	  } 
	  cout <<endl; 
	   for (int p = 0; p < nodeList.size(); p++ ) {
			//printf("%d \t\t %d\n", nodeList.at(p).dest, nodeList.at(p).destCost);
			if(nodeList.at(p).dest != nodeName) {
				printf("%d\t", nodeList.at(p).destCost);
			}
	  } 
     } 
}
 
// The main function that finds shortest distances from src to all other
// vertices using Bellman-Ford algorithm.  The function also detects negative
// weight cycle
void BellmanFord(struct Graph* graph, int src)
{
    int V = graph->V;
    int E = graph->E;
    int dist[V];
 
    // Step 1: Initialize distances from src to all other vertices as INFINITE
    for (int i = 0; i < V; i++ ) 
    {
	nodeList.at(i).srcCost = INT_MAX;  
    }
 
    // Step 2: Relax all edges |V| - 1 times. A simple shortest path from src
    // to any other vertex can have at-most |V| - 1 edges
    for (int i = 1; i <= V-1; i++)
    {
        for (int j = 0; j < E; j++)
        {
            int u = graph->edge[j].src;
            int v = graph->edge[j].dest;
            int weight = graph->edge[j].weight;
            if (nodeList.at(j).src != INT_MAX && nodeList.at(j).src + weight < nodeList.at(j).destCost)  {
                nodeList.at(j).destCost = nodeList.at(j).src + weight;
	   }
        }
    }

 
    return;
}


void updateDistantVector(char nodeName) {

	int V = nodeList.size() ; 
	int E = nodeList.size();  

	struct Graph *graph = createGraph(V,E);  
	
	for (int i = 0; i < nodeList.size(); i++ )  {

		graph->edge[i].src = (int) nodeList.at(i).src ; 		
		//cout << "src: " <<graph->edge[i].src <<endl; 		
		graph->edge[i].dest = (int)nodeList.at(i).dest; 
		//cout <<"dest: " <<graph->edge[i].dest <<endl; 
		graph->edge[i].weight = nodeList.at(i).destCost; 
		//cout <<"weight: " <<graph->edge[i].weight <<endl<<endl; 
	}

	BellmanFord(graph,(int)nodeName);  

} 

int argument(int argc, char **argv, char nodeName) {
	stringstream arg,arg2, arg3; 
	data = false; 
	sendTo = 'A'; 
	//check for data 
	if (argc > 1 ) {
		arg << argv[1] ; 
		string str; 
		arg >> str;  
		if (str == "data") { 
			if (argc > 2) {
				
				arg2 << argv[2];  
				arg2 >> str; 
				data = true; 		
				std::fstream  file(str.c_str(),std::fstream::in | std::fstream::binary);   
				if(!file.is_open()) {
					 return DV_FILE_OPEN_FAILED; 
				} 
				arg3 << argv[3]; 
				char tmp ; 
				arg3 >> tmp; 
				if (tmp != ' ') {
					sendTo = tmp; 
				}
				std::string tmpLine, sendToLine;  
				sendToLine[0] = nodeName;  
				msgLine = ""; 
				while (getline(file,tmpLine)) {
					file << tmpLine; 
					msgLine += tmpLine; 	
				} 
				msgLine += " FROM ";  
				msgLine += sendToLine[0];  
				cout <<"line" << msgLine;  
			}
			
		}
		
	}


} 

//Used to detect Control C 
//When controlC is presssed, reinistial file by removing 
//the "." file 
//Doesn't work correctly yet 
void my_handler(int s){ 
	ifstream inFile; 
	std::string line; 
	vector <string> routes; 
	inFile.open(".routes.dat",  ios::in | ios::binary);  
		//Read line from routes.dat 
	while (getline(inFile,line)) { 
		routes.push_back(line); 	
	}  

	inFile.close(); 
	std::reverse(routes.begin(),routes.end());

	vector<string> routes2; 
	while (!routes.empty()) {

		std::string str = routes.back(); 
		routes.pop_back(); 
		if (str[2] == thisNode) {
		
			str = str.substr(0, str.length() - 2); 
			routes2.push_back(str); 

		} else {

			routes2.push_back(str);  
		}
	} 

	std::fstream file(".route.dat",std::fstream::in | std::fstream::out | std::fstream::binary);  
	
	while (!routes2.empty()) {
		std::string dataStr = routes2.back(); 
		routes2.pop_back(); 
		file << dataStr; 
		cout <<dataStr <<endl; 
	} 
	file.close(); 
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


int main (int argc, char **argv) {


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
	if(argument(argc,argv,nodeName) < 0) {
		perror ("failed to open file");  

	}

	thisNode = nodeName; 
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


	//send thread
	pthread_t sendThread ; 
	sendArg *sArgs = new sendArg; 
	sArgs->fd = fd; 
	sArgs->nodeName = nodeName; 
	sArgs->port = serverPortNumber;
	pthread_create(&sendThread, &attr, sendData,sArgs);  


	//Ctr-C Trhread
	pthread_t controlCThread ; 
	pthread_attr_t cntAttr; 
	pthread_attr_init(&cntAttr); 
	int z = (int)nodeName; 
	pthread_create(&controlCThread, &cntAttr, controlC,(void*)z);  

	struct sockaddr_in dummy;
	socklen_t addrlen = sizeof(dummy);  


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


	} 

	while(1){
		
	} 
	
	pthread_mutex_destroy(&mutexRecv); 
	pthread_cancel(recvThread); 
	pthread_join(recvThread,NULL); 
	pthread_cancel(sendThread); 
	pthread_join(sendThread,NULL); 
	pthread_cancel(controlCThread); 
	pthread_join(controlCThread,NULL); 

	return 0; 
} 






