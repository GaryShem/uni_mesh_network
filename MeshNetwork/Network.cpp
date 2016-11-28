#include "Network.h"
#include <strstream>


Network::Network()
{
	network_ready = false;
	if (WSAStartup(MAKEWORD(1, 1), &wsaData) != 0) {
		exit(255);
	}
	srand(time(NULL));

	socket_sender = socket(AF_INET, SOCK_DGRAM, 0);
	if (socket_sender == INVALID_SOCKET)
	{
		std::cout << "Create sender socket error: " << WSAGetLastError << std::endl;
		exit(254);
	}
	std::cout << "sender socket created" << std::endl;

	bind_socket(socket_listener, MESSAGE_PORT);
	bind_socket(socket_hello, HELLO_PORT);

	message_thread = std::thread(&Network::receive_message, this);
	message_thread.detach();
	hello_thread = std::thread(&Network::receive_hello, this);
	hello_thread.detach();
//	&A::FunctA, this

	std::cout << "network init complete" << std::endl;
}

Network::~Network()
{
	closesocket(socket_listener);
	closesocket(socket_sender);
	closesocket(socket_hello);
	WSACleanup();
}

bool Network::generate_header(std::string ip, bool is_ack, MessageHeader* sig)
{
	in_addr receiver;
	if (inet_pton(AF_INET, ip.c_str(), &receiver) == false)
	{
		std::cout << "Invalid address: " << ip << std::endl;
		return false;
	}
	sig->receiver_ip = receiver;
	sig->sender_ip = own_ip;
	sig->timestamp = time(nullptr);
	sig->is_ack = is_ack;
	sig->is_delivered = false;
	sig->rnd = rand();
	return true;
}

bool Network::attach_header(char* buf, std::string message, MessageHeader sig)
{
	memcpy(buf, &sig, sizeof(sig));
	buf = buf + sizeof(sig);
	strcpy(buf, message.c_str());

	return true;
}

bool Network::send_message(MessageHeader header, std::string message, int port)
{
	char buf[BUFFER_SIZE];
	if (strlen(message.c_str()) > MESSAGE_LENGTH)
	{
		std::cout << "Message is too large" << std::endl;
		return false;
	}
	if (attach_header(buf, message, header) == false)
	{
		return false;
	}
	if (header.receiver_ip.S_un.S_addr != own_ip.S_un.S_addr)
	{
		add_signature_to_ignore(header);
	}
//	sendlist_mutex.lock();
	for (unsigned int i = 0; i < neighbor_ip.size(); i++)
	{
		sockaddr_in addr;
		in_addr ip = neighbor_ip[i];
		char ip_buf[50];
		addr.sin_family = AF_INET;
		addr.sin_port = htons(port);
		addr.sin_addr = ip;

		int success_send = sendto(socket_sender, buf, BUFFER_SIZE, 0, (SOCKADDR*)&addr, sizeof(addr));
		if (success_send == SOCKET_ERROR)
		{
//			std::cout << "Error sending message from" << inet_ntop(AF_INET, &header.sender_ip, ip_buf, 50);
//			std::cout << " to " << inet_ntop(AF_INET, &header.receiver_ip, ip_buf, 50) << " : " << WSAGetLastError << std::endl;
//			std::cout << "via " << inet_ntop(AF_INET, &ip, ip_buf, 50) << std::endl;
		}
		else
		{
//			std::cout << "Message sent to " << inet_ntop(AF_INET, &ip, ip_buf, 50) << std::endl;
		}
	}
//	sendlist_mutex.unlock();
	
	return true;
}

bool Network::generate_message(std::string ip, std::string message, bool wait_for_ack)
{
	MessageHeader header;
	if (generate_header(ip, false, &header) == false)
	{
		std::cout << "Could not generate header" << std::endl;
		return false;
	}
	bool message_sent = send_message(header, message, MESSAGE_PORT);
	if (wait_for_ack)
	{
		Sleep(5000);
		//TODO добавить проверку доставки
	}
	return message_sent;
}

void Network::add_signature_to_ignore(MessageHeader header)
{
	if (find_header_signature(header) == -1)
	{
		signatures_to_ignore.push_back(header);
	}
}

void Network::receive_message()
{
	std::cout << "receiving messages" << std::endl;
	while (true)
	{
		char buf[BUFFER_SIZE];
		char ip_buf[50];
		char* message;
		sockaddr_in relay_addr;
		MessageHeader header;
		int relay_addr_size = sizeof(relay_addr);
		int bytes_read = 1;
		bytes_read = recvfrom(socket_listener, buf, BUFFER_SIZE - 1, 0, (sockaddr*)&relay_addr, &relay_addr_size);
//		sendlist_mutex.lock();
		if (find_neighbor_ip(relay_addr.sin_addr) == -1) //если получаем сообщение от не-соседа, то добавляем его как соседа
		{
			neighbor_ip.push_back(relay_addr.sin_addr);
		}
		if (bytes_read < sizeof(header))
		{
//			std::cout << "malformed message received, ignoring" << std::endl;
			continue;
		}
		memcpy(&header, buf, sizeof(header));
		message = buf + sizeof(header);
//		std::cout << "packet relayed from " << inet_ntop(AF_INET, &relay_addr.sin_addr, ip_buf, 50) << std::endl;
//		std::cout << "sender: " << inet_ntop(AF_INET, &header.sender_ip, ip_buf, 50);
//		std::cout << " receiver: " << inet_ntop(AF_INET, &header.receiver_ip, ip_buf, 50) << std::endl;

		//проверяем, нам ли адресовано это сообщение и проверяем, получали ли мы такое сообщение до этого
		//если да, то выводим на экран и формируем ответ ACK
		//если нет, то передаём всем соседям
		if (find_header_signature(header) > -1) //сообщение с такой сигнатурой уже обработано
		{
//			std::cout << "Signature is to be ignored" << std::endl << std::endl;
		}
		else
		{
			add_signature_to_ignore(header); //добавляем сигнатуру в игнор-лист, чтобы не обрабатывать повторно
			if (header.receiver_ip.S_un.S_addr == own_ip.S_un.S_addr) //если сообщение адресовано нам, то обрабатываем его
			{
				if (header.is_ack == false) //если сообщение - датаграмма (т.е. само сообщение), а не подтверждение доставки (ACK), то выводим его и посылаем обратно ACK
				{
//					std::cout << "THIS MESSAGE IS OURS, forming ACK message" << std::endl;
					std::cout << std::endl << "Message: " << message << std::endl << std::endl;
					MessageHeader ack_header = reverse_header(header);
					ack_header.is_ack = true;
					send_message(ack_header, "ACK", MESSAGE_PORT);
//					std::cout << "ACK sent" << std::endl;
				}
				else //если сообщение - ACK, то ищем, было ли у нас сообщение, соответствующее ему, и ставим пометку "доставлено"
					//если соответствующего сообщения не было, то выводим сообщение о непонятном пакете или просто игнорируем
				{
//					std::cout << "some ACK received" << std::endl << std::endl;
					int i = find_reverse_header_signature(header);
					if (i > -1)
					{
						if (signatures_to_ignore[i].is_delivered == false)
						{
							signatures_to_ignore[i].is_delivered = true;
//							std::cout << "delivery ACKed" << std::endl;
						}
						else
						{
//							std::cout << "delivery is already ACKed" << std::endl << std::endl;
						}
					}
					else
					{
//						std::cout << "this ACK has no corresponding message" << std::endl << std::endl;
					}
				}
			}
			else //если же сообщение адресовано не нам, то пересылаем его дальше
			{
				send_message(header, std::string(message), MESSAGE_PORT);
			}
		}
//		sendlist_mutex.unlock();
	}
}

void Network::receive_hello()
{
	std::cout << "receiving hello" << std::endl;
	while (true)
	{
		char buf[BUFFER_SIZE];
		char ip_buf[50];
		sockaddr_in relay_addr;
//		MessageHeader header;
		int relay_addr_size = sizeof(relay_addr);
		int bytes_read = recvfrom(socket_hello, buf, BUFFER_SIZE - 1, 0, (sockaddr*)&relay_addr, &relay_addr_size);
//		sendlist_mutex.lock();
		std::cout << "HELLO received from " << inet_ntop(AF_INET, &relay_addr.sin_addr, ip_buf, 50) << std::endl;
		if (find_neighbor_ip(relay_addr.sin_addr) == -1)
		{
			std::cout << " , adding to neighbors" << std::endl;
			neighbor_ip.push_back(relay_addr.sin_addr);
		}
		else
		{
			std::cout << " , but it is already marked as neighbor" << std::endl;
		}
//		sendlist_mutex.unlock();
	}
}

void Network::send_hello()
{
//	sendlist_mutex.lock();
	for (in_addr ip : neighbor_ip)
	{
		sockaddr_in addr;
		addr.sin_family = AF_INET;
		addr.sin_port = htons(HELLO_PORT);
		addr.sin_addr = ip;
		sendto(socket_sender, "HELLO", strlen("HELLO"), 0, (sockaddr*)&addr, sizeof(addr));
	}
	neighbor_ip.push_back(own_ip);
//	sendlist_mutex.unlock();
}

void Network::bind_socket(SOCKET& s, int port)
{
	s = socket(AF_INET, SOCK_DGRAM, 0);
	if (s == INVALID_SOCKET)
	{
		std::cout << "Create socket error: " << WSAGetLastError << std::endl;
		exit(254);
	}
	std::cout << "socket created" << std::endl;

	sockaddr_in tcpaddr;
	memset(&tcpaddr, 0, sizeof(tcpaddr));
	tcpaddr.sin_family = AF_INET;
	tcpaddr.sin_port = htons(port);
	tcpaddr.sin_addr.s_addr = INADDR_ANY;

	if (bind(s, (sockaddr*)&tcpaddr, sizeof(tcpaddr)) < 0)
	{
		perror("bind");
		exit(253);
	}
	std::cout << "socket bound on port " << port << std::endl;
}

bool Network::add_ip_to_neighbors(in_addr ip)
{
	if (find_neighbor_ip(ip) == -1)
	{
		neighbor_ip.push_back(ip);
	}
	return true;
}

int Network::find_neighbor_ip(in_addr ip)
{
	for (int i = 0; i < neighbor_ip.size(); i++)
	{
		if (neighbor_ip[i].S_un.S_addr == ip.S_un.S_addr)
		{
			return i;
		}
	}
	return -1;
}

int Network::find_header_signature(MessageHeader header)
{
//	signatures_to_ignore_mutex.lock();
	int result = -1;
	for (int i = 0; i < signatures_to_ignore.size(); i++)
	{
		MessageHeader current = signatures_to_ignore[i];
		if (header.sender_ip.S_un.S_addr == current.sender_ip.S_un.S_addr && header.rnd == current.rnd && header.timestamp == current.timestamp)
		{
			result = i;
			break;
		}
	}
//	signatures_to_ignore_mutex.unlock();
	return result;
}

int Network::find_reverse_header_signature(MessageHeader header)
{
//	signatures_to_ignore_mutex.lock();
	int result = -1;
	for (int i = 0; i < signatures_to_ignore.size(); i++)
	{
		MessageHeader current = signatures_to_ignore[i];
		if (header.sender_ip.S_un.S_addr == current.sender_ip.S_un.S_addr && header.rnd == current.rnd && header.timestamp == current.timestamp)
		{
			result = i;
			break;
		}
	}
//	signatures_to_ignore_mutex.unlock();
	return result;
}

MessageHeader Network::reverse_header(MessageHeader header)
{
	MessageHeader result = header;
	std::swap(result.receiver_ip, result.sender_ip);
	return result;
}
