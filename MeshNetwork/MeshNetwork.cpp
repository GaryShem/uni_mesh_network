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
	input_ips(&network); // вводим нужные айпи
	network.send_hello(); // отправляем приветы соседям
	while (true)
	{
		std::string ip;
		std::string message;
		std::cout << "Введите IP получателя" << std::endl;
		std::getline(std::cin, ip);
		in_addr addr;
		if (inet_pton(AF_INET, ip.c_str(), &addr) != 1)
		{
			std::cout << "Некорректный IP" << std::endl;
			continue;
		}
		std::cout << "Введите сообщение" << std::endl;
		std::getline(std::cin, message);
		network.generate_message(ip, message, true); // true - если мы хотим ждать потверждение доставки (ack)
		Sleep(5000);
	}
    return 0;
}

void input_ips(Network* network)
{
	std::string ip_string;
	//ДОЛЖЕН совпадать с адресом машины, иначе пакеты могут не дойти
	std::cout << "Введите свой IP-адрес" << std::endl;
	std::cin >> ip_string;
	in_addr addr;
	if (inet_pton(AF_INET, ip_string.c_str(), &addr) == 1)
	{
		network->own_ip = addr;
	}
	else
	{
		std::cout << "Вы не смогли ввести свой IP. Закрытие программы.." << std::endl;
		exit(211);
	}

	//ДОЛЖЕН совпадать с адресами машин, иначе пакеты не дойдут
	std::cout << "Начало ввода IP соседей." << std::endl;
	std::cout << "Начните строку с \".\" для завершения ввода." << std::endl;
	while (true)
	{
		std::string ip_string;
		std::cin >> ip_string;
//		std::getline(std::cin, ip);
		if (ip_string[0] == '.')
		{
			break;
		}
		if (inet_pton(AF_INET, ip_string.c_str(), &addr) == 1) // если айпи введён корректно
		{
			network->add_ip_to_neighbors(addr);
		}
		else
		{
			std::cout << "Неккоректный IP " << ip_string << std::endl;
		}
	}
	std::cin.ignore(); // для игнорирования n\, на всякий случай
}