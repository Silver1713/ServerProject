#include "ACK.hpp"

#include <cstring>
#include <WinSock2.h>

#include "CMID.hpp"

ACK::ACK() : seq_num{ 0 }, ack_num{ 0 }, window_size{ 0 }, checksum{ 0 }, timestamp{ 0 }
{
}

ACK::ACK(uint32_t seq_num, uint32_t ack_num, uint32_t window_size, uint16_t checksum, uint32_t timestamp)
{
	this->seq_num = seq_num;
	this->ack_num = ack_num;
	this->window_size = window_size;
	this->checksum = checksum;
	this->timestamp = timestamp;
}


ACK::ACK(char* buffer, bool convert_to_host)
{
	Deserialize(buffer, convert_to_host);
}


char* ACK::Serialize(bool convert_to_network) const
{
	char* buffer = new char[ACK_SIZE];
	buffer[0] = static_cast<char>(UDPCMID::PAYLOAD_ACK);
	char* begin = buffer+1;

	uint32_t seq_num_net = convert_to_network ? htonl(seq_num) : seq_num;
	memcpy(begin, &seq_num_net, 4);
	begin += 4;

	uint32_t ack_num_net = convert_to_network ? htonl(ack_num) : ack_num;
	memcpy(begin, &ack_num_net, 4);
	begin += 4;

	uint32_t window_size_net = convert_to_network ? htonl(window_size) : window_size;
	memcpy(begin, &window_size_net, 4);
	begin += 4;

	uint32_t checksum_net = convert_to_network ? htonl(checksum) : checksum;
	memcpy(begin, &checksum_net, 4);
	begin += 4;

	uint32_t timestamp_net = convert_to_network ? htonl(timestamp) : timestamp;
	memcpy(begin, &timestamp_net, 4);

	return buffer;

}

ACK& ACK::Deserialize(char* buffer, bool convert_to_host)
{
	char* begin = buffer;

	uint32_t seq_num_net = *(reinterpret_cast<uint32_t*>(begin));
	seq_num = convert_to_host ? ntohl(seq_num_net) : seq_num_net;
	begin += 4;

	uint32_t ack_num_net = *(reinterpret_cast<uint32_t*>(begin));
	ack_num = convert_to_host ? ntohl(ack_num_net) : ack_num_net;
	begin += 4;

	uint32_t window_size_net = *(reinterpret_cast<uint32_t*>(begin));
	window_size = convert_to_host ? ntohl(window_size_net) : window_size_net;
	begin += 4;

	uint32_t checksum_net = *(reinterpret_cast<uint32_t*>(begin));
	checksum = convert_to_host ? ntohl(checksum_net) : checksum_net;
	begin += 4;

	uint32_t timestamp_net = *(reinterpret_cast<uint32_t*>(begin));
	timestamp = convert_to_host ? ntohl(timestamp_net) : timestamp_net;

	return *this;
}


