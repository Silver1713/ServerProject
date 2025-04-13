#include "Packet.hpp"

#include <WinSock2.h>
#define PACKET_TIMEOUT 1000

Packet::Packet() : seq_num(), data_len(), payload(nullptr), checksum()
{
	
}


Packet::Packet(char* buffer, bool convert_to_host) : seq_num(), data_len(), payload(nullptr), checksum()
{
	Deserialize(buffer, convert_to_host);
}


char* Packet::Serialize(bool convert_to_network) const
{
	// PAYLOAD_CMD (1B) // Session ID(4B) // SEQ_NUM (4B) // DATA LENGTH (4b) // CHECKSUM (4 BYTE) // PAYLOAD (VARIABLE SIZE)
	char* buffer = new char[PACKET_BUFFER_INITIAL + payload_len];
	char* begin = buffer;
	*begin = static_cast<char>(payload_cmd);
	begin += 1;
	uint32_t seq_session_net = convert_to_network ? htonl(session_id) : session_id;
	memcpy(begin, &seq_session_net, 4);
	begin += 4;
	uint32_t seq_num_net = convert_to_network ? htonl(seq_num) : seq_num;
	memcpy(begin, &seq_num_net, 4);
	begin += 4;
	uint32_t data_len_net = convert_to_network ? htonl(data_len) : data_len;
	memcpy(begin, &data_len_net, 4);
	begin += 4;
	uint32_t checksum_net = convert_to_network ? htons(checksum) : checksum;
	memcpy(begin, &checksum_net, 4);

	begin += 4;
	if (payload_len)
		memcpy(begin, payload.get(), payload_len);
	return buffer;
}

Packet& Packet::Deserialize(char* buffer, bool convert_to_host)
{
	// PAYLOAD_CMD (1B) // Session (4B) //  SEQ_NUM (4B) // DATA LENGTH (4b) // CHECKSUM (4 BYTE) // PAYLOAD (VARIABLE SIZE)
	char* begin = buffer;

	payload_cmd = static_cast<UDPCMID>(*begin);
	begin += 1;

	uint32_t session_host = *reinterpret_cast<uint32_t*>(begin);
	session_id = convert_to_host ? ntohl(session_host) : session_host;
	begin += 4;


	uint32_t seq_num_host = *reinterpret_cast<uint32_t*>(begin);
	seq_num = (convert_to_host) ? ntohl(seq_num_host) : seq_num_host;

	begin += 4;

	uint32_t data_len_h = *reinterpret_cast<uint32_t*>(begin);
	data_len = (convert_to_host) ? ntohl(data_len_h) : data_len_h;

	begin += 4;

	uint32_t checksum_h = *reinterpret_cast<uint32_t*>(begin);
	checksum = (convert_to_host) ? ntohl(checksum_h) : checksum_h;
	begin += 4;


	payload.reset();
	payload = std::make_shared<char[]>(payload_len);

	if (payload_len > 0)
	{
		memcpy(payload.get(), begin, payload_len);
	}
	else
	{
		payload = nullptr;
	}

	return  *this;
	

}


void Packet::CalculateChecksum()
{
	//Using the 32 bit Adler Checksum
	uint32_t a = 1, b = 0;
	for (size_t i = 0; i < payload_len; i++) // O(n) complexity [Can we do tabulation here?]
		
	{
		a = (a + static_cast<uint32_t>(payload.get()[i])) % 65521;
		b = (b + a) % 65521;
	}
	checksum = (b << 16) | a;


}


bool Packet::isTimeout()
{
	auto current_time = std::chrono::steady_clock::now();
	auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(current_time - start_time_stamp);
	return duration.count() > 1000;
}

bool Packet::isData() const
{
	return payload_cmd == UDPCMID::PAYLOAD_DATA;
}

bool Packet::isAck() const
{
	return payload_cmd == UDPCMID::PAYLOAD_ACK;
}


