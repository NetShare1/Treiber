#pragma once

#include "UDPWorkerConfig.h"
#include "Log.h"
#include "WorkPacket.h"

#include <WinSock2.h>


int WorkSocket(UDPWorkerConfig& conf) {
	WSADATA wsaData;
	SOCKET SendingSocket;
	SOCKADDR_IN SocketAddr;
	SOCKADDR_IN RecvAddr;

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
	SendingSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

	if (SendingSocket == INVALID_SOCKET) {
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
			(conf.socketIpv4Adress.ipp3 <<  8) |
			(conf.socketIpv4Adress.ipp4 <<  0)
		);

	if (bind(SendingSocket, (SOCKADDR*)&SocketAddr, sizeof(SocketAddr)) == SOCKET_ERROR) {
		wchar_t* s = NULL;
		FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL, WSAGetLastError(),
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			(LPWSTR)&s, 0, NULL
		);
		
		Log(WINTUN_LOG_ERR, L"Bind on socket failed! Error: %s.\n", s);
		
		LocalFree(s);
		// Close the socket
		closesocket(SendingSocket);
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

	RecvAddr.sin_family = AF_INET;
	RecvAddr.sin_port = htons(conf.conf->serverPort);
	RecvAddr.sin_addr.s_addr = 
		htonl(
			(conf.conf->serverIpv4Adress.ipp1 << 24) |
			(conf.conf->serverIpv4Adress.ipp2 << 16) |
			(conf.conf->serverIpv4Adress.ipp3 << 8 ) |
			(conf.conf->serverIpv4Adress.ipp4 << 1 )
		);
	

	if (!conf.reciever) {
		int iResult;
		while (true) {
			WorkPacket* packet = conf.conf->packetqueue->remove();

			NS_PACKET_RECIEVED;

			// if the driver has stopped
			if (!conf.conf->isRunning) {
				break;
			}

			iResult = sendto(SendingSocket, (char*)packet->packet, packet->packetSize, 0, (SOCKADDR*)&RecvAddr, sizeof(RecvAddr));
	
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
			delete packet;

			if (iResult == SOCKET_ERROR) {
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
				closesocket(SendingSocket);
				WSACleanup();
				LocalFree(s);
				return 2819;
			}
		}
	}
	else {

		BYTE* buffer = new BYTE[NS_RECIEVE_BUFFER_SIZE]();

		int remoteAdressLen = sizeof(RecvAddr);

		int iResult;
		while (true) {

			// when no data is recieved the thread will be force terminated
			// and can not clean after himself because this is a blocking function
			// need to find a solution for that.
			iResult = recvfrom(
				SendingSocket,
				(char*)buffer,
				NS_RECIEVE_BUFFER_SIZE,
				0,
				(SOCKADDR*)&RecvAddr,
				&remoteAdressLen	
			);

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

			if (iResult == SOCKET_ERROR) {
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
				closesocket(SendingSocket);
				WSACleanup();
				return 2820;
			}
		}

		delete buffer;
	}

	LOG(WINTUN_LOG_INFO, "Shutting down UDP Sender");
	WSACleanup();
	return ERROR_SUCCESS;
}