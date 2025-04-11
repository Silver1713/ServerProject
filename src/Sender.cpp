#include "Sender.hpp"

#include <filesystem>
#include <iostream>
#include <ws2tcpip.h>

#include "CMID.hpp"
#include "FileManager.hpp"
#include "Packet.hpp"

Sender::Sender() : session_id{ 0 }, window_size{ 0 }, base{ 0 }, next_seq_num{ 0 }
{

}

Sender::Sender(uint32_t session_id, std::string file_name) : session_id(session_id), window_size{ 0 }, base{ 0 }, next_seq_num{ 0 }
{
	size_t fSize = FileManager::files[file_name].data_size;


	if (fSize < 1000000)
	{
		window_size = 32;
	}
	else if (fSize < 100000000)
	{
		window_size = 128;
	}
	else if (fSize < 1000000000)
	{
		window_size = 256;
	}
	else if (fSize < 10000000000)
	{
		window_size = 1024;
	}

	load_file(file_name);
}


void Sender::load_file(std::string file_name)
{
	FileData& data = FileManager::files[file_name];
	data.ReadFile(); // Load the file into memory

	//Split the file into packets
	uint32_t numPackets = data.data_size / PAYLOAD_SIZE;
	uint32_t remainingBytes = data.data_size - numPackets * PAYLOAD_SIZE;

	for (int i = 0; i < numPackets; i++)
	{
		Packet* p = new Packet();
		p->payload_cmd = UDPCMID::PAYLOAD_SEND;
		p->seq_num = i;
		p->data_len = PACKET_SIZE;
		p->payload_len = PACKET_SIZE - PACKET_BUFFER_INITIAL;
		p->payload = std::make_shared<char[]>(PAYLOAD_SIZE);
		memcpy(p->payload.get(), data.data.get() + i * PAYLOAD_SIZE, PAYLOAD_SIZE);
		p->CalculateChecksum();
		packets.push_back(p);
	}

	if (remainingBytes > 0)
	{
		Packet* p = new Packet();
		p->payload_cmd = UDPCMID::PAYLOAD_SEND;
		p->seq_num = numPackets;
		p->data_len = remainingBytes + PACKET_BUFFER_INITIAL;
		p->payload_len = remainingBytes;
		p->payload = std::make_shared<char[]>(remainingBytes);
		memcpy(p->payload.get(), data.data.get() + numPackets * PAYLOAD_SIZE, remainingBytes);
		p->CalculateChecksum();
		packets.push_back(p);
	}


	loaded_file_name = file_name;
	
}



void Sender::send(SOCKET udpSocket)
{
	// Set socket to non-blocking
	u_long mode = 1;
	ioctlsocket(udpSocket, FIONBIO, &mode);
	

	while (state != FINISH)
	{

		//STATES
		/*
		 *	WHILE NOT FINISH
		 *		PREPARE_PACKET
		 *			LOAD FILE SPLITS INTO PACKETS
		 *			SET NEXT_SEQ_NUM TO FIRST PACKET SEQ_NUM
		 *			STATE = SEND_PACKET
		 *			BASE TO FIRST PACKET SEQ_NUM
		 *			
		 *
		 *		
		 */
		if (state == SRState::PREPARE_PACKET)
		{
			load_file(loaded_file_name);
			next_seq_num = packets[0]->seq_num;
			state = SRState::SEND_PACKET;
			base = packets[0]->seq_num;

		}
		else if (state == SRState::SEND_PACKET)
		{
			// Build addr info
			sockaddr_in destAddr;
			destAddr.sin_family = AF_INET;
			destAddr.sin_port = dest_port;
			destAddr.sin_addr.S_un.S_addr = dest_ip_addr;

			// if next_seq_num is less than base + window_size
			if ((next_seq_num < base + window_size) && next_seq_num < packets.size()) {
				Packet* p = packets[next_seq_num];
				p->start_time_stamp = std::chrono::steady_clock::now();
				char* buffer = p->Serialize();
				int data = sendto(udpSocket, buffer, PACKET_SIZE, 0, reinterpret_cast<sockaddr*>(&destAddr), sizeof(destAddr));

				if (data == SOCKET_ERROR)
				{
					int WSError = WSAGetLastError();

					if (WSError == WSAEWOULDBLOCK)
					{
						delete[] buffer;
						buffer = nullptr;
						continue;
					}

					std::cout << WSError << std::endl;
					continue;
				}

				if (next_seq_num + 1 <= base + window_size)
				{
					state = SRState::SEND_NEXT_PACKET;
				}
				delete[] buffer;
			}
			else {
				state = SRState::WAIT_ACK;
			}
				
		}
		else if (state == SRState::WAIT_ACK)
		{
			// Wait for ACK
			char ackBuffer[PACKET_SIZE];
			int bytes = recvfrom(udpSocket, ackBuffer, PACKET_SIZE, 0, nullptr, nullptr);

			if (bytes == SOCKET_ERROR)
			{
				int e = WSAGetLastError();
				if (e == WSAEWOULDBLOCK)
				{
					
				}
				else {
					return;
				}
			}

			if (bytes < 0)
			{
				//Check timeout
				for (uint32_t b = base; b < min(base + window_size, packets.size()); b++)
				{
					if (b < packets.size())
					{
						Packet* p = packets[b];
						if (p->isTimeout())
						{
							//retransmit packet
							state = RESEND_PACKET;
							retransmit_index = b;
							break;

						}
						else continue;

					}
					else continue;
				}
			}

			if (bytes > 0)
			{
				Packet ackPacket(ackBuffer);
				if (ackPacket.payload_cmd == UDPCMID::PAYLOAD_ACK)
				{
					if (acked_packets.find(ackPacket.seq_num) == acked_packets.end())
					{
						acked_packets.insert(ackPacket.seq_num);
						state = SRState::ACK_RECEIVED;
					}

					if (ackPacket.seq_num == base)
					{
						state = SRState::MOVE_WINDOW;
					}
				}
				else
				{

				}
			}
		}
		else if (state == SRState::ACK_RECEIVED)
		{
			std::cout << "ACKED RECEIVED\n";
			if (acked_packets.contains(base))
			{
				//Base is ACK
				state = SRState::MOVE_WINDOW;
			}
			else state = SRState::WAIT_ACK;
		}
		else if (state == SRState::TIMEOUT)
		{
			state = SRState::RESEND_PACKET;
		}
		else if (state == SRState::SEND_NEXT_PACKET)
		{
			if (next_seq_num == packets.size())
			{
				break;
			}
			else
			{
				next_seq_num++;
				state = SRState::SEND_PACKET;
			}
		}
		else if (state == SRState::RESEND_PACKET)
		{
			// Build addr info
			sockaddr_in destAddr;
			destAddr.sin_family = AF_INET;
			destAddr.sin_port = dest_port;
			destAddr.sin_addr.S_un.S_addr = dest_ip_addr;


			for (uint32_t b = retransmit_index; b <= min(packets.size(), base+ window_size); b++)
			{
				if (b < packets.size())
				{
					Packet* p = packets[b];
					if (p->isTimeout())
					{

						//retransmit packet
						char* buf = p->Serialize(true);
						int r = sendto(udpSocket, buf, PACKET_SIZE, 0, reinterpret_cast<sockaddr*>(&destAddr), sizeof(destAddr));

						std::cout << "Retransmit Packet " << p->seq_num << " With Payload of :" << p->payload_len << "B" << '\n';

						if (r == SOCKET_ERROR)
						{
							int err_no = WSAGetLastError();
							if (err_no == WSAEWOULDBLOCK)
							{

								// Resend the packets
								using namespace std::chrono_literals;
								std::this_thread::sleep_for(100ms); 

								b--;
								continue;
							}

						}

						p->start_time_stamp = std::chrono::steady_clock::now();

						continue;

					}

				}
			}
			state = SRState::WAIT_ACK;
		}
		else if (state == SRState::MOVE_WINDOW)
		{
			base++;
			if (base + window_size > packets.size())
			{
				state = FINISH;
			}
			else state = SEND_PACKET;
		}
	}

}

bool Sender::create_socket(std::string ip, uint16_t port, bool convert_to_network)
{
	addrinfo hints{};
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_protocol = IPPROTO_UDP;

	udp_socket = socket(hints.ai_family, hints.ai_socktype, hints.ai_protocol);
	uint32_t nIP{};
	inet_pton(AF_INET, ip.c_str(), &nIP);

	if (INVALID_SOCKET == udp_socket)
	{
		std::cerr << "socket() failed." << std::endl;
		
		return false;
	}
	struct sockaddr_in udp_addr_in;
	udp_addr_in.sin_family = AF_INET;
	udp_addr_in.sin_port = convert_to_network ? htons(port) : port;
	udp_addr_in.sin_addr.S_un.S_addr = nIP;

	int errorCode;
	errorCode = bind(udp_socket, reinterpret_cast<sockaddr*>(&udp_addr_in), sizeof(udp_addr_in));

	if (NO_ERROR != errorCode)
	{
		std::cerr << "bind() failed." << std::endl;
		std::cout << WSAGetLastError() << std::endl;
		closesocket(udp_socket);
		udp_socket = INVALID_SOCKET;
	}



}


bool Sender::checkTimeout()
{
	auto now = std::chrono::steady_clock::now();


	auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastResponse);

	using namespace std::chrono_literals;

	return (duration > 5s);
}



