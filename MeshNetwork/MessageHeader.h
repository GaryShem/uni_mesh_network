#pragma once
#define WIN32_LEAN_AND_MEAN
#pragma comment(lib,"Ws2_32.lib")
#include <WinSock2.h>
#include <Windows.h>
#include <inaddr.h>

struct MessageHeader
{
	time_t timestamp; // время
	in_addr sender_ip; // айпи отправителя
	in_addr receiver_ip; // айпи получателя
	int rnd; // рандомное число
	bool is_ack; //пакет-подтверждение ли это
	bool is_delivered; // доставлено ли сообщение уже (и подтверждена доставка)
	//unsigned char current;
	//unsigned char total;
};