/*
** talker.c -- a datagram "client" demo
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "packet.h"
#include <string.h>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <math.h>
#include <pthread.h>
using namespace std;

#define MAX_PACKET_SIZE 2048

void send_packet(rdt_packet pack, int sockfd, struct sockaddr *their_addr, socklen_t addr_len)
{
	if(int numbytes = sendto(sockfd, pack.to_string().c_str(), pack.to_string().length(), 0, their_addr, addr_len) == -1)
	{
		perror("listener: sendto");
		exit(1);
	}	
}

int main(int argc, char *argv[])
{ 
	pthread_t senderthread;
	int sockfd;
	struct addrinfo hints, *servinfo, *p;
	int rv;
	int numbytes;
	struct sockaddr_storage their_addr;
	int sequence_number = 0;
	char *buf;
	string packetType = "";
	string fileBuffer = "";
	ofstream f;

	if (argc != 6) {
		cout << "usage: talker hostname port filename timeout(ms) window_size" << endl;
		exit(1);
	}
	
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;
	char* SERVERPORT = argv[2];
	string filePath = argv[3];
	int timeout = stoi(argv[4]);
	int windowSize = stoi(argv[5]);

	//create filename from path
	int i = 0;
	int j = filePath.length() -1;
	string filename = "";

	while(filePath[j] != '/')
	{
		filename.insert(0, 1, filePath[j]);
		j--;
	}
	
	cout << filename << endl;

	if ((rv = getaddrinfo(argv[1], SERVERPORT, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	// loop through all the results and make a socket
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			perror("talker: socket");
			continue;
		}
		break;
	}

	if (p == NULL) {
		fprintf(stderr, "talker: failed to create socket\n");
		return 2;
	}

	if ((rv = getaddrinfo(NULL, SERVERPORT, &hints, &servinfo)) != 0)
	{
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}
	bind(sockfd, servinfo->ai_addr, servinfo->ai_addrlen);

	string lastPacketType = "";

	//Always send connection request packet first
	rdt_packet pack = rdt_packet(to_string(0), "CONN_REQUEST", "");

	cout << "Sending packet: " << pack.to_string() << endl;

	if ((numbytes = sendto(sockfd, pack.to_string().c_str() , pack.to_string().length(), 0,
		 p->ai_addr, p->ai_addrlen)) == -1) 
	{
		perror("talker: sendto");
		exit(1);
	}
	
	while(packetType != "DATA_END")
	{
		int addr_len = sizeof their_addr;
		buf = new char[300];
		memset(buf, '\0', 300);
		if((numbytes = recvfrom(sockfd, buf, 300, 0,
		(struct sockaddr *)&their_addr, (socklen_t*)&addr_len)) == -1)
		{
			perror("recvfrom");
			exit(1);
		}
		rdt_packet receivedPacket = rdt_packet(string(buf));
		cout << "Received " << receivedPacket.to_string() << endl;

		int sequence_number = stoi(receivedPacket.get_sequence_number()); 
		sequence_number++;
		rdt_packet responsePacket = rdt_packet(to_string(sequence_number), "", "");
		
		packetType = receivedPacket.get_packet_type();
		//If we receive a connection accepted packet, send a file request packet with the path to the file as the payload
		if(packetType == "CONN_ACCEPT")
		{
			responsePacket.set_packet_type("FILE_REQUEST");
			responsePacket.set_payload(filePath);
			send_packet(responsePacket, sockfd, (p->ai_addr), p->ai_addrlen);
		}

		else if(packetType == "FILE_OK")
		{
			responsePacket.set_packet_type("ACK");
			send_packet(responsePacket, sockfd, p->ai_addr, p->ai_addrlen);
		}

		//If we receive a data packet, add the data to our buffer then send an ACK
		else if(packetType == "DATA")
		{
			fileBuffer += receivedPacket.get_payload();
			responsePacket.set_packet_type("ACK");
			responsePacket.set_payload("");
			send_packet(responsePacket, sockfd, p->ai_addr, p->ai_addrlen);
		}
		else if(packetType == "DATA_END")
		{

			fileBuffer += receivedPacket.get_payload();
			responsePacket.set_packet_type("END_CONN");
			responsePacket.set_payload("");
			f.open(filename);
			
			f << fileBuffer.c_str();
			f.close();
			send_packet(responsePacket, sockfd, p->ai_addr, p->ai_addrlen);
		}
		else if(packetType == "ERROR")
		{
			cout << receivedPacket.get_payload() << endl;
			close(sockfd);
			exit(1);
		}

		cout << "Responding with " << responsePacket.to_string() << endl;	
		/*if ((numbytes = sendto(sockfd, responsePacket.to_string().c_str() , responsePacket.to_string().length(), 0, p->ai_addr, p->ai_addrlen)) == -1) 
		{
			perror("talker: sendto");
			exit(1);
		}*/
		delete []buf;
	}	
	close(sockfd);
	freeaddrinfo(servinfo);
	return 0;
}
