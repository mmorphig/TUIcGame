#ifndef IRC_LOGIC_HPP
#define IRC_LOGIC_HPP

#include <iostream>
#include <string>
#include <sstream>
#include <cstring>
#include <cstdint>
#include <thread>
#include <atomic>
#include <fstream>
#include <mutex>
#include <vector>
#include <chrono>
#include <unistd.h>
#include <arpa/inet.h>
#include "txt_writer.h"

extern std::string ircWindowFile;
extern std::string ircUserPrompt;
extern std::string ircNickname;
extern std::string ircText;
extern std::string ircConnectionTempFile;
extern bool ircConnected;
extern bool ircOpened;

#define IRCSERVERIP "127.0.0.1"
#define IRCPORT 12345

#pragma pack(push, 1)
struct Packet {
    uint8_t type;
    uint32_t length;
    char payload[256];
};
#pragma pack(pop)

namespace IRCLogic {
	namespace {
		int sock = -1;
		int clientID = -1;
		std::atomic<bool> running{false};
		std::atomic<bool> listenerStarted = false;
		std::thread listenerThread;
		std::mutex file_mutex;

		std::string introMessage = "\n\
Welcome to the chat window!\n\
 \n\
Make sure to not send any passwords,\n\
secrets or revealing info in here, obviously.\n\
use \"/nick [NICKNAME]\" to get started.\n\
";

		#pragma pack(push, 1)
		struct Packet {
			uint8_t type;
			uint32_t length;
			char payload[256];
		};
		#pragma pack(pop)
	}
	
	int interpretIRCInput(std::string input);
	bool isConnected();
	int reconnect(const std::string& server_ip = IRCSERVERIP, int port = IRCPORT);
	int initNetworkConnection(const std::string& server_ip = IRCSERVERIP, int port = IRCPORT);
	int sendMessage(const std::string& userNick, const std::string& message);
	int closeConnection();
	
	namespace Commands {
		std::vector<std::string> split(const std::string& str);
		void interpretIRCCommand(std::string commandIn);
	}
}

#endif
