#ifndef UDP_RECEIVER_HPP
#define UDP_RECEIVER_HPP


#include <cstdint>
#include <string>
#include <mutex>
#include <vector>
#include <chrono>
#include <thread>
#include <WinSock2.h>
#include <atomic>
#include <fstream>
struct Packet;

constexpr int TIMEOUT_MS = 1000; // Timeout for receiving packets`


// Go-Back-N receiver
class UDPReceiver {
public:
    UDPReceiver();
	UDPReceiver(uint32_t session_id, std::string file_name, std::string ip, uint16_t local_port);
	UDPReceiver(uint32_t session_id, std::string file_name);

    ~UDPReceiver();
    void ReceiveAsync();
    void Stop();

private:
	int socket_fd; // Socket file descriptor - 
	sockaddr_in sender_addr; // Sender address
	uint16_t local_port; // Local port to bind to

    std::mutex recv_buf_mutex;
	std::vector<uint8_t> recv_buffer; // Buffer for receiving data

	uint32_t session_id; // Session ID
    std::string file_name;
    uint32_t expected_seq_num;

    std::thread receiver_thread;
    std::atomic<bool> running;

    std::ofstream output_file;
    size_t bytes_received;

    void ReceiverWorker();
    bool InitializeSocket();
    void CloseSocket();
    bool ProcessPacket(const std::vector<uint8_t>& packet, size_t packet_size);
    bool ValidatePacket(uint32_t recv_session_id, uint32_t seq_num);
    void SendAck(uint32_t ack_num);
    void HandleInOrderPacket(const uint8_t* data, size_t length, uint32_t seq_num);
    void HandleOutOfOrderPacket(uint32_t seq_num);
    bool IsActive() const;
    size_t GetBytesReceived() const;


};

#endif // !UDP_SENDER_HPP