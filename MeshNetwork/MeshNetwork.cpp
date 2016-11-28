// NetworkObjects.cpp : Defines the entry point for the console application.
//

#include "MessageHeader.h"
#include "Network.h"

void input_ips(Network* network);

int main()
{
	Network network;
	input_ips(&network);
//	std::thread hello_thread(network.receive_message);
	network.send_hello();
	while (true)
	{
		std::string ip;
		std::string message;
		std::cout << "input recipient ip" << std::endl;
		std::getline(std::cin, ip);
		in_addr addr;
		if (inet_pton(AF_INET, ip.c_str(), &addr) != 1)
		{
			std::cout << "incorrect ip" << std::endl;
			continue;
		}
		std::cout << "input your message" << std::endl;
		std::getline(std::cin, message);
		network.generate_message(ip, message, false);
		Sleep(5000);
	}
    return 0;
}

void input_ips(Network* network)
{
	std::string ip_string;
	//ДОЛЖЕН совпадать с адресом машины, иначе пакеты не дойдут
	std::cout << "input your own ip address" << std::endl;
	std::cin >> ip_string;
	in_addr addr;
	if (inet_pton(AF_INET, ip_string.c_str(), &addr) == 1)
	{
		network->own_ip = addr;
	}
	else
	{
		std::cout << "Incorrect own ip, shutting down" << std::endl;
		exit(211);
	}

	//ДОЛЖЕН совпадать с адресами машин, иначе пакеты не дойдут
	std::cout << "input neighbor ip addresses" << std::endl;
	std::cout << "start line with \".\" to finish" << std::endl;
	while (true)
	{
		std::string ip_string;
		std::cin >> ip_string;
//		std::getline(std::cin, ip);
		if (ip_string[0] == '.')
		{
			break;
		}
		if (inet_pton(AF_INET, ip_string.c_str(), &addr) == 1)
		{
			network->add_ip_to_neighbors(addr);
		}
		else
		{
			std::cout << "Incorrect ip " << ip_string << std::endl;
		}
	}
	std::cin.ignore();
}