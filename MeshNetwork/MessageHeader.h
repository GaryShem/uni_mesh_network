#pragma once
#define WIN32_LEAN_AND_MEAN
#pragma comment(lib,"Ws2_32.lib")
#include <WinSock2.h>
#include <Windows.h>
#include <inaddr.h>

struct MessageHeader
{
	time_t timestamp;
	in_addr sender_ip;
	in_addr receiver_ip;
	int rnd;
	bool is_ack;
	bool is_delivered;
	//unsigned char current;
	//unsigned char total;
};