#ifndef MESH_NETWORK_H
#define MESH_NETWORK_H

#pragma comment(lib,"Ws2_32.lib")
#define WIN32_LEAN_AND_MEAN
#include <string>
#include <Winsock2.h>
#include <Ws2tcpip.h>
#include <Windows.h>
#include <stdlib.h>
#include <iostream>
#include <thread>
#include <ctime>
#include <vector>
#include "MessageHeader.h"
#include <mutex>

#define BUFFER_SIZE 512
#define MESSAGE_LENGTH 400
#define MESSAGE_PORT 8888
#define HELLO_PORT MESSAGE_PORT+1
//#define HELLO_PORT ACK_PORT+1

class Network
{
public:
	Network();
	~Network();
	std::vector<in_addr> neighbor_ip;
	in_addr own_ip;
	SOCKET socket_listener;
	SOCKET socket_hello;
	SOCKET socket_sender;
	char buffer[BUFFER_SIZE];
	WSADATA wsaData;
//	std::mutex sendlist_mutex;
//	std::mutex signatures_to_ignore_mutex;
	bool network_ready = false;
	std::vector<MessageHeader> signatures_to_ignore;
	std::thread message_thread;
	std::thread hello_thread;

	bool attach_header(char* buf, std::string message, MessageHeader sig);
	bool generate_header(std::string ip, bool is_ack, MessageHeader* sig);

	void bind_socket(SOCKET& s, int port);
	void send_hello();

	bool send_message(MessageHeader header, std::string message, int port);
	bool generate_message(std::string ip, std::string message, bool wait_for_ack);
	void receive_message();
	void receive_hello();

	bool add_ip_to_neighbors(in_addr ip);
	int find_neighbor_ip(in_addr ip);
	int find_header_signature(MessageHeader header);
	int find_reverse_header_signature(MessageHeader header);
	MessageHeader reverse_header(MessageHeader header);
	void add_signature_to_ignore(MessageHeader header);
};


#endif MESH_NETWORK_H