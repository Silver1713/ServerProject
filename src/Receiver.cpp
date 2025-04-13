#include "Receiver.hpp"

#include <ws2tcpip.h>


void UDPReceiver::ReceiverWorker()
{
	// Main Loop for receiving packets
}

bool UDPReceiver::InitializeSocket()
{
	socket_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (socket_fd == -1)
	{
		return false;
	}
	sockaddr_in local_addr;  // UDP socket address structure
	local_addr.sin_family = AF_INET; // IPv4
	local_addr.sin_port = htons(local_port); // Local port
	local_addr.sin_addr = sender_addr.sin_addr; // Bind to my local address
	u_long mode = 1; // Non-blocking mode
	if (ioctlsocket(socket_fd, FIONBIO, &mode) == SOCKET_ERROR)
	{
		closesocket(socket_fd);
		socket_fd = -1;
		return false;
	}
	
	return true;
}


void UDPReceiver::CloseSocket()
{
	if (socket_fd != -1)
	{
		closesocket(socket_fd);
		socket_fd = -1;
	}
}


void UDPReceiver::Stop()
{
	running = false;
	if (receiver_thread.joinable())
	{
		receiver_thread.join();
	}
	CloseSocket();
}


UDPReceiver::UDPReceiver() : session_id{ 0 }, expected_seq_num{ 0 }, bytes_received{ 0 }
{
}

UDPReceiver::UDPReceiver(uint32_t session_id, std::string file_name, std::string ip, uint16_t local_port) : session_id(session_id), file_name(file_name), expected_seq_num{ 0 }, bytes_received{ 0 }, local_port(local_port)
{
	sender_addr.sin_family = AF_INET;
	sender_addr.sin_port = htons(local_port);
	inet_pton(AF_INET, ip.c_str(), &sender_addr.sin_addr);
	InitializeSocket();
}

UDPReceiver::UDPReceiver(uint32_t session_id, std::string file_name) : session_id(session_id), file_name(file_name), expected_seq_num{ 0 }, bytes_received{ 0 }
{

	InitializeSocket();
}

UDPReceiver::~UDPReceiver()
{
	CloseSocket();
}


void UDPReceiver::ReceiveAsync()
{
	running = true;
	receiver_thread = std::thread(&UDPReceiver::ReceiverWorker, this);
}

bool UDPReceiver::ProcessPacket(const std::vector<uint8_t>& packet, size_t packet_size)
{
	// Process the received packet
	return true;
}

bool UDPReceiver::ValidatePacket(uint32_t recv_session_id, uint32_t seq_num)
{
	// Validate the packet
	return true;
}

void UDPReceiver::SendAck(uint32_t ack_num)
{
	// Send ACK for the received packet
}

void UDPReceiver::HandleInOrderPacket(const uint8_t* data, size_t length, uint32_t seq_num)
{
	// Handle in-order packet
}

void UDPReceiver::HandleOutOfOrderPacket(uint32_t seq_num)
{
	// Handle out-of-order packet
}

bool UDPReceiver::IsActive() const
{
	return running;
}





