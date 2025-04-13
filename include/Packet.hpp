#ifndef PACKET_HPP 
#define PACKET_HPP
#include <cstdint>
#include <memory>
#include <chrono>
#include "CMID.hpp"


constexpr size_t PACKET_BUFFER_INITIAL = 17;
constexpr size_t PACKET_SIZE = 1400;
constexpr size_t PAYLOAD_SIZE = PACKET_SIZE - PACKET_BUFFER_INITIAL;


struct Packet
{

	std::chrono::steady_clock::time_point start_time_stamp{}; //time stamp when packet was sent or received, this will not be serialized into the data buffer
	bool is_acknowledged{ false };
	// Field Serialization
	uint32_t payload_len{};
	UDPCMID payload_cmd;

	uint32_t session_id;
	uint32_t seq_num;
	uint32_t data_len;
	
	uint32_t checksum;
	std::shared_ptr<char[]> payload;

	Packet();
	Packet(char* buffer, bool convert_to_host=true);

	char* Serialize(bool convert_to_network=true) const;
	Packet& Deserialize(char* buffer , bool convert_to_host=true);

	void CalculateChecksum();

	bool isTimeout();

	bool isData() const; // is the packet a data packet
	bool isAck() const;  // is the packet an ack packet


	
};
#endif