/* Start Header
*****************************************************************/
/*!
\file server.cpp
\author Yeo Jia Hao (jiahao.yeo\@digipen.edu)
\par CSD2161 - Computer Networks
\par Assignment 2
\date 22 Feb 2025
\brief
This file contain the implementation of a simple TCP/IP server application that
allows for echo messages and listing of connected users.

Furthermore, it contain threading to allow for multiple clients to connect to the
server and communicate with each other.


Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/
/* End Header
*******************************************************************/

/*******************************************************************************
 * A simple TCP/IP server application
 ******************************************************************************/

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include "Windows.h"		// Entire Win32 API...
#include "winsock2.h"		// ...or Winsock alone
#include "ws2tcpip.h"		// getaddrinfo()


 // Tell the Visual Studio linker to include the following library in linking.
 // Alternatively, we could add this file to the linker command-line parameters,
 // but including it in the source code simplifies the configuration.
#pragma comment(lib, "ws2_32.lib")
#include "CMID.hpp"
#include <cstdio>
#include <functional>
#include <iostream>			   // cout, cerr
#include <string>			     // string
#include <chrono>

#include "FileManager.hpp"
#include "NetworkUtility.hpp"
#include "safe_cout.hpp"
#include "Sender.hpp"
#include "taskqueue.h"

#define WINSOCK_VERSION     2
#define WINSOCK_SUBVERSION  2
#define MAX_STR_LEN         1000
#define MAX_BUFFER_SIZE     5000
#define RETURN_CODE_1       1
#define RETURN_CODE_2       2
#define RETURN_CODE_3       3
#define RETURN_CODE_4       4


#define ECHO_BUFFER_SIZE 1 + 4 + 2 + 4
#define NULL_TERMINATOR 1

std::unordered_map<std::string, SOCKET> userSOCKETs; //!< Map of user IP address and port to their respective socket

SOCKET UDPSocket; //!< The UDP socket for the server
std::vector<ULONG> sessions;

std::mutex session_mutex;
static SafeCout safeCout{};

/*!*********************************************************
@brief
Gracefully disconnects the client from the server.
This function sends a shutdown signal to the client to signal the client to
gracefully disconnect from the server. This function also waits for the client
to respond before closing the connection.

@param clientSocket The client socket to disconnect
@param skipWait Flag to skip waiting for the client to respond
@return True if the client is disconnected successfully
***********************************************************/
bool graceful_disconnect(SOCKET clientSocket, bool skipWait = false)
{
	int status = shutdown(clientSocket, SD_SEND);

	if (status == SOCKET_ERROR)
	{
		std::cerr << "shutdown() failed." << std::endl;
		return false;
	}

	char* garbage_buffer = new char[MAX_STR_LEN] {};


	if (!skipWait)
	{
		int bytesReceived = recv(clientSocket, garbage_buffer, MAX_STR_LEN, 0);

		garbage_buffer += bytesReceived;
		while (bytesReceived > 0)
		{
			bytesReceived = recv(clientSocket, garbage_buffer, MAX_STR_LEN, 0);
		}

		if (bytesReceived == SOCKET_ERROR)
		{
			std::cerr << "recv() failed." << std::endl;
			std::cerr << WSAGetLastError() << std::endl;
			return false;
		}
	}


	delete[] garbage_buffer;

	std::cout << "Client gracefully disconnected." << std::endl;

	return true;
}

/*!*********************************************************
@brief Disconnects the socket.
This function disconnects the socket and closes the connection.
@param listenerSocket The socket to disconnect
@return True if the socket is disconnected successfully else false.
***********************************************************/
bool disconnectSocket(SOCKET& listenerSocket)
{

	int status = shutdown(listenerSocket, SD_BOTH);
	if (status == SOCKET_ERROR)
	{
		std::cerr << "shutdown() failed." << std::endl;
		return false;
	}
	closesocket(listenerSocket);
	listenerSocket = INVALID_SOCKET;

	return  true;
}

/*!*********************************************************
@brief This the main worker function for the secondary threads.
This function is a worker function used by threads to process the data
received from clients. It allow handling of different commands such as
echo, list users and quit. This function also handles the disconnection
of clients from the server. Furthermore, this function also handles
the response to the client and the forwarding of messages to the
respective clients.
@param clientSocket The client socket to process
@return True if the server is still running
***********************************************************/
bool work(SOCKET clientSocket)
{
	

	char bufferData[MAX_BUFFER_SIZE]{};

	bool serverRunning = true;

	u_long enableNonBlocking = 1;
	ioctlsocket(clientSocket, FIONBIO, &enableNonBlocking);

	while (serverRunning)
	{
		bool echoError = false;
		int bytesReceived = recv(clientSocket, bufferData, MAX_BUFFER_SIZE, 0);

		if (bytesReceived == SOCKET_ERROR)
		{
			if (WSAGetLastError() == WSAEWOULDBLOCK)
			{
				using namespace std::chrono_literals;
				std::this_thread::sleep_for(10ms);
				continue;
			}

			safeCout << "recv() failed." << std::endl;
			safeCout << WSAGetLastError() << std::endl;

			break;
		}




		if (!bytesReceived)
		{

			safeCout << "Graceful shutdown\n";
			char clientIPAddr[MAX_STR_LEN];
			char clientPort[MAX_STR_LEN];

			sockaddr client_in{};
			int client_in_size = sizeof(client_in);
			getpeername(clientSocket, &client_in, &client_in_size);
			getnameinfo(&client_in, client_in_size, clientIPAddr, sizeof(clientIPAddr), clientPort, sizeof(clientPort), NI_NUMERICHOST);

			std::string fullId = std::string(clientIPAddr) + ':' + std::string(clientPort);

			userSOCKETs.erase(fullId);

			break;
		}

		char commandID = bufferData[0];

		if (commandID == static_cast<char>(CMID::REQ_QUIT))
		{
			CMID commandID = CMID::REQ_QUIT;
			char* data = TCPNetworkUtility::buildSImpleCMD(commandID);
			send(clientSocket, data, 1, 0);
			delete[] data;
			break;
		}

		else if (commandID == static_cast<char>(CMID::REQ_LISTFILES))
		{
			CMID cmdID = CMID::RSP_LISTFILES;
			std::vector<FILE_DATA> fileList{};

			FileManager::UpdateFiles();
			auto& files = FileManager::files;

			for (std::pair<std::string const, FileData>& f : files)
			{
				FILE_DATA file{};
				file.filename = f.second.file_name;
				file.len = file.filename.size();
				fileList.push_back(file);
			}
			std::pair r = TCPNetworkUtility::buildFileList(cmdID, static_cast<uint16_t>(fileList.size()), fileList, true);
			send(clientSocket, r.second, static_cast<int>(r.first), 0);
			delete[] r.second;
		}
		else if (commandID == static_cast<char>(CMID::REQ_DOWNLOAD))
		{
			CMID ID;
			uint32_t ip_addr;
			uint16_t rec_port;
			uint32_t rec_len;
			std::string name;

			TCPNetworkUtility::ReadDownloadReq(bufferData, ID, ip_addr, rec_port, rec_len, name, true);

			FileData& data = FileManager::files[name];
			data.ReadFile();

			ULONG sessionID = sessions.size();
			{
				std::lock_guard<std::mutex> lock_session{ session_mutex };
				sessions.push_back(sessionID);
			}



			sockaddr clientAddress{};
			SecureZeroMemory(&clientAddress, sizeof(clientAddress));
			int clientAddressSize = sizeof(clientAddress);

			/* PRINT SERVER IP ADDRESS AND PORT NUMBER */
			char serverIPAddr[MAX_STR_LEN];
			char serverPort[MAX_STR_LEN];
			unsigned int server_addr_source;
			unsigned short server_port_source;

			Sender sender{ sessionID, name };
			sender.dest_ip_addr = ip_addr;
			sender.dest_port = rec_port;


			sender.create_socket(serverIPAddr,9004, true); // 9004
			int n = getsockname(sender.udp_socket, &clientAddress, &clientAddressSize);
			int a  = getnameinfo(&clientAddress, clientAddressSize, serverIPAddr, sizeof(serverIPAddr), serverPort, sizeof(serverPort), NI_NUMERICHOST);

			inet_pton(AF_INET, serverIPAddr, &server_addr_source);
			server_port_source = unsigned short(std::stoul(serverPort));
			uint32_t serveraddrsrc = ntohl(server_addr_source);
			uint16_t server_addr_port = server_port_source;

			uint32_t session_id_big_endian = ntohl(sessionID);
			uint32_t file_length_big_endian = ntohl(data.data_size);

			char* sendBuffer = TCPNetworkUtility::buildDownloadRsp(CMID::RSP_DOWNLOAD, serveraddrsrc, server_addr_port, sessionID, data.data_size, true);


			send(clientSocket, sendBuffer, RSP_DL_BUFFER_SIZE, 0);
			delete[] sendBuffer;


			sender.send(sender.udp_socket);


		}
		/*if (commandID == REQ_QUIT)
		{
			char commandID = REQ_QUIT;
			char* data = buildData(commandID, 0, nullptr, true);
			send(clientSocket, data, 1, 0);
			delete[] data;
			break;

		}
		else if (commandID == REQ_ECHO)
		{
			char commandID{};
			std::string IPAddrString{};
			uint16_t dst_port{};
			uint32_t textLen{};
			std::string text_data;


			extractEchoData(bufferData, commandID, IPAddrString, dst_port, textLen, text_data, true);


			sockaddr client_in{};
			int client_in_size = sizeof(client_in);
			getpeername(clientSocket, &client_in, &client_in_size);
			char clientIPAddr[MAX_STR_LEN];
			char clientPort[MAX_STR_LEN];
			getnameinfo(&client_in, client_in_size, clientIPAddr, sizeof(clientIPAddr), clientPort, sizeof(clientPort), NI_NUMERICHOST);

			std::string srcfullId = std::string(clientIPAddr) + ':' + std::string(clientPort);

			std::string dstfullId = IPAddrString + ':' + std::to_string(dst_port);

			uint16_t srcPort = ntohs(reinterpret_cast<sockaddr_in*>(&client_in)->sin_port);

			char* data = buildEchoData(REQ_ECHO, clientIPAddr, srcPort, textLen, text_data.c_str(), true);



			auto const& findResult = userSOCKETs.find(dstfullId);

			if (findResult != userSOCKETs.end())
			{
				send(findResult->second, data, textLen + ECHO_BUFFER_SIZE, 0);
				std::cout << "==========RECV START==========" << std::endl;
				std::cout << srcfullId << '\n' << text_data << std::endl;
				std::cout << "==========RECV END==========" << std::endl;
			}
			else
			{
				bufferData[0] = ECHO_ERROR;
				echoError = true;
			}

			delete[] data;
		}
		else if (commandID == RSP_ECHO)
		{
			char commandID{};
			std::string IPAddrString{};
			uint16_t dst_port{};
			uint32_t textLen{};
			std::string text_data;


			extractEchoData(bufferData, commandID, IPAddrString, dst_port, textLen, text_data, true);


			sockaddr client_in{};
			int client_in_size = sizeof(client_in);
			getpeername(clientSocket, &client_in, &client_in_size);
			char clientIPAddr[MAX_STR_LEN];
			char clientPort[MAX_STR_LEN];
			getnameinfo(&client_in, client_in_size, clientIPAddr, sizeof(clientIPAddr), clientPort, sizeof(clientPort), NI_NUMERICHOST);

			std::string srcfullId = std::string(clientIPAddr) + ':' + std::string(clientPort);

			std::string dstfullId = IPAddrString + ':' + std::to_string(dst_port);

			uint16_t srcPort = ntohs(reinterpret_cast<sockaddr_in*>(&client_in)->sin_port);

			char* data = buildEchoData(commandID, clientIPAddr, srcPort, textLen, text_data.c_str(), true);



			auto const& findResult = userSOCKETs.find(dstfullId);

			if (findResult != userSOCKETs.end())
			{
				send(findResult->second, data, textLen + ECHO_BUFFER_SIZE, 0);
			}
			else
			{
				bufferData[0] = ECHO_ERROR;
				echoError = true;
			}

			delete[] data;
		}
		else if (commandID == CMDID::REQ_LISTUSERS)
		{
			char commandId = CMDID::RSP_LISTUSERS;

			std::vector<User> userList;

			for (auto const& [key, value] : userSOCKETs)
			{
				sockaddr client_in{};
				int client_in_size = sizeof(client_in);
				getpeername(value, &client_in, &client_in_size);
				char clientIPAddr[MAX_STR_LEN];
				char clientPort[MAX_STR_LEN];
				getnameinfo(&client_in, client_in_size, clientIPAddr, sizeof(clientIPAddr), clientPort, sizeof(clientPort), NI_NUMERICHOST);

				User user{};
				user.ip_addr_string = std::string(clientIPAddr);
				user.port = ntohs(reinterpret_cast<sockaddr_in*>(&client_in)->sin_port);

				userList.push_back(user);
			}

			char* data = buildUserListData(commandId, static_cast<short>(userList.size()), userList, true);

			send(clientSocket, data, 1 + 2 + 6 * static_cast<short>(userList.size()), 0);


		}
		else
		{
			std::cout << "Invalid command received\n";
			char commandID = REQ_QUIT;
			char* data = buildData(commandID, 0, nullptr, true);
			send(clientSocket, data, 1, 0);
			delete[] data;
		}*/



		if (echoError)
		{
			send(clientSocket, bufferData, 1, 0);
		}


	}

	sockaddr address{};
	int addressSize = sizeof(address);

	SecureZeroMemory(&address, addressSize);

	int status = getpeername(clientSocket, &address, &addressSize);

	if (status == SOCKET_ERROR)
	{
		std::cerr << "getpeername() failed." << std::endl;
		std::cerr << WSAGetLastError() << std::endl;
		return serverRunning;
	}
	char ipAddrStr[MAX_STR_LEN]{};

	inet_ntop(AF_INET, &(reinterpret_cast<sockaddr_in*>(&address)->sin_addr), ipAddrStr, INET_ADDRSTRLEN);

	std::string fullId = std::string(ipAddrStr) + ':' + std::to_string(ntohs(reinterpret_cast<sockaddr_in*>(&address)->sin_port));


	userSOCKETs.erase(fullId);


	shutdown(clientSocket, SD_BOTH);

	closesocket(clientSocket);


	return serverRunning;
}
int main()
{
#pragma region TEST
#define TESTER
#ifdef TESTER
	//----------------------TEST----------------------
	FileManager::SetDirectory("data");
	FileManager::UpdateFiles();

	//Sender sender{ 0, "SampleA.txt" };
	//----------------------TEST----------------------
#endif
#pragma  endregion
	std::string TCPPortNumber;
	std::string UDPPortNumber;
	std::string ServerFileContainer;
	std::cout << "Server TCP Port Number: ";
	std::getline(std::cin, TCPPortNumber);
	std::cout << "Server UDP Port Number: ";
	std::getline(std::cin, UDPPortNumber);
	std::cout << "Path: ";
	std::getline(std::cin, ServerFileContainer);

	std::string portString = TCPPortNumber;
	std::string udpPortString = UDPPortNumber;
	std::string pathString = ServerFileContainer;


	// -------------------------------------------------------------------------
	// Start up Winsock, asking for version 2.2.
	//
	// WSAStartup()
	// -------------------------------------------------------------------------

	// This object holds the information about the version of Winsock that we
	// are using, which is not necessarily the version that we requested.
	WSADATA wsaData{};

	// Initialize Winsock. You must call WSACleanup when you are finished.
	// As this function uses a reference counter, for each call to WSAStartup,
	// you must call WSACleanup or suffer memory issues.
	int errorCode = WSAStartup(MAKEWORD(WINSOCK_VERSION, WINSOCK_SUBVERSION), &wsaData);
	if (NO_ERROR != errorCode)
	{
		std::cerr << "WSAStartup() failed." << std::endl;
		return errorCode;
	}

	// -------------------------------------------------------------------------
	// Resolve own host name into IP addresses (in a singly-linked list).
	//
	// getaddrinfo()
	// -------------------------------------------------------------------------
	// Object hints indicates which protocols to use to fill in the info.
	addrinfo hints{};
	addrinfo UDPHints{};

	SecureZeroMemory(&hints, sizeof(hints));
	SecureZeroMemory(&UDPHints, sizeof(UDPHints));
	hints.ai_family = AF_INET;			// IPv4
	// For UDP use SOCK_DGRAM instead of SOCK_STREAM.
	hints.ai_socktype = SOCK_STREAM;	// Reliable delivery
	// Could be 0 for autodetect, but reliable delivery over IPv4 is always TCP.
	hints.ai_protocol = IPPROTO_TCP;	// TCP
	// Create a passive socket that is suitable for bind() and listen().
	hints.ai_flags = AI_PASSIVE;

	UDPHints.ai_family = AF_INET;
	UDPHints.ai_socktype = SOCK_DGRAM;
	UDPHints.ai_protocol = IPPROTO_UDP;

	char host[MAX_STR_LEN];
	gethostname(host, MAX_STR_LEN);

	addrinfo* info = nullptr;
	addrinfo* UDPInfo = nullptr;
	errorCode = getaddrinfo(host, portString.c_str(), &hints, &info);
	if ((NO_ERROR != errorCode) || (nullptr == info))
	{
		std::cerr << "getaddrinfo() failed." << std::endl;
		WSACleanup();
		return errorCode;
	}

	errorCode = getaddrinfo(host, udpPortString.c_str(), &UDPHints, &UDPInfo);
	if ((NO_ERROR != errorCode) || (nullptr == UDPInfo))
	{
		std::cerr << "getaddrinfo() failed." << std::endl;
		WSACleanup();
		return errorCode;
	}


	/* PRINT SERVER IP ADDRESS AND PORT NUMBER */
	char serverIPAddr[MAX_STR_LEN];
	struct sockaddr_in* serverAddress = reinterpret_cast<struct sockaddr_in*> (info->ai_addr);
	inet_ntop(AF_INET, &(serverAddress->sin_addr), serverIPAddr, INET_ADDRSTRLEN);

	getnameinfo(info->ai_addr, static_cast <socklen_t> (info->ai_addrlen), serverIPAddr, sizeof(serverIPAddr), nullptr, 0, NI_NUMERICHOST);

	std::cout << std::endl;
	std::cout << "Server IP Address: " << serverIPAddr << std::endl;
	std::cout << "Server Port Number: " << portString << std::endl;
	std::cout << "Server UDP Port Number: " << udpPortString << std::endl;
	std::cout << "Server File Directory: " << pathString << std::endl;

	// -------------------------------------------------------------------------
	// Create a socket and bind it to own network interface controller.
	//
	// socket()
	// bind()
	// -------------------------------------------------------------------------

	SOCKET listenerSocket = socket(
		hints.ai_family,
		hints.ai_socktype,
		hints.ai_protocol);

	// UDPSocket = socket(UDPHints.ai_family, UDPHints.ai_socktype, UDPHints.ai_protocol);
	if (INVALID_SOCKET == listenerSocket)
	{
		std::cerr << "socket() failed." << std::endl;
		freeaddrinfo(info);
		WSACleanup();
		return RETURN_CODE_1;
	}

	

	errorCode = bind(
		listenerSocket,
		info->ai_addr,
		static_cast<int>(info->ai_addrlen));


	if (NO_ERROR != errorCode)
	{
		std::cerr << "bind() failed." << std::endl;
		closesocket(listenerSocket);
		listenerSocket = INVALID_SOCKET;
	}



	if (INVALID_SOCKET == listenerSocket)
	{
		std::cerr << "bind() failed." << std::endl;
		WSACleanup();
		return RETURN_CODE_2;
	}

	

	// -------------------------------------------------------------------------
	// Set a socket in a listening mode and accept 1 incoming client.
	//
	// listen()
	// accept()
	// -------------------------------------------------------------------------
	errorCode = listen(listenerSocket, SOMAXCONN);
	if (errorCode != NO_ERROR)
	{
		std::cerr << "listen() failed." << std::endl;
		closesocket(listenerSocket);
		WSACleanup();
		return RETURN_CODE_3;
	}

	auto OnDisconnect = [&]() -> void
	{
		disconnectSocket(listenerSocket);
	};

	TaskQueue<SOCKET, decltype(work), decltype(OnDisconnect)> taskQueue{ 10,20, work, OnDisconnect };

	while (listenerSocket != INVALID_SOCKET) // While socket is valid
	{
		sockaddr client{}; // Client address

		SecureZeroMemory(&client, sizeof(client)); // Zero out the memory

		int addrSize = sizeof(client); // Get address size

		SOCKET socket = accept(listenerSocket, &client, &addrSize); // Accept the connection

		if (socket == INVALID_SOCKET)
		{
			break;
		}
		// Get the named IP address and port number of the client
		char clientIPAddr[MAX_STR_LEN];
		char clientPort[MAX_STR_LEN];

		getpeername(socket, &client, &addrSize);
		getnameinfo(&client, addrSize, clientIPAddr, sizeof(clientIPAddr), clientPort, sizeof(clientPort), NI_NUMERICHOST);


		safeCout << '\n';
		safeCout << "Client IP Address: " << clientIPAddr << std::endl;
		safeCout << "Client Port Number: " << clientPort << std::endl;

		// Get the binary representation of the client IP address
		uint32_t clientIPAddress{};
		uint16_t clientPortNumber{};

		inet_pton(AF_INET, clientIPAddr, &clientIPAddress);

		clientPortNumber = static_cast<uint16_t>(std::stoul(clientPort));

		std::string fullId = std::string(clientIPAddr) + ':' + std::string(clientPort);



		userSOCKETs[fullId] = socket; // Add the client to the map

		taskQueue.produce(socket); // Produce the socket to the task queue using the producer-consumer pattern

	}


	// Shutdown the listener socket


	shutdown(listenerSocket, SD_BOTH);

	// Close the listener socket
	closesocket(listenerSocket);


	// -------------------------------------------------------------------------
	// Clean-up after Winsock.
	//
	// WSACleanup()
	// -------------------------------------------------------------------------

	WSACleanup();
}
