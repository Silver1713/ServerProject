#ifndef UDP_RECEIVER_HPP
#define UDP_RECEIVER_HPP


#include <cstdint>
#include <string>
#include <unordered_set>
#include <vector>
#include <chrono>
#include <WinSock2.h>
struct Packet;

constexpr int TIMEOUT_MS = 5000;

enum SR_RECEIVER
{

	AWAIT_PACKET,
	RECEIVE_PACKET,
	BUFFER_PACKET,
	MOVE_WINDOW,
	TIMEOUT_REC,
	FINISH

};

//Selective Repeat -  UDP Sender
class Receiver
{
public:
	SOCKET udp_socket;
	std::chrono::steady_clock::time_point lastResponse;
	size_t fileSize;
	uint32_t retransmit_index;


	SR_RECEIVER state{ AWAIT_PACKET };
	uint32_t session_id;
	uint32_t window_size;
	uint32_t base;
	uint32_t next_seq_num;

	std::vector<Packet*> received_packets;

	Packet* selected_packet{ nullptr };
	std::unordered_set<uint32_t> acked_packets; // Set of acked packets



	ULONG dest_ip_addr; // Destination IP address in network byte order
	uint16_t dest_port; // Destination port in network byte order



	Receiver(); // New instance of sender
	Receiver(uint32_t sessionID, size_t file_size);
	

	bool create_socket(uint16_t port, bool convert_to_network = true);

	void receive(SOCKET udpSocket); // FSM for receiver


	bool isTimeout(); // Check for connection timeout

	

};

#endif // !UDP_SENDER_HPP