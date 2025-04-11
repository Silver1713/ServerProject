#include "Receiver.hpp"

#include <iostream>

#include "Packet.hpp"


Receiver::Receiver() : udp_socket(), lastResponse(), retransmit_index(), session_id(), window_size(), base(), next_seq_num(), received_packets(), dest_ip_addr(), dest_port()
{
	
}

Receiver::Receiver(uint32_t sessionID, size_t file_size) : session_id(sessionID), fileSize(file_size)
{
	if (fileSize < 1000000)
	{
		window_size = 32;
	}
	else if (fileSize < 100000000)
	{
		window_size = 128;
	}
	else if (fileSize < 1000000000)
	{
		window_size = 256;
	}
	else if (fileSize < 10000000000)
	{
		window_size = 1024;
	}
}


bool Receiver::create_socket(uint16_t port, bool convert_to_network)
{
	addrinfo hints{};
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_protocol = IPPROTO_UDP;

	udp_socket = socket(hints.ai_family, hints.ai_socktype, hints.ai_protocol);

	if (INVALID_SOCKET == udp_socket)
	{
		std::cerr << "socket() failed." << std::endl;

		return false;
	}

	struct sockaddr_in udp_addr_in;
	udp_addr_in.sin_family = AF_INET;
	udp_addr_in.sin_port = convert_to_network ? htons(port) : port;
	udp_addr_in.sin_addr.S_un.S_addr = INADDR_ANY;

	int errorCode;
	errorCode = bind(udp_socket, reinterpret_cast<sockaddr*>(&udp_addr_in), sizeof(udp_addr_in));

	if (NO_ERROR != errorCode)
	{
		std::cerr << "bind() failed." << std::endl;
		closesocket(udp_socket);
		udp_socket = INVALID_SOCKET;
	}


	return true;



}


void Receiver::receive(SOCKET udpSocket)
{
	u_long mode = 1;
	ioctlsocket(udp_socket, FIONBIO, &mode);
	lastResponse = std::chrono::steady_clock::now();
	while (state != FINISH)
	{
		if (state == AWAIT_PACKET)
		{
			char* buffer = new char[2000];
			
			sockaddr_in destAddr;
			destAddr.sin_family = AF_INET;
			destAddr.sin_port = dest_port;
			destAddr.sin_addr.S_un.S_addr = dest_ip_addr;
			int len = sizeof(destAddr);
			int recvByte = recvfrom(udp_socket, buffer, PACKET_SIZE, MSG_PEEK, reinterpret_cast<sockaddr*>(&destAddr), &len);

			Packet pkt{ buffer };
			if (session_id != pkt.session_id)
			{
				delete[] buffer;
				continue;
			}

			if (recvByte == SOCKET_ERROR)
			{
				int WSErr = WSAGetLastError();

				if (WSErr == WSAEWOULDBLOCK)
				{
					
				}

			}

			bool windowMoving = true;
			if (recvByte > 0)
			{
				Packet* p = new Packet(buffer);
				received_packets.push_back(p);
				state = RECEIVE_PACKET;

				if (p->seq_num == base)
				{
					windowMoving = true;
				}

			}

			if (windowMoving)
				state = MOVE_WINDOW;

			if (isTimeout())
			{
				state = TIMEOUT_REC;
				continue;
			}


			

		}
		if (state == MOVE_WINDOW)
		{
			int lastPkt = fileSize / PACKET_SIZE;
			int remainingSeq = fileSize % PACKET_SIZE;

			if (remainingSeq)
				lastPkt++;

			if (base + window_size < static_cast<unsigned>(lastPkt))
			{
				base++;
				state = AWAIT_PACKET;
			}
			else if (next_seq_num == static_cast<uint32_t>(lastPkt + 1))
			{
				state = FINISH;
				continue;
			}
			else
			{
				state = FINISH;
				continue;
			}
				
		}
		if (state == TIMEOUT_REC)
		{
			state = FINISH;
		}
	}
}


bool Receiver::isTimeout()
{
	using namespace  std::chrono_literals;
	std::chrono::steady_clock::duration duration = std::chrono::steady_clock::now() - lastResponse;
	return duration > 5s;


}




