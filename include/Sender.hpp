#ifndef UDP_SENDER_HPP
#define UDP_SENDER_HPP


#include <cstdint>
#include <string>
#include <unordered_set>
#include <vector>
#include <chrono>
#include <WinSock2.h>
struct Packet;

constexpr int TIMEOUT_SECOND = 5;

enum SRState
{

	PREPARE_PACKET,
	SEND_PACKET,
	CHECK_ACK,
	WAIT_ACK,
	ACK_RECEIVED,
	TIMEOUT,
	SEND_NEXT_PACKET,
	MOVE_WINDOW,
	RESEND_PACKET,
	FINISH,

};

//Selective Repeat -  UDP Sender
class Sender
{
public:
	SOCKET udp_socket;
	std::chrono::steady_clock::time_point lastResponse;
	std::string loaded_file_name;
	uint32_t retransmit_index;


	SRState state{ PREPARE_PACKET };
	uint32_t session_id;
	uint32_t window_size;
	uint32_t base;
	uint32_t next_seq_num;

	std::vector<Packet*> packets;
	Packet* selected_packet{ nullptr };
	std::unordered_set<uint32_t> acked_packets; // Set of acked packets



	ULONG dest_ip_addr; // Destination IP address in network byte order
	uint16_t dest_port; // Destination port in network byte order



	Sender(); // New instance of sender
	Sender(uint32_t session_id, std::string file_name);

	bool create_socket(std::string ip, uint16_t port, bool convert_to_network = true);
	void load_file(std::string file_name);
	void send(SOCKET udpSocket); // FSM for sender


	bool checkTimeout();

};

#endif // !UDP_SENDER_HPP