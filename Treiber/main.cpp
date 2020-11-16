#ifndef WIN32
	#define WIN32
#endif
#include <WinSock2.h>
#include <ws2tcpip.h>
// wenn ich das drüber ned mach bekomm ich 50+ Fehler nice
#include <Windows.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config-msvc.h"
#include "syshead.h"
#include "tun.h"
#include "proto.h"

// link with Ws2_32.lib
// needed for getaddrinfo and freeaddrinfo
#pragma comment (lib, "Ws2_32.lib")

int main() {

	DWORD dwRetval;

	struct addrinfo hints;
	struct addrinfo* result = NULL;

	ZeroMemory(&hints, sizeof(hints)); // zero the memory of hints
	hints.ai_family = AF_INET; // is ipv4 address
	hints.ai_socktype = SOCK_STREAM; // two way stream socket
	hints.ai_protocol = IPPROTO_TCP; // tcp connection ["IPPROTO_UDP" for udp]


	dwRetval = getaddrinfo(
		"127.0.0.2", // IP Addresse
		"80", // Port
		&hints, // hints to the connection
		&result // object where data will be stored [is a double linked list]
	);

	if (dwRetval != 0) { // error accoured
		printf("getaddrinfo failed with error: %d\n", dwRetval);
		exit(1);
	}

	struct tuntap* tt = init_tun(
		"tun",
		"tun",
		TOP_P2P, // topology = Point to Point
		"127.0.0.1", // local IP Adresse dotted
		"127.0.0.2", // remote IP Address
		NULL, // ipv6 not needed
		0, // ipv6 not needed
		NULL, // ipv6 not needed
		result, // first address in the linked list. Hopfully this is my own
		result->ai_next, // second Hopefully this is the remote
		true, // use better configuration checks
		NULL, // not needed only for configuration over envirement variables
		NULL // is not used in the function at all wtf
	);
	
	open_tun(
		"tun",
		"tun", // type of device i want to init can be: "tun", "tap", "null"
		"ROOT/NET/0005",
		tt
	);


	freeaddrinfo(result); // remove information again
}