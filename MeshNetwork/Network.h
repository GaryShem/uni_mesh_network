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
	std::vector<in_addr> neighbor_ip_list;
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

	bool attach_header(char* buf, std::string message, MessageHeader sig); // пакуем наш пакет (хэдер + сообщение)
	bool generate_header(std::string ip, bool is_ack, MessageHeader* sig); // генерация хэдера с сигнатурой

	void bind_socket(SOCKET& s, int port); //ставим на прослушку порты
	void send_hello(); // рассылаем соседям приветы

	bool send_message(MessageHeader header, std::string message, int port); // сборка и передача пакета между соседями
	bool generate_message(std::string ip, std::string message, bool wait_for_ack); // обработка команды отправки сообщения
	void receive_message(); // получение сообщения
	void receive_hello(); // получение привета

	bool add_ip_to_neighbors(in_addr ip); // добавление айпи к списку соседей
	int find_neighbor_ip(in_addr ip); // поиск айпи потенциального соседа в списке соседей
	int find_header_signature(MessageHeader header); // поиск сигнатуры сообщения в игнор-листе
	int find_reverse_header_signature(MessageHeader header); // поиск по списку игнора пакета (только ищем со свапнутыми полями айпи получателя и отправителя)
	MessageHeader reverse_header(MessageHeader header); // обмен полей с айпи получателя и отправителя внутри пакета
	void add_signature_to_ignore(MessageHeader header); // добавление сигнатуры пакета в игнор-лист
};


#endif MESH_NETWORK_H