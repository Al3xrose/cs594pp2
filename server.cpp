/*
** 
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string>
#include <cstring>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "packet.h"
#include <iostream>
#include <fstream>
#include <ctime>
#include <signal.h>
#include <time.h>
using namespace std;

#define PAYLOAD_SIZE 256

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

void send_packet(rdt_packet pack, int sockfd, sockaddr_storage their_addr, socklen_t addr_len)
{
	if(int numbytes = sendto(sockfd, pack.to_string().c_str(), pack.to_string().length(), 0, ((struct sockaddr*)&their_addr), addr_len) == -1)
	{
		perror("listener: sendto");
		exit(1);
	}	
}

int main(int argc, char **argv)
{
	timer_t timerid;
	int sockfd;
	struct addrinfo hints, *servinfo, *p;
	int rv;
	int numbytes;
	struct sockaddr_storage their_addr;
	char *buf;
	socklen_t addr_len;
	char s[INET6_ADDRSTRLEN];
	string packetType = "";
	char *fileReadBuffer;
	int fileIndex = 0;
	ifstream f;
	int timeStamp = 0;


	if(argc != 2)
	{
	  printf("usage: listener port\n");
	  exit(1);

	}
	char* MYPORT = argv[1];

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags = AI_PASSIVE; // use my IP

	if ((rv = getaddrinfo(NULL, MYPORT, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	// loop through all the results and bind to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			perror("listener: socket");
			continue;
		}

		if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("listener: bind");
			continue;
		}

		break;
	}

	if (p == NULL) {
		fprintf(stderr, "listener: failed to bind socket\n");
		return 2;
	}

	freeaddrinfo(servinfo);

	cout << "listener: listening on port " <<  MYPORT << endl;

	while(packetType != "END_CONN")
	{
	buf = new char[300];
	memset(buf, '\0', 300);
	addr_len = sizeof their_addr;
	if ((numbytes = recvfrom(sockfd, buf, 300 , 0,
		(struct sockaddr *)&their_addr, &addr_len)) == -1) {
		perror("recvfrom");
		exit(1);
	}

	rdt_packet receivedPacket = rdt_packet(buf);
	cout << "Received " << receivedPacket.to_string() << endl;

	int sequence_number = stoi(receivedPacket.get_sequence_number()); 
	sequence_number++;
	rdt_packet responsePacket = rdt_packet(to_string(sequence_number), "", "");

	packetType = receivedPacket.get_packet_type();
	//If we receive a connection request, response with a connection accept
	if(packetType == "CONN_REQUEST")
	{
		responsePacket.set_packet_type("CONN_ACCEPT");
		send_packet(responsePacket, sockfd, their_addr, addr_len);
	}
	else if(packetType == "FILE_REQUEST")
	{
		string filePath = receivedPacket.get_payload();
		cout << "Opening " << filePath;
		f.open(filePath);

		if(f)
		{
			responsePacket.set_packet_type("FILE_OK");
		}
		else
		{
			responsePacket.set_packet_type("ERROR");
			responsePacket.set_payload("File not found");
			packetType = "END_CONN";	
		}
		send_packet(responsePacket, sockfd, their_addr, addr_len);
	}
	else if(packetType == "ACK")
	{
		//if we're done reading the file, send a packet
		//indicating that the file send is complete
		fileReadBuffer = new char[PAYLOAD_SIZE+1];
		memset(fileReadBuffer, '\0', PAYLOAD_SIZE+1);
		f.read(fileReadBuffer, PAYLOAD_SIZE);
		if(f)
		{
			responsePacket.set_packet_type("DATA");
		}
		else
		{
			responsePacket.set_packet_type("DATA_END");
		}
		responsePacket.set_payload(fileReadBuffer);

		fileIndex += PAYLOAD_SIZE;

		delete [] fileReadBuffer;
		send_packet(responsePacket, sockfd, their_addr, addr_len);
	}
	else if(packetType == "END_CONN")
	{
		close(sockfd);
		exit(0);
	}

	string responseString = responsePacket.to_string();
	cout << "Responding with" << responseString << endl;

	delete [] buf;
	}

	close(sockfd);

	return 0;
}
