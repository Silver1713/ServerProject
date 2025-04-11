#ifndef ACKNOWLEDGEMENT_HPP
#define ACKNOWLEDGEMENT_HPP

#define ACK_SIZE 21
#include <cstdint>

//UDP ACK for Selective Repeat
struct ACK
{
	
	uint32_t seq_num;
	uint32_t ack_num;
	uint32_t window_size;
	uint32_t checksum;
	uint32_t timestamp;

	ACK();
	ACK(char* buffer, bool convert_to_host);
	ACK(uint32_t seq_num, uint32_t ack_num, uint32_t window_size, uint16_t checksum, uint32_t timestamp);

	char* Serialize(bool convert_to_network) const;
	ACK& Deserialize(char* buffer, bool convert_to_host);
};

#endif // !ACKNOWLEDGEMENT_HPP