#pragma once
#include <cstdint>
#include <string>
#include <vector>

#include "CMID.hpp"

#define REQ_DL_BUFFER_SIZE 11
#define RSP_DL_BUFFER_SIZE 15
#define FILE_LIST_SIZE_INITIAL 7
#define NULL_TERMINATOR_SIZE 1
struct FILE_DATA
{
	uint32_t len;
	std::string filename;
};

struct TCPNetworkUtility
{
	static char* buildDownloadReq(CMID ID, uint32_t ip_addr, uint16_t port, uint32_t len, std::string file_name, bool convert_to_network = false);


	static char* buildDownloadRsp(CMID ID, uint32_t ip_addr, uint16_t port, uint32_t sessionID, uint32_t file_len, bool convert_to_network = false);


	static std::pair<size_t, char*> buildFileList(CMID ID, uint16_t numFiles, std::vector<FILE_DATA>& file_datas, bool convert_to_network=false);
	


	static char* buildSImpleCMD(CMID ID);


	static void ReadDownloadReq(char* buffer, CMID& ID, uint32_t& ip_addr, uint16_t& port, uint32_t& len, std::string& file_name, bool convert_to_host = false);

	static void ReadDownloadRsp(char* buffer, CMID& ID, uint32_t& ip_addr, uint16_t& port, uint32_t& sessionID, uint32_t& file_len, bool convert_to_host = false);
	static void ReadFileList(char* buffer, CMID& ID, uint16_t& numFiles, uint32_t& lenFiles, std::vector<FILE_DATA>& file_datas, bool convert_to_host = false);
};