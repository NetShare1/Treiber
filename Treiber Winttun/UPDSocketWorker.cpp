#pragma once

#include "UPDSocketWorker.h"


int initSocket(std::shared_ptr<UDPWorkerConfig> conf) {
	MTR_SCOPE("Startup", __FUNCSIG__);
	WSADATA wsaData;
	SOCKADDR_IN SocketAddr;

	// initialize winsock
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		wchar_t* s = NULL;
		FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL, WSAGetLastError(),
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			(LPWSTR)&s, 0, NULL
		);
		std::wstring ws(s);
		std::string str(ws.begin(), ws.end());
		NS_LOG_APP_CRITICAL("Error trying to start WSA: {}", str);
		LocalFree(s);
		return -1;
	}

	// Create UDP Socket
	conf->socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

	if (conf->socket == INVALID_SOCKET) {
		wchar_t* s = NULL;
		FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL, WSAGetLastError(),
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			(LPWSTR)&s, 0, NULL
		);
		std::wstring ws(s);
		std::string str(ws.begin(), ws.end());
		NS_LOG_APP_CRITICAL("Error creating socket(): {}", str);
		LocalFree(s);
		// Clean up
		WSACleanup();
		// Exit with error
		return -1;
	}

	// The IPv4 family
	SocketAddr.sin_family = AF_INET;

	// Set port and IpAddress of Socket
	// IP address needs to be the one of the adapter we want to bind to
	SocketAddr.sin_port = htons(conf->socketPort);
	SocketAddr.sin_addr.s_addr =
		htonl(
			(conf->socketIpv4Adress.ipp1 << 24) |
			(conf->socketIpv4Adress.ipp2 << 16) |
			(conf->socketIpv4Adress.ipp3 << 8) |
			(conf->socketIpv4Adress.ipp4 << 0)
		);

	if (bind(conf->socket, (SOCKADDR*)&SocketAddr, sizeof(SocketAddr)) == SOCKET_ERROR) {
		wchar_t* s = NULL;
		FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL, WSAGetLastError(),
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			(LPWSTR)&s, 0, NULL
		);

		std::wstring ws(s);
		std::string str(ws.begin(), ws.end());
		NS_LOG_APP_CRITICAL("Bind on socket failed! Error: {}", str);

		LocalFree(s);
		// Close the socket
		closesocket(conf->socket);
		// Do the clean up
		WSACleanup();
		// and exit with error
		return -1;
	}

	NS_LOG_APP_DEBUG(
		"Successfully binded socket on {}.{}.{}.{}:{}",
		conf->socketIpv4Adress.ipp1,
		conf->socketIpv4Adress.ipp2,
		conf->socketIpv4Adress.ipp3,
		conf->socketIpv4Adress.ipp4,
		conf->socketPort
	);

	u_long mode = 1;
	int res = ioctlsocket(conf->socket, FIONBIO, &mode);
	if (res != NO_ERROR) {
		wchar_t* s = NULL;
		FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL, WSAGetLastError(),
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			(LPWSTR)&s, 0, NULL
		);

		std::wstring ws(s);
		std::string str(ws.begin(), ws.end());
		NS_LOG_APP_CRITICAL("Bind on socket failed! Error: {}", str);

		LocalFree(s);
		// Close the socket
		closesocket(conf->socket);
		// Do the clean up
		WSACleanup();
		// and exit with error
		return -1;
	}
}

void closeSockets(std::vector<std::shared_ptr<UDPWorkerConfig>>* configs) {
	MTR_SCOPE("Shutdown", __FUNCSIG__);
	NS_LOG_APP_DEBUG("Shutting down UDP Sockets");
	for (int i = 0; i < configs->size(); i++) {
		closesocket(configs->at(i)->socket);
	}
}

void shutdownWSA() {
	MTR_SCOPE("Wintun_adapter", __FUNCSIG__);
	WSACleanup();
}

int WorkSocket(std::shared_ptr<UDPWorkerConfig> conf) {
#ifdef NS_PERF_PROFILE
	std::string threadName = "UDPWorker Thread " + conf->uid;
	threadName += conf->reciever ? " [receiving]" : " [sending]";
	MTR_META_THREAD_NAME(threadName.c_str());
#endif

	std::string loggerName = "udpworker-";
	conf->reciever ? loggerName.append("receiver-") : loggerName.append("sender-");
	loggerName.append(std::to_string(conf->uid));

	NS_CREATE_WORKER_LOGGER(loggerName, ns::log::trace);


	WSADATA wsaData;
	SOCKET SendingSocket = conf->socket;
	SOCKADDR_IN SocketAddr;
	SOCKADDR_IN RecvAddr;

	RecvAddr.sin_family = AF_INET;
	RecvAddr.sin_port = htons(conf->conf->serverPort);
	RecvAddr.sin_addr.s_addr =
		htonl(
			(conf->conf->serverIpv4Adress.ipp1 << 24) |
			(conf->conf->serverIpv4Adress.ipp2 << 16) |
			(conf->conf->serverIpv4Adress.ipp3 << 8) |
			(conf->conf->serverIpv4Adress.ipp4 << 0)
		);


	if (!conf->reciever) {
		int iResult;
		while (conf->conf->isRunning) {
			size_t elements;
			WorkPacket** packet = conf->conf->sendingPacketQueue->popSize(3, &elements);
			// if the driver has stopped
			if (!conf->conf->isRunning) {
				delete[] packet;
				break;
			}
			for (size_t i = 0; i < elements; i++) {

				MTR_SCOPE("UDP_Sending", "Sending Multible packets");
				
				MTR_BEGIN("UDP_Sending", "SendingPacket");
				iResult = sendto(
					SendingSocket,
					(char*)packet[i]->packet,
					packet[i]->packetSize,
					0,
					(SOCKADDR*)&RecvAddr,
					sizeof(RecvAddr)
				);
				MTR_END("UDP_Sending", "SendingPacket");
				conf->conf->stats.udpPacketSent(packet[i]->packetSize);

				NS_LOG_TRACE(
					loggerName,
					"Sent Datagram to {}.{}.{}.{}:{} from{}.{}.{}.{}:{} with length: {}",
					conf->conf->serverIpv4Adress.ipp1,
					conf->conf->serverIpv4Adress.ipp2,
					conf->conf->serverIpv4Adress.ipp3,
					conf->conf->serverIpv4Adress.ipp4,
					conf->conf->serverPort,
					conf->socketIpv4Adress.ipp1,
					conf->socketIpv4Adress.ipp2,
					conf->socketIpv4Adress.ipp3,
					conf->socketIpv4Adress.ipp4,
					conf->socketPort,
					std::to_string(packet[i]->packetSize)
				);

				if (iResult == SOCKET_ERROR) {
					MTR_SCOPE("UDP_Sending", "Handling Packet Error");
					if (WSAGetLastError() == WSAEWOULDBLOCK) {
						// packet could not be sent because internal buffer is full. Try again
						// next time until all packets are sent away. Could get very CPU intensive
						i--;
						continue;
					}
					wchar_t* s = NULL;
					FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
						NULL, WSAGetLastError(),
						MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
						(LPWSTR)&s, 0, NULL
					);

					std::wstring ws(s);
					std::string str(ws.begin(), ws.end());
					NS_LOG_WARN(
						loggerName,
						"sendto failed with error: {}",
						str
					);
					LocalFree(s);
					return 2819;
				}

			}
		}
	}
	else {

		BYTE* buffer = new BYTE[NS_RECIEVE_BUFFER_SIZE]();

		int remoteAdressLen = sizeof(RecvAddr);

		FD_SET readfds;
		FD_SET writefds;
		FD_SET exceptfds;

		FD_ZERO(&readfds);
		FD_ZERO(&writefds);
		FD_ZERO(&exceptfds);

		FD_SET(SendingSocket, &readfds);

		timeval timeout{};
		timeout.tv_sec = 1;
		timeout.tv_usec = 0;

		int iResult;
		while (conf->conf->isRunning) {
			{
				FD_ZERO(&readfds);
				FD_ZERO(&writefds);
				FD_ZERO(&exceptfds);

				FD_SET(SendingSocket, &readfds);

				int total = select(0, &readfds, &writefds, &exceptfds, &timeout);

				if (total == -1) {
					if (WSAGetLastError() != WSAEWOULDBLOCK) {
						MTR_SCOPE("UDP_Receiving", "Error Handling critical Packet error");
						wchar_t* s = NULL;
						FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
							NULL, WSAGetLastError(),
							MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
							(LPWSTR)&s, 0, NULL
						);
						std::wstring ws(s);
						std::string str(ws.begin(), ws.end());
						NS_LOG_WARN(
							loggerName,
							"select failed with error: {}",
							str
						);
						LocalFree(s);
						return 2820;
					}
				}
				else if (total == 0) {
					if (!conf->conf->isRunning) {
						NS_LOG_INFO(loggerName, "Shutting down Adapter");
						break;
					}
					continue;
				}
				else {
					MTR_SCOPE("UDP_Receiving", "Getting and Processing Packet");
					{
						MTR_SCOPE("UDP_Receiving", "Receiving Packet");
						iResult = recvfrom(
							SendingSocket,
							(char*)buffer,
							NS_RECIEVE_BUFFER_SIZE,
							0,
							(SOCKADDR*)&RecvAddr,
							&remoteAdressLen
						);
					}

					if (iResult > 0) {
						MTR_SCOPE("UDP_Receiving", "Processing Packet");

						BYTE* internalPacket = new BYTE[iResult];
						std::copy(buffer, buffer + iResult, internalPacket);

						conf->conf->recievingPacketQueue->push(new WorkPacket(internalPacket, iResult));

						conf->conf->stats.udpPacketRecieved(iResult);

						if (!conf->conf->isRunning) {
							break;
						}

						NS_LOG_TRACE(
							loggerName,
							"Recieved Datagram from {}.{}.{}.{}:{} on {}.{}.{}.{}:{} with length: {}",
							conf->conf->serverIpv4Adress.ipp1,
							conf->conf->serverIpv4Adress.ipp2,
							conf->conf->serverIpv4Adress.ipp3,
							conf->conf->serverIpv4Adress.ipp4,
							conf->conf->serverPort,
							conf->socketIpv4Adress.ipp1,
							conf->socketIpv4Adress.ipp2,
							conf->socketIpv4Adress.ipp3,
							conf->socketIpv4Adress.ipp4,
							conf->socketPort,
							iResult
						);

					}

					if (iResult == SOCKET_ERROR) {
						MTR_SCOPE("UDP_Receiving", "Error Handling Packet");
						if (WSAGetLastError() != WSAEWOULDBLOCK) {
							MTR_SCOPE("UDP_Receiving", "Error Handling critical Packet error");
							wchar_t* s = NULL;
							FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
								NULL, WSAGetLastError(),
								MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
								(LPWSTR)&s, 0, NULL
							);
							std::wstring ws(s);
							std::string str(ws.begin(), ws.end());
							NS_LOG_WARN(
								loggerName,
								"recvfrom failed with error: {}",
								str
							);
							LocalFree(s);
							return 2820;
						}
						NS_LOG_WARN(loggerName, "Read reading buffer without any data init");
					}

				}
			}
		}
		delete[] buffer;
	}

	NS_LOG_INFO(loggerName, "Shutting down UDP Socket Worker");
	return ERROR_SUCCESS;
}