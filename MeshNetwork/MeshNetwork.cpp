// NetworkObjects.cpp : Defines the entry point for the console application.
//

#include "MessageHeader.h"
#include "Network.h"

void input_ips(Network* network);

int main()
{
	SetConsoleOutputCP(1251);
	SetConsoleCP(1251);
	Network network;
	input_ips(&network); // ������ ������ ����
	network.send_hello(); // ���������� ������� �������
	while (true)
	{
		std::string ip;
		std::string message;
		std::cout << "������� IP ����������" << std::endl;
		std::getline(std::cin, ip);
		in_addr addr;
		if (inet_pton(AF_INET, ip.c_str(), &addr) != 1)
		{
			std::cout << "������������ IP" << std::endl;
			continue;
		}
		std::cout << "������� ���������" << std::endl;
		std::getline(std::cin, message);
		network.generate_message(ip, message, true); // true - ���� �� ����� ����� ������������ �������� (ack)
		Sleep(5000);
	}
    return 0;
}

void input_ips(Network* network)
{
	std::string ip_string;
	//������ ��������� � ������� ������, ����� ������ ����� �� �����
	std::cout << "������� ���� IP-�����" << std::endl;
	std::cin >> ip_string;
	in_addr addr;
	if (inet_pton(AF_INET, ip_string.c_str(), &addr) == 1)
	{
		network->own_ip = addr;
	}
	else
	{
		std::cout << "�� �� ������ ������ ���� IP. �������� ���������.." << std::endl;
		exit(211);
	}

	//������ ��������� � �������� �����, ����� ������ �� ������
	std::cout << "������ ����� IP �������." << std::endl;
	std::cout << "������� ������ � \".\" ��� ���������� �����." << std::endl;
	while (true)
	{
		std::string ip_string;
		std::cin >> ip_string;
//		std::getline(std::cin, ip);
		if (ip_string[0] == '.')
		{
			break;
		}
		if (inet_pton(AF_INET, ip_string.c_str(), &addr) == 1) // ���� ���� ����� ���������
		{
			network->add_ip_to_neighbors(addr);
		}
		else
		{
			std::cout << "������������ IP " << ip_string << std::endl;
		}
	}
	std::cin.ignore(); // ��� ������������� n\, �� ������ ������
}