#pragma once

#include "UDPWorkerConfig.h"
#include "Log.h"
#include "WorkPacket.h"
#include "diagnostics.h"

#include <WinSock2.h>


int initSocket(UDPWorkerConfig& conf) {
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
		Log(WINTUN_LOG_ERR, L"Error trying to start WSA: %d", s);
		LocalFree(s);
		return -1;
	}

	// Create UDP Socket
	conf.socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

	if (conf.socket == INVALID_SOCKET) {
		wchar_t* s = NULL;
		FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL, WSAGetLastError(),
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			(LPWSTR)&s, 0, NULL
		);
		Log(WINTUN_LOG_ERR, L"Error creating socket(): %s\n", s);
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
	SocketAddr.sin_port = htons(conf.socketPort);
	SocketAddr.sin_addr.s_addr =
		htonl(
			(conf.socketIpv4Adress.ipp1 << 24) |
			(conf.socketIpv4Adress.ipp2 << 16) |
			(conf.socketIpv4Adress.ipp3 << 8) |
			(conf.socketIpv4Adress.ipp4 << 0)
		);

	if (bind(conf.socket, (SOCKADDR*)&SocketAddr, sizeof(SocketAddr)) == SOCKET_ERROR) {
		wchar_t* s = NULL;
		FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL, WSAGetLastError(),
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			(LPWSTR)&s, 0, NULL
		);

		Log(WINTUN_LOG_ERR, L"Bind on socket failed! Error: %s.\n", s);

		LocalFree(s);
		// Close the socket
		closesocket(conf.socket);
		// Do the clean up
		WSACleanup();
		// and exit with error
		return -1;
	}

	Log(
		WINTUN_LOG_INFO,
		L"Successfully binded socket on %d.%d.%d.%d:%d",
		conf.socketIpv4Adress.ipp1,
		conf.socketIpv4Adress.ipp2,
		conf.socketIpv4Adress.ipp3,
		conf.socketIpv4Adress.ipp4,
		conf.socketPort
	);

	u_long mode = 1;
	int res = ioctlsocket(conf.socket, FIONBIO, &mode);
	if (res != NO_ERROR) {
		Log(WINTUN_LOG_ERR, L"Error setting socket in nonblocking mode");
	}

	/*struct timeval read_timeout;
	read_timeout.tv_sec = 0;
	read_timeout.tv_usec = 10;
	setsockopt(conf.socket, SOL_SOCKET, SO_RCVTIMEO, (char*)&read_timeout, sizeof read_timeout);
	setsockopt(conf.socket, SOL_SOCKET, SO_SNDTIMEO, (char*)&read_timeout, sizeof read_timeout);
	*/
}

void closeSockets(std::vector<UDPWorkerConfig*>* configs) {
	MTR_SCOPE("Shutdown", __FUNCSIG__);
	Log(WINTUN_LOG_INFO, L"Shutting down UDP Sockets");
	for (int i = 0; i < configs->size(); i++) {
		closesocket(configs->at(i)->socket);
	}
}

void shutdownWSA() {
	MTR_SCOPE("Wintun_adapter", __FUNCSIG__);
	WSACleanup();
}

int WorkSocket(UDPWorkerConfig& conf) {
#ifdef NS_PERF_PROFILE
	std::string threadName = "UDPWorker Thread " + conf.uid; 
	threadName += conf.reciever ? " [receiving]" : " [sending]";
	MTR_META_THREAD_NAME(threadName.c_str());
#endif

	WSADATA wsaData;
	SOCKET SendingSocket = conf.socket;
	SOCKADDR_IN SocketAddr;
	SOCKADDR_IN RecvAddr;
	
	RecvAddr.sin_family = AF_INET;
	RecvAddr.sin_port = htons(conf.conf->serverPort);
	RecvAddr.sin_addr.s_addr = 
		htonl(
			(conf.conf->serverIpv4Adress.ipp1 << 24) |
			(conf.conf->serverIpv4Adress.ipp2 << 16) |
			(conf.conf->serverIpv4Adress.ipp3 << 8 ) |
			(conf.conf->serverIpv4Adress.ipp4 << 0 )
		);
	

	if (!conf.reciever) {
		int iResult;
		while (true) {
			MTR_SCOPE("UDP_Sending", "Getting/Sending Packet");
			WorkPacket* packet = conf.conf->sendingPacketQueue->remove();

			// if the driver has stopped
			if (!conf.conf->isRunning) {
				break;
			}
			MTR_BEGIN("UDP_Sending", "SendingPacket");
			iResult = sendto(SendingSocket, (char*)packet->packet, packet->packetSize, 0, (SOCKADDR*)&RecvAddr, sizeof(RecvAddr));
			MTR_END("UDP_Sending", "SendingPacket");
			conf.conf->stats.udpPacketSent(packet->packetSize);

			DLOG(
				WINTUN_LOG_INFO,
				L"[%d] Sent Datagram to %d.%d.%d.%d:%d from %d.%d.%d.%d:%d with length: %d",
				conf.uid,
				conf.conf->serverIpv4Adress.ipp1,
				conf.conf->serverIpv4Adress.ipp2,
				conf.conf->serverIpv4Adress.ipp3,
				conf.conf->serverIpv4Adress.ipp4,
				conf.conf->serverPort,
				conf.socketIpv4Adress.ipp1,
				conf.socketIpv4Adress.ipp2,
				conf.socketIpv4Adress.ipp3,
				conf.socketIpv4Adress.ipp4,
				conf.socketPort,
				packet->packetSize
			);

			// delete packet because it is not needed anymore 
			delete packet->packet;
			delete packet;

			if (iResult == SOCKET_ERROR) {
				MTR_SCOPE("UDP_Sending", "Handling Packet Error");
				if (WSAGetLastError() == WSAEWOULDBLOCK) {
					continue;
				}
				wchar_t* s = NULL;
				FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
					NULL, WSAGetLastError(),
					MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
					(LPWSTR)&s, 0, NULL
				);
				Log(WINTUN_LOG_WARN,
					L"sendto failed with error: %s",
					s
				);
				LocalFree(s);
				return 2819;
			}
		}
	}
	else {

		BYTE* buffer = new BYTE[NS_RECIEVE_BUFFER_SIZE]();

		int remoteAdressLen = sizeof(RecvAddr);

		const int runForConst = 10;
		int runFor = runForConst; // run throughs until next wait
		int currWaitTime = 10; // Wait time in miliseconds
		int packetsReceivedCurr = 0;
		int packetsReceivedLast = 0;

		int iResult;
		while (true) {
			MTR_SCOPE("UDP_Receiving", "Getting/Processing Packet");
			// when no data is recieved the thread will be force terminated
			// and can not clean after himself because this is a blocking function
			// need to find a solution for that.
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
				packetsReceivedCurr++;
				BYTE* internalPacket = new BYTE[iResult];
				std::copy(buffer, buffer + iResult, internalPacket);

				conf.conf->recievingPacketQueue->insert(new WorkPacket(internalPacket, iResult));

				conf.conf->stats.udpPacketRecieved(iResult);

				// if the driver has stopped while blocked
				if (!conf.conf->isRunning) {
					break;
				}

				DLOG(
					WINTUN_LOG_INFO,
					L"[%d] Recieved Datagram from %d.%d.%d.%d:%d on %d.%d.%d.%d:%d with length: %d",
					conf.uid,
					conf.conf->serverIpv4Adress.ipp1,
					conf.conf->serverIpv4Adress.ipp2,
					conf.conf->serverIpv4Adress.ipp3,
					conf.conf->serverIpv4Adress.ipp4,
					conf.conf->serverPort,
					conf.socketIpv4Adress.ipp1,
					conf.socketIpv4Adress.ipp2,
					conf.socketIpv4Adress.ipp3,
					conf.socketIpv4Adress.ipp4,
					conf.socketPort,
					iResult
				);

			}

			if (iResult == SOCKET_ERROR) {
				MTR_SCOPE("UDP_Receiving", "Error Handling Packet");
				if (WSAGetLastError() != WSAEWOULDBLOCK) {
					wchar_t* s = NULL;
					FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
						NULL, WSAGetLastError(),
						MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
						(LPWSTR)&s, 0, NULL
					);
					Log(WINTUN_LOG_WARN,
						L"recvfrom failed with error: %s",
						s
					);
					LocalFree(s);
					return 2820;
				}
			}

			runFor--;
			if (runFor == 0) {
				if (!conf.conf->isRunning) {
					break;
				}

				MTR_SCOPE("UDP_Receiving", "Calculating Wait time and Waiting");
				if (packetsReceivedCurr != 0 && packetsReceivedCurr >= packetsReceivedLast) {
					currWaitTime / 2;
				}
				if (packetsReceivedCurr == 0 || packetsReceivedCurr < packetsReceivedLast) {
					if (currWaitTime = 0) {
						currWaitTime = 1;
					} else {
						currWaitTime * 2;
					}

					if (currWaitTime > 20) {
						currWaitTime = 20;
					}
				}
				packetsReceivedLast = packetsReceivedCurr;
				packetsReceivedCurr = 0;
				runFor = runForConst;

				std::this_thread::sleep_for(std::chrono::milliseconds(currWaitTime));
			}

		}
		delete buffer;
	}

	LOG(WINTUN_LOG_INFO, "Shutting down UDP Sender");
	return ERROR_SUCCESS;
}