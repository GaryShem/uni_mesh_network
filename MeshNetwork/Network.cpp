#include "Network.h"
#include <strstream>


Network::Network()
{
	network_ready = false;
	if (WSAStartup(MAKEWORD(1, 1), &wsaData) != 0) { // если не создалась "площадка" для сокетов
		exit(255);
	}
	srand(time(NULL)); //для рандома

	socket_sender = socket(AF_INET, SOCK_DGRAM, 0); // сокет для отправки пакетов
	if (socket_sender == INVALID_SOCKET)
	{
		std::cout << "Ошибка создания сокета отправки: " << WSAGetLastError << std::endl; // если не удалось создать сокет отправки
		exit(254);
	}
//	std::cout << "сокет для отправкии сообщений создан" << std::endl;

	bind_socket(socket_listener, MESSAGE_PORT); // сокет для получения пакетов
	bind_socket(socket_hello, HELLO_PORT); // сокет для обмена приветами с соседями

	// нам нужны два потока, для сообщений и для приветов соседей
	message_thread = std::thread(&Network::receive_message, this);
	message_thread.detach(); // если не отделить поток от переменной, то при закрытии программы вылетает ошибка
	hello_thread = std::thread(&Network::receive_hello, this);
	hello_thread.detach();
//	&A::FunctA, this

//	std::cout << "сеть работает!" << std::endl;
}

Network::~Network()
{
	// так после закрытия программы точно все сокеты поудаляются и всё почистится
	closesocket(socket_listener);
	closesocket(socket_sender);
	closesocket(socket_hello);
	WSACleanup();
}

bool Network::generate_header(std::string ip, bool is_ack, MessageHeader* sig)
{
	in_addr receiver;
	if (inet_pton(AF_INET, ip.c_str(), &receiver) == false) // если что-то не так с айпишником
	{
		std::cout << "Неправильный IP-адрес " << ip << std::endl;
		return false;
	}
	sig->receiver_ip = receiver; // получатель
	sig->sender_ip = own_ip; // отправитель (свой айпи) 
	sig->timestamp = time(nullptr); // время на данный момент
	sig->is_ack = is_ack; // ответка ли это
	sig->is_delivered = false; // доставлено ли сообщение
	sig->rnd = rand(); // рандом
	return true;
}

bool Network::attach_header(char* buf, std::string message, MessageHeader sig)
{
	// собираем в буффер наш пакет, хэдер + сообщение
	memcpy(buf, &sig, sizeof(sig));
	buf = buf + sizeof(sig);
	strcpy(buf, message.c_str());

	return true;
}

bool Network::send_message(MessageHeader header, std::string message, int port)
{
	char buf[BUFFER_SIZE]; // наш пакет с хэдером и сообщением
	if (message.length() > MESSAGE_LENGTH) // если переполнили пакет, то всё, нельзя
	{
		std::cout << "Сообщение слишком большое" << std::endl;
		return false;
	}
	if (attach_header(buf, message, header) == false) //  если пакет почему-то не собрался
	{
		return false;
	}
	if (header.receiver_ip.S_un.S_addr != own_ip.S_un.S_addr) // если сообщение не нам
	{
		add_signature_to_ignore(header); // пытаемся добавить в игнор
	}
	for (unsigned int i = 0; i < neighbor_ip_list.size(); i++) // идём по списку соседей и отправляем пакет
	{
		sockaddr_in addr; // говорит куда я буду посылать сообщение по сокету
		in_addr ip = neighbor_ip_list[i];
		char ip_buf[50];
		addr.sin_family = AF_INET;
		addr.sin_port = htons(port);
		addr.sin_addr = ip;

		int success_send = sendto(socket_sender, buf, BUFFER_SIZE-1, 0, (SOCKADDR*)&addr, sizeof(addr)); // передаём пакет по сокету, говорим куда через addr-структуру
//		if (success_send == SOCKET_ERROR)
//		{
////			std::cout << "Error sending message from" << inet_ntop(AF_INET, &header.sender_ip, ip_buf, 50);
////			std::cout << " to " << inet_ntop(AF_INET, &header.receiver_ip, ip_buf, 50) << " : " << WSAGetLastError << std::endl;
////			std::cout << "via " << inet_ntop(AF_INET, &ip, ip_buf, 50) << std::endl;
//		}
//		else
//		{
////			std::cout << "Message sent to " << inet_ntop(AF_INET, &ip, ip_buf, 50) << std::endl;
//		}
	}
//	sendlist_mutex.unlock();
	
	return true;
}

bool Network::generate_message(std::string ip, std::string message, bool wait_for_ack)
{
	MessageHeader header;
	if (generate_header(ip, false, &header) == false) // генерируем хэдер, если что-то пошло не так, то выводим ошибку и ничего не посылаем
	{
		std::cout << "Хэдер не сгенерировался" << std::endl;
		return false;
	}
	bool message_sent = send_message(header, message, MESSAGE_PORT); // посылаем сообщение
	if (wait_for_ack) // сейчас мы всегда ждём подтверждения доставки
	{
		Sleep(5000); // ждём 5 секунд
		int index = find_header_signature(header); // теперь проверяем было ли хотя бы отправлено сообщение
		if (index > -1) // здесь мы знаем, что сообщение хотя бы было отправлено
		{
			if (signatures_to_ignore[index].is_delivered == true) // если пакет-подтверждение доставки было получено
			{
				std::cout << std::endl << "Сообщение доставлено" << std::endl;
			}
			else // тут точно мы не знаем, дошло ли в итоге сообщение или нет, но да ладно
			{
				std::cout << std::endl << "Сообщение могло потеряться :(" << std::endl;
			}
		}
	}
	return message_sent;
}

void Network::add_signature_to_ignore(MessageHeader header)
{
	if (find_header_signature(header) == -1) // если ещё не было в игноре
	{
		signatures_to_ignore.push_back(header);
	}
}

void Network::receive_message() // ожидаем сообщения
{
//	std::cout << "ожидаем сообщения" << std::endl;
	while (true)
	{
		char buf[BUFFER_SIZE];
		char ip_buf[50];
		char* message;
		sockaddr_in relay_addr; // откуда мы как бы ждём эти пакеты, addr-структура
		MessageHeader header;
		int relay_addr_size = sizeof(relay_addr);
		int bytes_read;
		bytes_read = recvfrom(socket_listener, buf, BUFFER_SIZE - 1, 0, (sockaddr*)&relay_addr, &relay_addr_size); // здесь мы ждём пакеты
		if (bytes_read < 0)
		{
			auto error = WSAGetLastError();
			continue;
		}
		buf[bytes_read] = '\0'; //получили что-то, на всякий случай починили конец буфферу
		if (find_neighbor_ip(relay_addr.sin_addr) == -1) //если получаем сообщение от не-соседа, то добавляем его как соседа
		{
			neighbor_ip_list.push_back(relay_addr.sin_addr); // добавили в конец
		}
		if (bytes_read < sizeof(header)) // если прочитали меньше, чем размер нашего хэдера, то уже что-то не так
		{
//			std::cout << "Получили неправильно сформированное сообщение, игнорируем его" << std::endl;
			continue;
		}

		// находим начало сообщения в буффере
		memcpy(&header, buf, sizeof(header)); // копируем хэдер (из буффера в переменную header)
		message = buf + sizeof(header); // ставим указатель на начало сообщения, после хэдера

		if (header.is_ack == true && header.receiver_ip.S_un.S_addr == own_ip.S_un.S_addr) 
		//если сообщение - ACK, и адресовано нам, то ищем, было ли у нас сообщение, соответствующее ему, и ставим пометку "доставлено"
		//если соответствующего сообщения не было, то выводим сообщение о непонятном пакете или просто игнорируем
		{
//			std::cout << "Пришёл какой-то ACK" << std::endl << std::endl;
			int i = find_reverse_header_signature(header); // тогда ищем обратный хэдер (то есть первоначальный, т.к. другого у отправителя не будет)
			if (i > -1) // если нашли сведения об отправленном пакете
			{
				if (signatures_to_ignore[i].is_delivered == false) // если ещё не подтверждали его доставку получателю
				{
					signatures_to_ignore[i].is_delivered = true; // то подтверждаем доставку (сообщение куда надо было доставлено)
//					std::cout << "Сообщение было доставлено получателю" << std::endl;
				}
				else
				{
//					std::cout << "Доставка сообщения уже была подтверждена" << std::endl << std::endl;
				}
			}
			else
			{
				std::cout << "Нет сообщения, доставку которого подтверждает пришедший ACK" << std::endl << std::endl;
			}
		}

		// в случае, если это не подтверждение отправки получателю для нас(отправителя), то смотрим что делать с этим сообщением
		else if (find_header_signature(header) > -1) //сообщение с такой сигнатурой уже обработано, то ничего не делаем
		{
//			std::cout << "Пакет будет проигнорирован" << std::endl << std::endl;
		}
		else
		{
			add_signature_to_ignore(header); //добавляем сигнатуру в игнор-лист, чтобы не обрабатывать повторно
			if (header.receiver_ip.S_un.S_addr == own_ip.S_un.S_addr) //если сообщение адресовано нам, то обрабатываем его
			{
				//выводим полученное сообщение и посылаем обратно ACK
//				std::cout << std::endl << "ЭТО СООБЩЕНИЕ НАШЕ, создаём ACK" << std::endl;
				std::cout << std::endl << "Сообщение: " << std::string(message) << std::endl << std::endl;
				MessageHeader ack_header = reverse_header(header); // меняем в хэжере местами айпи отправителя и получателя
				ack_header.is_ack = true; // говорим, что теперь наш обычный пакет это ack-пакет (пакет-подтверждение)
				send_message(ack_header, "ACK", MESSAGE_PORT);
				//std::cout << "ACK отправлен" << std::endl;

			}
			else //если же сообщение адресовано не нам, то пересылаем его дальше всем соседям
			{
				send_message(header, std::string(message), MESSAGE_PORT);
			}
		}
	}
}

void Network::receive_hello() // ожидаем приветы от соседей
{
//	std::cout << "ожидаем приветы" << std::endl;
	while (true)
	{
		char buf[BUFFER_SIZE];
		char ip_buf[50];
		sockaddr_in relay_addr; // откуда мы как бы ждём эти приветы, addr-структура
//		MessageHeader header;
		int relay_addr_size = sizeof(relay_addr);
		int bytes_read = recvfrom(socket_hello, buf, BUFFER_SIZE - 1, 0, (sockaddr*)&relay_addr, &relay_addr_size); // здесь ожидаем новый привет от нового соседа
		std::cout << "Пришёл ПРИВЕТ от " << inet_ntop(AF_INET, &relay_addr.sin_addr, ip_buf, 50) << std::endl; 
		if (find_neighbor_ip(relay_addr.sin_addr) == -1) // если такого соседа у нас ещё не было
		{
			std::cout << " , добавлен в соседы" << std::endl;
			neighbor_ip_list.push_back(relay_addr.sin_addr); // добавляем его в соседы
		}
		else
		{
			std::cout << " , но он уже и так наш сосед" << std::endl;
		}
	}
}

void Network::send_hello() // отправка приветов соседям
{
	for (in_addr ip : neighbor_ip_list) // отправляем заданным соседям привет, чтобы они знали, что мы их новый сосед
	{
		sockaddr_in addr;
		addr.sin_family = AF_INET;
		addr.sin_port = htons(HELLO_PORT);
		addr.sin_addr = ip;
		sendto(socket_sender, "ПРИВЕТ", strlen("ПРИВЕТ"), 0, (sockaddr*)&addr, sizeof(addr));
	}
	neighbor_ip_list.push_back(own_ip); // добавляем себя в соседи себе (делаем после, чтобы себе привет не слать
}

void Network::bind_socket(SOCKET& s, int port) // создаёт сокет и ставит порт на прослушку
{
	s = socket(AF_INET, SOCK_DGRAM, 0); // создаём сокет
	if (s == INVALID_SOCKET)
	{
		std::cout << "Ошибка создания сокета: " << WSAGetLastError << std::endl; // сворачиваемся
		exit(254);
	}
//	std::cout << "Сокет был создан" << std::endl;

	sockaddr_in tcpaddr;
	memset(&tcpaddr, 0, sizeof(tcpaddr));
	tcpaddr.sin_family = AF_INET;
	tcpaddr.sin_port = htons(port);
	tcpaddr.sin_addr.s_addr = INADDR_ANY;

	if (bind(s, (sockaddr*)&tcpaddr, sizeof(tcpaddr)) == SOCKET_ERROR) // привязываем сокет к порту, если не получилось
	{
		std::cout << "Ошибка привязки" << std::endl;
		exit(253);
	}
//	std::cout << "Сокет забиндился " << port << std::endl;
}

bool Network::add_ip_to_neighbors(in_addr ip) // добавляем айпи в соседи
{
	if (find_neighbor_ip(ip) == -1) // если до этого он не был нашим соседом
	{
		neighbor_ip_list.push_back(ip);
	}
	return true;
}

int Network::find_neighbor_ip(in_addr ip) // ищем айпи в списке соседей
{
	for (int i = 0; i < neighbor_ip_list.size(); i++)
	{
		if (neighbor_ip_list[i].S_un.S_addr == ip.S_un.S_addr)
		{
			return i;
		}
	}
	return -1;
}

int Network::find_header_signature(MessageHeader header) // ищем пакет в игнор-листе прошедших уже пакетов
{
	int result = -1;
	for (int i = 0; i < signatures_to_ignore.size(); i++) // для каждого пакета в игнор-листе смотрим
	{
		MessageHeader current = signatures_to_ignore[i];
		if (header.sender_ip.S_un.S_addr == current.sender_ip.S_un.S_addr && header.rnd == current.rnd && header.timestamp == current.timestamp)
		{ // мы проверяем по айпи отправителя, рандомному числу и времени отправки, чтобы распознать нужную нам сигнатуру пакета
			result = i; // если таки нашли его
			break;
		}
	}
	return result;
}

int Network::find_reverse_header_signature(MessageHeader header) // используем это в случае, если пакет-подтверждение пришло обратно отправителю
{
	int result = -1;
	for (int i = 0; i < signatures_to_ignore.size(); i++) // смотрим отправлял ли отправитель вообще этот пакет. Тогда он тут будет
	{
		MessageHeader current = signatures_to_ignore[i];
		if (header.sender_ip.S_un.S_addr == current.receiver_ip.S_un.S_addr && header.rnd == current.rnd && header.timestamp == current.timestamp)
		{ // смотрим поля айпи отправителя и получателя наоборот, т.к. у отправителя в игнор лист не заносится пакет-подтверждение (ack), там только отправленный пакет
			result = i; // если нашли
			break;
		}
	}
	return result;
}

MessageHeader Network::reverse_header(MessageHeader header) // для свапа айпи отправителя и получателя местами, для создания пакетов-ответок
{
	MessageHeader result = header;
	std::swap(result.receiver_ip, result.sender_ip);
	return result;
}
