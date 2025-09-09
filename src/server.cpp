#include <iostream>
#include <thread>
#include <vector>
#include <map>
#include <set>
#include <mutex>
#include <atomic>
#include <fstream>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstring>
#include <queue>
#include <ctime>
#include <chrono>
#include "../lib/txt_writer.h"
#include "../lib/cxx_utils.hpp"

/* Packet types: 
 * 0: welcome S2C
 * 1: send chat message C2S
 * 2: chat log data S2C
 * 3: begin sending chat log S2C
 * 4: end sending chat log S2C
 * 5: client disconnect S2C & C2S
 * 6: client  connect (with nick) C2S
 * 7: deny nick (reason in payload) S2C
 * 8: accept nick (includes nick, as callback) S2C
 * 9: keep alive C2S
 */

#define PORT 12345
const char* HISTORY_FILE = "./server/history.txt";
std::mutex temp_mutex, working_mutex;
std::vector<std::string> tempHistory;
std::vector<std::string> workingHistory;

#pragma pack(push, 1)
struct Packet {
    uint8_t type;
    uint32_t length;
    char payload[256];
};
#pragma pack(pop)

std::mutex clients_mutex;
std::map<int, int> client_ids;
std::set<int> client_sockets;
std::queue<int> available_ids;
std::atomic<int> next_id{1};
std::vector<std::string> usedNicks;

uint_fast8_t currentDay = 0;
time_t tt = 0;

// Send a string to a client as a Packet
void sendTextPacket(int client_sock, const std::string& msg, uint8_t type = 2) {
    Packet packet{};
    packet.type = type;
    packet.length = std::min((int)msg.size(), 256);
    strncpy(packet.payload, msg.c_str(), sizeof(packet.payload));

    ssize_t sent = send(client_sock, &packet, sizeof(packet), 0);
    if (sent != sizeof(packet)) {
        std::cout << Timekeeping::getCurrentTimeHMS() << " Failed to send to client " << client_sock << ", removing\n";
        std::lock_guard<std::mutex> lock(clients_mutex);
        close(client_sock);
        client_sockets.erase(client_sock);
        available_ids.push(client_ids[client_sock]);
        client_ids.erase(client_sock);
    }
}

// Broadcast file content to all clients
void broadcastHistory() {
    {
        // Copy temp → working under lock
        std::lock_guard<std::mutex> lock(temp_mutex);
        std::lock_guard<std::mutex> lock2(working_mutex);
        workingHistory = tempHistory; 
    }

    std::lock_guard<std::mutex> lock(clients_mutex);

    Packet begin{};
    begin.type = 3; begin.length = 0;

    std::set<int> toRemove;
    for (int client : client_sockets) {
        if (send(client, &begin, sizeof(begin), 0) != sizeof(begin)) {
            toRemove.insert(client);
        }
    }

    // Iterate over working snapshot
    std::lock_guard<std::mutex> lockW(working_mutex);
    for (const std::string &line : workingHistory) {
        if (line.empty()) continue;

        Packet packet{};
        packet.type = 2;
        packet.length = std::min((int)line.size(), 256);
        std::memcpy(packet.payload, line.c_str(), packet.length);

        for (int client : client_sockets) {
            if (send(client, &packet, sizeof(packet), 0) != sizeof(packet)) {
                toRemove.insert(client);
            }
        }
    }

    Packet end{};
    end.type = 4; end.length = 0;
    for (int client : client_sockets) {
        if (send(client, &end, sizeof(end), 0) != sizeof(end)) {
            toRemove.insert(client);
        }
    }

    // Remove dead clients
    for (int client : toRemove) {
        close(client);
        client_sockets.erase(client);
        available_ids.push(client_ids[client]);
        client_ids.erase(client);
        std::cout << Timekeeping::getCurrentTimeHMS()
                  << " Removed dead client socket " << client << "\n";
    }
}

void handle_client(int client_sock) {
    int client_id;
	{
	    std::lock_guard<std::mutex> lock(clients_mutex);
	    if (!available_ids.empty()) {
	        client_id = available_ids.front();
	        available_ids.pop();
	    } else {
	        client_id = next_id++;
	    }
	    client_ids[client_sock] = client_id;
	    client_sockets.insert(client_sock);
	}

    std::cout << Timekeeping::getCurrentTimeHMS() << " New client connected with ID: " << client_id << "\n";

    // Send welcome message
    Packet welcome{};
    welcome.type = 0;
    std::string msg = "Welcome " + std::to_string(client_id);
    welcome.length = msg.size();
    strncpy(welcome.payload, msg.c_str(), sizeof(welcome.payload));
    send(client_sock, &welcome, sizeof(welcome), 0);

    std::this_thread::sleep_for((std::chrono::milliseconds)200);
    
    // Send current history
    broadcastHistory();

    Packet packet;
    while (true) {
	    ssize_t n = recv(client_sock, &packet, sizeof(packet), 0);
	    if (n <= 0) {
	        break; // Client disconnected or error
	    }
        
		if (packet.type == 5) break; // Disconnect packet
        if (packet.type == 1) { // Chat message
            std::string received(packet.payload, packet.length);
            std::cout << Timekeeping::getCurrentTimeHMS() 
                      << " [Client " << client_id << "] Sent: " << received << "\n";

            // Append to temp history (thread-safe)
            {
                std::lock_guard<std::mutex> lock(temp_mutex);
                tempHistory.push_back(received);
            }
            // Also append to file for persistence
            writeLineToFile(HISTORY_FILE, received.c_str());
        
            // Broadcast updated history
            broadcastHistory();
        }
    }

    // Cleanup
    close(client_sock);
    {
	    std::lock_guard<std::mutex> lock(clients_mutex);
	    client_sockets.erase(client_sock);
	    auto it = client_ids.find(client_sock);
	    if (it != client_ids.end()) {
	        available_ids.push(it->second);  // Recycle ID
	        client_ids.erase(it);
	    }
	}

    std::cout << Timekeeping::getCurrentTimeHMS() << " Client " << client_id << " disconnected.\n";
}

int main() {
    int server_fd;
    sockaddr_in address{};
    socklen_t addrlen = sizeof(address);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);
    
    int opt = 1;
	setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
	
    bind(server_fd, (sockaddr*)&address, sizeof(address));
    listen(server_fd, 5);

    std::cout << Timekeeping::getCurrentTimeHMS() << " Server listening on port " << PORT << "...\n";

    tt = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    tm utc_time = *gmtime(&tt);
    currentDay = utc_time.tm_mday;

    while (true) {
        int client_sock = accept(server_fd, (sockaddr*)&address, &addrlen);
	    if (client_sock < 0) {
	        std::cout << Timekeeping::getCurrentTimeHMS() << " [Server] Failed to accept connection: " << strerror(errno) << "\n";
	        continue;
	    }
	    std::thread(handle_client, client_sock).detach();
        
        // check new day
        tt = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        utc_time = *gmtime(&tt);

        if (utc_time.tm_mday != currentDay) {
            currentDay = utc_time.tm_mday;
            std::string message = "[SERVER] A new day begins (" + std::to_string(currentDay) + ")";
            writeLineToFile(HISTORY_FILE, message.c_str());
            broadcastHistory();
        }
	}
	
    close(server_fd);
    return 0;
}
