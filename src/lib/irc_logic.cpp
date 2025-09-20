#include "irc_logic.hpp"

std::string ircWindowFile;
std::string ircUserPrompt;
std::string ircNickname;
std::string ircText;
std::string ircConnectionTempFile;
bool ircConnected;
bool ircOpened;

namespace IRCLogic {
	std::string serverHandshakeResponse;
	std::string lastNickname;

	namespace {
		void safeWriteToIrcWindow(const std::string& content) {
			std::lock_guard<std::mutex> lock(file_mutex);
			std::ofstream out(ircWindowFile, std::ios::trunc);
			if (out) {
				out << content;
				fileLog("[Client] Updated irc window file\n");
			} else {
				fileLog("[Client] Failed to write to %s\n", ircWindowFile);
			}
		}

		void receiveLoop() {
		    std::string buffer;
		    bool receivingHistory = false;

		    if (running && sock >= 0) {
				ircConnected = true;
				fileLog("[IRC] Packet receive loop started");
				while (running && sock >= 0) {
					// "keep alive" packet, really checks if the server is still up
					Packet keepAlivePacket = {.type = 9, .length = (uint32_t)(ircNickname.length() + 1), .payload = *const_cast<char*>(ircNickname.c_str())};
					int sent = send(sock, &keepAlivePacket, sizeof(keepAlivePacket), 0);
					if (sent <= 0) {
						running = false;
						sock = -1;
						break;
					} else {
						fileLog("[IRC] Sent keepAlive packet.");
					}
					
		    	    Packet packet{};
		    	    ssize_t total = 0;
		    	    uint8_t* ptr = reinterpret_cast<uint8_t*>(&packet);
				
		    	    // Receive full packet
		    	    while (total < sizeof(packet)) {
		    	        ssize_t n = recv(sock, ptr + total, sizeof(packet) - total, 0);
		    	        if (n < 0) {
		    	            break;
		    	        }
		    	        total += n;
		    	    }
									
		    	    if (packet.type == 3) {  // BEGIN
						fileLog("[IRC] Started receiving.");
		    	        receivingHistory = true;
		    	        buffer.clear();
		    	    } else if (packet.type == 2 && receivingHistory) {
		    	        std::string line(packet.payload, packet.length);
		    	        buffer += line + "\n";
		    	    } else if (packet.type == 4 && receivingHistory) {  // END
						fileLog("[IRC] Stopped receiving.");
		    	        receivingHistory = false;
					
		    	        std::lock_guard<std::mutex> lock(file_mutex);
		    	        std::ofstream out(ircWindowFile, std::ios::trunc);
		    	        if (out) out << buffer;
		    	        buffer.clear();
		    	    }

					std::this_thread::sleep_for((std::chrono::milliseconds)250); // eepy
				}
				ircConnected = false;
				fileLog("[IRC] Connection to server lost, sock: %i", sock);
		    } else {
				fileLog("[IRC] Packet receive loop failed to start");
			}
			fileLog("[IRC] Packet receive loop stopped");
		}
	}
	
	namespace Commands {
		// Simple string splitting function
		std::vector<std::string> split(const std::string& str) {
			std::istringstream iss(str);
			std::vector<std::string> tokens;
			std::string token;
			while (iss >> token) tokens.push_back(token);
			return tokens;
		}	
		void interpretIRCCommand(std::string commandIn) {
		    auto tokens = split(commandIn);
		    if (tokens.empty()) return;
		
		    int argc = static_cast<int>(tokens.size());
		    std::vector<char*> argv(argc);
		    for (int i = 0; i < argc; ++i) {
		        argv[i] = &tokens[i][0];
		    }
		
		    std::string cmd = tokens[0];
		
		    // Dispatch commands
		    if (cmd == "/nick") {
				lastNickname = ircNickname;
				ircNickname = argv[1];
				initNetworkConnection();
				ircUserPrompt = ircNickname + " -> ";
		    } else if (cmd == "/connect" || cmd == "/join") {
				initNetworkConnection();
			} else if (cmd == "/disconnect" || cmd == "/leave") {
				closeConnection();
			}
		}
	}

	int interpretIRCInput(std::string input) {
		if (input[0] == '/') { // If it's a command
			Commands::interpretIRCCommand(input);
		} else { // If it's a message
			if (ircNickname == "") {
				
			} else {
				sendMessage(ircNickname, input);
			}
		}
		return 1;
	}
	
	bool isConnected() {
		return running && sock >= 0;
	}

	int closeConnection() {
		if (sock >= 0) {
			// Send disconnect packet
			Packet p{};
			std::string text = "disconnect";
			p.type = 5;
			p.length = std::min((int)text.size(), 256);
			memcpy(p.payload, text.c_str(), p.length);
	
			send(sock, &p, sizeof(p), 0); // Ignore result; server may be gone
			close(sock);
			sock = -1;  // Do this before touching any thread logic
		}
		
		running = false;

		if (listenerStarted && listenerThread.joinable()) {
			listenerThread.join();
			listenerStarted = false;
		}

		clientID = -1;

		fileLog("[Client] Disconnected and cleaned up");
		return 1;
	}

	int initNetworkConnection(const std::string& server_ip, int port) {
	    closeConnection();  // Clean up any previous state
	
	    sock = socket(AF_INET, SOCK_STREAM, 0);
	    if (sock < 0) {
	        fileLog("[Networking] socket() failed");
	        return -1;
	    }
	
	    sockaddr_in serv_addr{};
	    serv_addr.sin_family = AF_INET;
	    serv_addr.sin_port = htons(port);
	
	    if (inet_pton(AF_INET, server_ip.c_str(), &serv_addr.sin_addr) <= 0) {
	        fileLog("[Networking] Invalid server address: %s\n", server_ip.c_str());
	        close(sock);
	        sock = -1;
	        return -1;
	    }
	
	    if (connect(sock, (sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
	        fileLog("[Networking] Connection failed");
	        close(sock);
	        sock = -1;
	        return -1;
	    }
	
	    // Set a timeout for recv to avoid blocking forever
	    struct timeval timeout = {5, 0}; // 5 seconds
	    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));
	
	    // Receive welcome packet
	    Packet welcome{};
	    ssize_t received = recv(sock, &welcome, sizeof(welcome), 0);
	    if (received <= 0 || welcome.type != 0) {
	        fileLog("[Networking] Welcome packet failed (recv returned %zd, type: %u)", received, welcome.type);
	        close(sock);
	        sock = -1;
	        return -1;
	    }
	
	    std::string response(welcome.payload, welcome.length);
	    if (std::sscanf(response.c_str(), "Welcome %d", &clientID) != 1) {
	        fileLog("[Networking] Failed to parse client ID from welcome");
	        close(sock);
	        sock = -1;
	        return -1;
	    }
	
	    fileLog("[Networking] Connected successfully. ID: %d", clientID);
	
	    // Only mark running and start thread now
	    running = true;
	    listenerThread = std::thread(receiveLoop);
	    listenerStarted = true;
	
	    return clientID;
	}

	int reconnect(const std::string& server_ip, int port) {
		int result = -1;
		result = initNetworkConnection();
		return result;
	}

	int sendMessage(const std::string& userNick, const std::string& message) {
		if (!isConnected()) {
			fileLog("[Networking] Not connected. Cannot send message.");
			return -1;
		}

		std::string full = "[" + userNick + "] " + message;
		Packet p{};
		p.type = 1;
		p.length = std::min((int)full.size(), 256);
		memcpy(p.payload, full.c_str(), p.length);

		int sent = send(sock, &p, sizeof(p), 0);
		if (sent != sizeof(p)) {
			fileLog("[Networking] Failed to send packet fully");
			return -1;
		}

		fileLog("[Networking] Message sent: %s", full.c_str());
		return 0;
	}
}
