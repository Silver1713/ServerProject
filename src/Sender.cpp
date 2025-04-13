#include "Sender.hpp"

#include <iostream>
#include <ws2tcpip.h>

#include "FileManager.hpp"
#include "Packet.hpp"
#include "safe_cout.hpp"


std::unordered_map<std::string, UDPSender*> UDPSender::active_senders{};
std::mutex UDPSender::active_senders_mutex;



UDPSender::UDPSender() : socket_fd{ -1 }, session_id{ 0 }, file_name{ "" }, client_port{ 0 }, window_size{ 10 }, max_seq_num{ 0 }, base_seq_num{ 0 }, next_seq_num{ 0 }, ack_num{ 0 }
{}


UDPSender::UDPSender(long long socket_fd, uint32_t session_id, std::string file_name, CONNECTIONINFO info, uint32_t window_size)
	: ConnectionInfo{ info }, socket_fd {
	socket_fd
}, session_id{ session_id }, file_name{ file_name }, window_size{ window_size }, max_seq_num{ 0 }, base_seq_num{ 0 }, next_seq_num{ 0 }, ack_num{ 0 }
{
	// Initialize the sender
	Intialize();
	LoadFile();
}

UDPSender::UDPSender(long long socket_fd, uint32_t session_id, std::string file_name, uint32_t window_size)
	: socket_fd{ socket_fd }, session_id{ session_id }, file_name{ file_name }, window_size{ window_size }, max_seq_num{ 0 }, base_seq_num{ 0 }, next_seq_num{ 0 }, ack_num{ 0 }
{
	// Initialize the sender
	Intialize();

	LoadFile();
}



void UDPSender::Intialize()
{
	// Setup sender socket
	//socket_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (socket_fd == -1)
	{
		std::cerr << "NO socket loaded." << std::endl;
		return;
	}
	sockaddr_in local_addr{};
	local_addr.sin_family = AF_INET;
	local_addr.sin_port = htons(ConnectionInfo.src_port); // Local port
	inet_pton(AF_INET, ConnectionInfo.src_ip.c_str(), &local_addr.sin_addr); // Bind to my local address


	

	// Setup Client address
	client_addr.sin_family = AF_INET;
	client_addr.sin_port = htons(ConnectionInfo.dst_port); // Client destination port
	inet_pton(AF_INET, ConnectionInfo.dst_ip.c_str(), & client_addr.sin_addr); // Client destination address
	// Set socket to non-blocking mode
	u_long mode = 1; // Non-blocking mode
	if (ioctlsocket(socket_fd, FIONBIO, &mode) == SOCKET_ERROR)
	{
		std::cerr << "Failed to set socket to non-blocking mode." << std::endl;
		closesocket(socket_fd);
		socket_fd = -1;
		return;
	}
	




	
}



void UDPSender::LoadFile()
{
	{
		std::lock_guard<std::mutex> fd{ FileManager::file_mutex };
		file_data = &FileManager::files[file_name];
	}

	if (file_data == nullptr)
	{
		std::cerr << "File not found." << std::endl;
		return;
	}

	max_seq_num = file_data->data_size / PAYLOAD_SIZE;
	if (file_data->data_size % PAYLOAD_SIZE != 0)
	{
		max_seq_num++;
	}
	
	std::lock_guard<std::mutex> fd2{ send_buf_mutex };
	send_buffer.resize(max_seq_num);

	size_t total = file_data->data_size;
	size_t current = 0;
												   
	for (uint32_t i = 0; i < max_seq_num; i++)
	{
		Packet& packet = send_buffer[i];
		packet.payload_cmd = UDPCMID::PAYLOAD_DATA;
		packet.session_id = session_id;
		packet.seq_num = i;
		packet.data_len = (total < PAYLOAD_SIZE) ? total : PAYLOAD_SIZE;
		packet.payload = std::shared_ptr<char[]>(file_data->data, file_data->data.get() + current);
		packet.payload_len = packet.data_len;
		packet.CalculateChecksum();
		packet.start_time_stamp = std::chrono::steady_clock::now();
		current += packet.data_len;
		total -= packet.data_len;
	}
}






UDPSender::~UDPSender()
{
}


void UDPSender::SenderWorker()
{
	updateSessionTime();
	while (running)
	{
		
		if (next_seq_num < base_seq_num + window_size && next_seq_num < max_seq_num)
		{
			safe_cout << "Sending packet with sequence number: " << next_seq_num << std::endl;
			
			SendPacket(next_seq_num);
			if (next_seq_num == base_seq_num)
			{
				StartTimer();
			}


			++next_seq_num;
			
		}

		if (next_seq_num > max_seq_num)
		{
			std::cout << "All packets sent." << std::endl;
			std::cout << "Waiting for acknowledgements..." << std::endl;
			
		}

		if (isTimeout())
		{
			std::cout << "Timeout occurred. Resending packets..." << std::endl;
			for (uint32_t i = base_seq_num; i < next_seq_num; i++)
			{
				SendPacket(i);
			}
			StartTimer();
		}
		if (isClosing())
		{
			std::cout << "Network takes too long..." << std::endl;
			std::cout << "Closing the sender..." << std::endl;
			running = false;

		}
	}
}


void UDPSender::AcknowledgementWorker()
{

	while (running)
	{
		char buffer[2048];
		socklen_t addr_len = sizeof(client_addr);
		int peekMsg = recvfrom(socket_fd, buffer, sizeof(buffer), MSG_PEEK, (sockaddr*)&client_addr, &addr_len);

		if (peekMsg < 0)
		{
			int err = WSAGetLastError();
			if (err != WSAEWOULDBLOCK)
				std::cout << "Peek Error: " << err << std::endl;
			continue;
		}
		SecureZeroMemory(buffer, sizeof(buffer));
		int bytesReceived = recvfrom(socket_fd, buffer, sizeof(buffer), 0, (sockaddr*)&client_addr, &addr_len);
		Packet packet{ buffer, true };

		if (packet.session_id != session_id)
		{
			std::cerr << "Invalid packet received." << std::endl;
			continue;
		} else
		{
			if (packet.isAck() == false)
			{
				std::cerr << "Invalid packet received." << std::endl;
				updateSessionTime();
				continue;
			}
			updateSessionTime();
		}

		if (packet.seq_num > base_seq_num)
		{
			int s = base_seq_num;
			for (uint32_t i = s; i < packet.seq_num+1; i++)
			{
				if (send_buffer[i].is_acknowledged == false)
				{
					send_buffer[i].is_acknowledged = true;
					safe_cout << "ACK received for packet: " << i << std::endl;
					if (base_seq_num + window_size < max_seq_num)
					{
						++base_seq_num;
					}
					safe_cout << "Base sequence number updated to: " << base_seq_num << std::endl;
					if (i == max_seq_num - 1) {
						safe_cout << "Final acks has been received, download is completed.";
						running = false;
					}
					
				} else
				{
					safe_cout << "ACK already received for packet: " << i << std::endl;
				}
			}
		}
	}
}


void UDPSender::SendAsync()
{
	running = true;
	sender_thread = std::thread(&UDPSender::SenderWorker, this);
	sender_acknowledgement_thread = std::thread(&UDPSender::AcknowledgementWorker, this);

}

void UDPSender::Stop()
{
	running = false;
	if (sender_thread.joinable())
	{
		sender_thread.join();
	}
	if (sender_acknowledgement_thread.joinable())
	{
		sender_acknowledgement_thread.join();
	}
	CloseSocket();
}

void UDPSender::CloseSocket()
{
	if (socket_fd != -1)
	{
		closesocket(socket_fd);
		socket_fd = -1;
	}
}


void UDPSender::StartTimer()
{
	auto ticks = std::chrono::steady_clock::now().time_since_epoch().count();
	sender_time_since_response_epoch = ticks;
}

bool UDPSender::isTimeout()
{
	auto ticks = std::chrono::steady_clock::now().time_since_epoch().count() - sender_time_since_response_epoch;
	double ms = ticks * double(std::chrono::steady_clock::period::num) / std::chrono::steady_clock::period::den * 1000.f;
	return ms > PKT_TIMEOUT_MS;

	
}

bool UDPSender::isClosing()
{
	auto ticks = std::chrono::steady_clock::now().time_since_epoch().count() - sender_start_time_stamp_epoch;
	double ms = ticks * double(std::chrono::steady_clock::period::num) / std::chrono::steady_clock::period::den * 1000.f;
	return ms > SENDER_MAX_WAIT_TIMEOUT_MS;
}

void UDPSender::updateSessionTime()
{
	auto ticks = std::chrono::steady_clock::now().time_since_epoch().count();
	sender_start_time_stamp_epoch = ticks;
}

void UDPSender::SendPacket(uint32_t seq_num)
{
	std::lock_guard<std::mutex> fd{ send_buf_mutex };
	Packet& packet = send_buffer[seq_num];
	packet.start_time_stamp = std::chrono::steady_clock::now();
	int bytesSent = sendto(socket_fd, packet.Serialize(), PACKET_BUFFER_INITIAL + packet.payload_len, 0, (sockaddr*)&client_addr, sizeof(client_addr));
	if (bytesSent <= 0)
	{
		std::cerr << "Packet send failed with: " << WSAGetLastError()
		<< std::endl;
	}
	else
	{
		std::cout << "Sent packet: " << seq_num << std::endl;
	}
}


void UDPSender::PrintStatus() const
{
	std::cout << "Base: " << base_seq_num << std::endl;
	std::cout << "Next: " << next_seq_num << std::endl;
	std::cout << "Max: " << max_seq_num << std::endl;
	std::cout << "Window: " << window_size << std::endl;
	
}











