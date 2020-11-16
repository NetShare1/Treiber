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
#include "socket.h"
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

struct tuntap*
    init_tun(const char* dev,        /* --dev option */
        const char* dev_type,   /* --dev-type option */
        int topology,           /* one of the TOP_x values */
        const char* ifconfig_local_parm,           /* --ifconfig parm 1 */
        const char* ifconfig_remote_netmask_parm,  /* --ifconfig parm 2 */
        const char* ifconfig_ipv6_local_parm,      /* --ifconfig parm 1 IPv6 */
        int ifconfig_ipv6_netbits_parm,
        const char* ifconfig_ipv6_remote_parm,     /* --ifconfig parm 2 IPv6 */
        struct addrinfo* local_public,
        struct addrinfo* remote_public,
        const bool strict_warn,
        struct env_set* es,
        openvpn_net_ctx_t* ctx)
{
    struct gc_arena gc = gc_new();
    struct tuntap* tt;

    ALLOC_OBJ(tt, struct tuntap);
    clear_tuntap(tt);

    tt->type = dev_type_enum(dev, dev_type);
    tt->topology = topology;

    if (ifconfig_local_parm && ifconfig_remote_netmask_parm)
    {
        bool tun = false;

        /*
         * We only handle TUN/TAP devices here, not --dev null devices.
         */
        tun = is_tun_p2p(tt);

        /*
         * Convert arguments to binary IPv4 addresses.
         */

        tt->local = getaddr(
            GETADDR_RESOLVE
            | GETADDR_HOST_ORDER
            | GETADDR_FATAL_ON_SIGNAL
            | GETADDR_FATAL,
            ifconfig_local_parm,
            0,
            NULL,
            NULL);

        tt->remote_netmask = getaddr(
            (tun ? GETADDR_RESOLVE : 0)
            | GETADDR_HOST_ORDER
            | GETADDR_FATAL_ON_SIGNAL
            | GETADDR_FATAL,
            ifconfig_remote_netmask_parm,
            0,
            NULL,
            NULL);

        /*
         * Look for common errors in --ifconfig parms
         */
        if (strict_warn)
        {
            struct addrinfo* curele;
            ifconfig_sanity_check(tt->type == DEV_TYPE_TUN, tt->remote_netmask, tt->topology);

            /*
             * If local_public or remote_public addresses are defined,
             * make sure they do not clash with our virtual subnet.
             */

            for (curele = local_public; curele; curele = curele->ai_next)
            {
                if (curele->ai_family == AF_INET)
                {
                    check_addr_clash("local",
                        tt->type,
                        ((struct sockaddr_in*)curele->ai_addr)->sin_addr.s_addr,
                        tt->local,
                        tt->remote_netmask);
                }
            }

            for (curele = remote_public; curele; curele = curele->ai_next)
            {
                if (curele->ai_family == AF_INET)
                {
                    check_addr_clash("remote",
                        tt->type,
                        ((struct sockaddr_in*)curele->ai_addr)->sin_addr.s_addr,
                        tt->local,
                        tt->remote_netmask);
                }
            }

            if (tt->type == DEV_TYPE_TAP || (tt->type == DEV_TYPE_TUN && tt->topology == TOP_SUBNET))
            {
                check_subnet_conflict(tt->local, tt->remote_netmask, "TUN/TAP adapter");
            }
            else if (tt->type == DEV_TYPE_TUN)
            {
                check_subnet_conflict(tt->local, IPV4_NETMASK_HOST, "TUN/TAP adapter");
            }
        }

#ifdef _WIN32
        /*
         * Make sure that both ifconfig addresses are part of the
         * same .252 subnet.
         */
        if (tun)
        {
            verify_255_255_255_252(tt->local, tt->remote_netmask);
            tt->adapter_netmask = ~3;
        }
        else
        {
            tt->adapter_netmask = tt->remote_netmask;
        }
#endif

        tt->did_ifconfig_setup = true;
    }

    if (ifconfig_ipv6_local_parm && ifconfig_ipv6_remote_parm)
    {

        /*
         * Convert arguments to binary IPv6 addresses.
         */

        if (inet_pton(AF_INET6, ifconfig_ipv6_local_parm, &tt->local_ipv6) != 1
            || inet_pton(AF_INET6, ifconfig_ipv6_remote_parm, &tt->remote_ipv6) != 1)
        {
            msg(M_FATAL, "init_tun: problem converting IPv6 ifconfig addresses %s and %s to binary", ifconfig_ipv6_local_parm, ifconfig_ipv6_remote_parm);
        }
        tt->netbits_ipv6 = ifconfig_ipv6_netbits_parm;

        tt->did_ifconfig_ipv6_setup = true;
    }

    /*
     * Set environmental variables with ifconfig parameters.
     */
    if (es)
    {
        do_ifconfig_setenv(tt, es);
    }

    gc_free(&gc);
    return tt;
}