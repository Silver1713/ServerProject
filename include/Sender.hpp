#ifndef UDP_SENDER_HPP
#define UDP_SENDER_HPP


#include <cstdint>
#include <string>
#include <unordered_set>
#include <vector>
#include <chrono>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <WinSock2.h>
struct FileData;
struct Packet;

constexpr bool DEBUG_MODE = true;


//Go-Back-N sender
class UDPSender
{
public:
	struct CONNECTIONINFO
	{
		std::string src_ip;
		uint16_t src_port;
		std::string dst_ip;
		uint16_t dst_port;
	} ConnectionInfo;

	UDPSender();
	UDPSender(long long socket_fd, uint32_t session_id, std::string file_name, CONNECTIONINFO info, uint32_t window_size=10);
	UDPSender(long long socket_fd , uint32_t session_id, std::string file_name, uint32_t window_size=10);
	~UDPSender();
	void SendAsync();
	void Stop();
	static std::unordered_map<std::string, UDPSender*> active_senders;
	static std::mutex active_senders_mutex;

	void PrintStatus() const;
private:
	int SENDER_MAX_WAIT_TIMEOUT_MS = DEBUG_MODE ? 20000 : 5000;
	int PKT_TIMEOUT_MS = DEBUG_MODE ? 4000 : 150;

	long long socket_fd; // Socket file descriptor
	sockaddr_in server_addr, client_addr;
	uint16_t server_port; // Server port
	uint16_t client_port; // Client destination port
	uint32_t session_id; // Session ID
	std::string file_name;

	FileData* file_data; // File data to be sent

	std::vector<Packet> send_buffer; // Buffer for sending data
	uint32_t window_size; // Window size
	uint32_t max_seq_num; // Maximum sequence number

	std::atomic<uint32_t> base_seq_num; // Base sequence number - Atomic for thread safety
	std::atomic<uint32_t> next_seq_num; // Next sequence number - Atomic for thread safety

	uint32_t ack_num; // Acknowledgment number

	// Mutexes
	std::mutex send_buf_mutex; // Mutex for send buffer

	std::atomic_bool running{ false }; // Flag to indicate if the sender is running
	std::thread sender_thread; // Thread for sending packets
	std::thread sender_acknowledgement_thread;



	std::atomic<long long> sender_start_time_stamp_epoch{};
	std::atomic<long long> sender_time_since_response_epoch{};
	

	void Intialize();
	void CloseSocket();

	void SenderWorker();
	void AcknowledgementWorker();
	void SendPacket(uint32_t seq_num);
	void SendAck(uint32_t ack_num_n);

	void LoadFile();


	void StartTimer();

	void ResetTimer();

	void ResetTimeout();

	void updateSessionTime();

	bool isTimeout();
	bool isClosing();


	//Debug


	


	
};

#endif // !UDP_SENDER_HPP