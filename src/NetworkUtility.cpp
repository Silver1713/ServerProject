#include "NetworkUtility.hpp"
#include "WinSock2.h"
char* TCPNetworkUtility::buildDownloadReq(CMID ID, uint32_t ip_addr, uint16_t port, uint32_t len, std::string file_name, bool convert_to_network)
{
	size_t total_size = REQ_DL_BUFFER_SIZE + len + NULL_TERMINATOR_SIZE;

	char* buffer = new char[total_size];
	char* begin = buffer;
	buffer[0] = static_cast<char>(ID);

	begin++;

	uint32_t ip_addr_network = convert_to_network ? htonl(ip_addr) : ip_addr;

	memcpy(begin, &ip_addr_network, sizeof(uint32_t));

	begin += sizeof(uint32_t);

	uint32_t port_network = convert_to_network ? htons(port) : port;
	memcpy(begin, &port_network, sizeof(uint16_t));

	begin += sizeof(uint16_t);


	uint32_t network_len = convert_to_network ? htonl(len) : len;
	memcpy(begin, &network_len, sizeof(uint32_t));

	begin += sizeof(uint32_t);

	memcpy(begin, file_name.c_str(), file_name.size());

	buffer[total_size - 1] = '\0';

	return buffer;


}

char* TCPNetworkUtility::buildDownloadRsp(CMID ID, uint32_t ip_addr, uint16_t port, uint32_t sessionID, uint32_t file_len, bool convert_to_network)
{
	size_t total_size = RSP_DL_BUFFER_SIZE;

	char* buffer = new char[total_size];
	char* begin = buffer;
	buffer[0] = static_cast<char>(ID);

	begin++;

	uint32_t ip_addr_network = convert_to_network ? htonl(ip_addr) : ip_addr;

	memcpy(begin, &ip_addr_network, sizeof(uint32_t));

	begin += sizeof(uint32_t);


	uint32_t port_network = convert_to_network ? htons(port) : port;
	memcpy(begin, &port_network, sizeof(uint16_t));

	begin += sizeof(uint16_t);


	uint32_t session_id_net = convert_to_network ? htonl(sessionID) : sessionID;
	memcpy(begin, &session_id_net, sizeof(uint32_t));

	begin += sizeof(uint32_t);

	uint32_t file_len_net = convert_to_network ? htonl(file_len) : file_len;
	memcpy(begin, &file_len_net, sizeof(uint32_t));

	return buffer;
}

std::pair<size_t, char*> TCPNetworkUtility::buildFileList(CMID ID, uint16_t numFiles, std::vector<FILE_DATA>& file_datas, bool convert_to_network)
{
	std::pair<size_t, char*> result;
	size_t total_size = FILE_LIST_SIZE_INITIAL;
	uint32_t lenFileList = 0;


	for (FILE_DATA const& data : file_datas)
	{
		lenFileList += 4;
		lenFileList += data.len;
	}

	char* buffer = new char[total_size + lenFileList];
	char* begin = buffer;
	buffer[0] = static_cast<char>(ID);

	begin++;

	uint16_t num_file_net = convert_to_network ? htons(numFiles) : numFiles;

	memcpy(begin, &num_file_net, sizeof(uint16_t));

	begin += sizeof(uint16_t);


	uint32_t len_file_net = convert_to_network ? htonl(lenFileList) : lenFileList;
	memcpy(begin, &len_file_net, sizeof(uint32_t));

	begin += sizeof(uint32_t);

	for (FILE_DATA const& data : file_datas)
	{
		uint32_t dataLen = convert_to_network ? ntohl(data.len) : data.len;
		memcpy(begin, &dataLen, sizeof(uint32_t));
		begin += sizeof(uint32_t);
		memcpy(begin, data.filename.c_str(), data.filename.size());
		begin += data.filename.size();
	}
	result.first = total_size + lenFileList;
	result.second = buffer;
	return result;

}

char* TCPNetworkUtility::buildSImpleCMD(CMID ID)
{
	char* buffer = new char[1];

	buffer[0] = static_cast<char>(ID);

	return buffer;
}


void TCPNetworkUtility::ReadDownloadReq(char* buffer, CMID& ID, uint32_t& ip_addr, uint16_t& port, uint32_t& len, std::string& file_name, bool convert_to_host)
{
	char* bufPtr = buffer;

	ID = static_cast<CMID>(*bufPtr);

	bufPtr++;

	uint32_t ip_addr_host = *(reinterpret_cast<uint32_t*>(bufPtr));
	ip_addr_host = convert_to_host ? ntohl(ip_addr_host) : ip_addr_host;
	ip_addr = ip_addr_host;
	bufPtr += sizeof(uint32_t);

	uint16_t port_h = *(reinterpret_cast<uint16_t*>(bufPtr));
	port_h = convert_to_host ? ntohs(port_h) : port_h;
	port = port_h;
	bufPtr += sizeof(uint16_t);
	uint32_t len_host = *(reinterpret_cast<uint32_t*>(bufPtr));
	len = convert_to_host ? ntohl(len_host) : len_host;
	
	bufPtr += sizeof(uint32_t);


	file_name = { bufPtr, len };
	
}

void TCPNetworkUtility::ReadDownloadRsp(char* buffer, CMID& ID, uint32_t& ip_addr, uint16_t& port, uint32_t& sessionID, uint32_t& file_len, bool convert_to_host)
{
	char* bufPtr = buffer;

	ID = static_cast<CMID>(*bufPtr);

	bufPtr++;

	uint32_t ip_addr_host = *(reinterpret_cast<uint32_t*>(bufPtr));
	ip_addr_host = convert_to_host ? ntohl(ip_addr_host) : ip_addr_host;
	ip_addr = ip_addr_host;
	bufPtr += sizeof(uint32_t);

	uint16_t port_h = *(reinterpret_cast<uint16_t*>(bufPtr));
	port_h = convert_to_host ? ntohs(port_h) : port_h;
	port = port_h;
	bufPtr += sizeof(uint16_t);

	uint32_t session_h = *(reinterpret_cast<uint32_t*>(bufPtr));
	sessionID = convert_to_host ? ntohl(session_h) : session_h;

	bufPtr += sizeof(uint32_t);

	uint32_t filelen_h = *(reinterpret_cast<uint32_t*>(bufPtr));
	file_len = convert_to_host ? ntohl(filelen_h) : filelen_h;
}

void TCPNetworkUtility::ReadFileList(char* buffer, CMID& ID, uint16_t& numFiles, uint32_t& lenFiles, std::vector<FILE_DATA>& file_datas, bool convert_to_host)
{
	ID = static_cast<CMID>(*buffer);
	char* bufPtr = buffer+1;

	uint16_t numFiles_h = *(reinterpret_cast<uint16_t*>(bufPtr));
	numFiles = convert_to_host ? ntohs(numFiles_h) : numFiles_h;
	bufPtr += sizeof(uint16_t);

	uint32_t lenFiles_h = *(reinterpret_cast<uint32_t*>(bufPtr));
	lenFiles = convert_to_host ? ntohl(lenFiles_h) : lenFiles_h;
	bufPtr += sizeof(uint32_t);

	for (uint16_t i = 0; i < numFiles; i++)
	{
		FILE_DATA data{};
		uint32_t len_h = *(reinterpret_cast<uint32_t*>(bufPtr));
		data.len = convert_to_host ? ntohl(len_h) : len_h;
		bufPtr += sizeof(uint32_t);

		data.filename = { bufPtr, data.len };
		bufPtr += data.len;

		file_datas.push_back(data);


	}
}





