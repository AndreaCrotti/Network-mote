// vim:encoding=latin1:foldmethod=marker:foldmarker=//[[,//]]
/*
 * Copyright (c) 2009, Distributed Systems Group, RWTH Aachen
 * All rights reserved.
 */

/**
 * @file	system_tun.c
 * @brief	Platform-dependent functions for LINUX, using tun/tap devices
 */

///////////////////////////////////////////
// THIS FILE GETS INCLUDED FROM SYSTEM.C //
///////////////////////////////////////////

#include <linux/if_tun.h>

// The interface name of the alpha tun/tap network interface
// This is only needed within this file, so keep it local (but file-global)
static char g_device[IFNAMSIZ]=IFACENAME;

/*********************** PUBLIC FUNCTIONS *****************************/

ap_err_code system_addhost(const struct sockaddr_in *dest, const config_t* conf) {
//[[

	char buf[SYS_BUFSIZE];

	// Add mangle rule for UDP
	snprintf(buf, SYS_BUFSIZE, "%s %s -t mangle -A OUTPUT -p udp -m udp -d %s ! --dport %u -j MARK --set-mark %u",
		SYS_PREFIX, SYS_IPTABLES, inet_ntoa(dest->sin_addr), ntohs(dest->sin_port), SYS_MARKTABLE
	);
	if(system(buf) == -1) {
		print_error("system(): Failed to add iptables mangle rule for udp. Aborting.\n");
		return AP_ERR_SOCKET;
	}

	// Add mangle rule for TCP
	snprintf(buf, SYS_BUFSIZE, "%s %s -t mangle -A OUTPUT -p tcp -m tcp -d %s -j MARK --set-mark %u",
		SYS_PREFIX, SYS_IPTABLES, inet_ntoa(dest->sin_addr), SYS_MARKTABLE
	);
	if(system(buf) == -1) {
		print_error("system(): Failed to add iptables mangle rule for tcp. Aborting.\n");
		return AP_ERR_SOCKET;
	}

	// Add mangle rule for ICMP
	#ifdef MANGLE_ICMP
	snprintf(buf, SYS_BUFSIZE, "%s %s -t mangle -A OUTPUT -p icmp -d %s -j MARK --set-mark %u",
		SYS_PREFIX, SYS_IPTABLES, inet_ntoa(dest->sin_addr), SYS_MARKTABLE
	);
	if(system(buf) == -1) {
		print_error("system(): Failed to add iptables mangle rule for icmp. Aborting.\n");
		return AP_ERR_SOCKET;
	}
	#endif

	// Delete possibly existing old rules
	snprintf(buf, SYS_BUFSIZE, "%s %s rule del fwmark %u lookup %u 2> /dev/null",
		SYS_PREFIX, SYS_IPROUTE, SYS_MARKTABLE, SYS_ROUTINGTABLE
	);
	system(buf); // We dont care if this fails

	// Add new marktable to routing table
	snprintf(buf, SYS_BUFSIZE, "%s %s rule add fwmark %u lookup %u", SYS_PREFIX, SYS_IPROUTE, SYS_MARKTABLE, SYS_ROUTINGTABLE);
	if(system(buf) == -1) {
		print_error("system(): Failed to add iproute2 routing rule. Aborting.\n");
		return AP_ERR_SOCKET;
	}

	// Delete possibly existing old default interface
	snprintf(buf, SYS_BUFSIZE, "%s %s route del default dev %s table %u 2> /dev/null", SYS_PREFIX, SYS_IPROUTE, g_device, SYS_ROUTINGTABLE);
	system(buf); // We dont care if this fails

	// Add default interface for new table
	snprintf(buf, SYS_BUFSIZE, "%s %s route add default dev %s table %u", SYS_PREFIX, SYS_IPROUTE, g_device, SYS_ROUTINGTABLE);
	if(system(buf) == -1) {
		print_error("system(): Failed to add iproute2 default route. Aborting.\n");
		return AP_ERR_SOCKET;
	}

	return AP_ERR_SUCCESS;
} //]] 

// This uses ioctl to set the interface MTU without restarting it
ap_err_code system_set_tun_mtu(const unsigned int mtu) {
//[[
	int err=0;
	struct ifreq arg;
	arg.ifr_mtu = mtu;
	strcpy(arg.ifr_name, g_device);

	int sock = socket(AF_INET, SOCK_DGRAM, 0);
	err = ioctl(sock,SIOCSIFMTU,(caddr_t) &arg);
	close(sock);

	if(err < 0) {
		print_error("ioctl() failed: %s%s\n", strerror(errno), (errno == EPERM)?" (are you root?)":"");
		return AP_ERR_SOCKET;
	}
	return AP_ERR_SUCCESS;
} //]]
	
/*********************** PRIVATE FUNCTIONS *****************************/

/** Set up and open the tun device
 * @param	sock	Pointer to the variable where the tunnels file descriptor should be saved to
 * @param	mtu	The mtu of the tunnel device we want to set
 * @return	AP_ERR_SUCCESS or AP_ERR_SOCKET if something fails.
 */
static ap_err_code system_open_sock_outgoing(int *sock, const unsigned int mtu) {
//[[
	struct ifreq ifr;

	// open the tun device
	if( (*sock = open(TUNTAPDEVICE, O_RDWR)) < 0 ) {
		print_error("Error opening tun device: %s%s\n", strerror(errno), (errno==EACCES||errno==EPERM)?" (are you root?)":"");
		return AP_ERR_SOCKET;
	}

	memset(&ifr, 0, sizeof(ifr));

	// No packet information
	// so not ioctl() necessary here (it wont work anyways)
	int err;
	ifr.ifr_flags = IFF_TUN | IFF_NO_PI;
	strncpy(ifr.ifr_name, g_device, IFNAMSIZ);

	if((err = ioctl(*sock, TUNSETIFF, &ifr)) < 0) {
		print_error("ioctl() failed: %s%s\n", strerror(errno), (errno == EPERM)?" (are you root?)":"");

		if(close(*sock) == -1) {
			print_error("close() failed: %s\n", strerror(errno));
		}

		return AP_ERR_SOCKET;
	}

	strcpy(g_device, ifr.ifr_name);
	statusmsg("Successfully opened tun/tap device (%s)\n", g_device);

	statusmsg("Setting up linux specific iptables rules and setting mtu (%d)\n", mtu);
	char buf[SYS_BUFSIZE];

	// Flush the iptables mangle table
	// WARNING: If you have other mangle rules, they get flushed here!
	// TODO: This should be changed somehow, so only Alpha rules are deleted
	if(system(SYS_PREFIX" "SYS_IPTABLES" -t mangle -F OUTPUT") == -1) {
		print_error("system(): Failed to flush iptables mangle table. Aborting.\n");
		return AP_ERR_SOCKET;
	}

	// Set up the interface with its mtu
	snprintf(buf, SYS_BUFSIZE, "%s %s %s up mtu %u", SYS_PREFIX, SYS_IFCONFIG, g_device, mtu);
	if(system(buf) == -1) {
		print_error("system(): Failed to set mtu on %s to %u. Aborting.\n", g_device, mtu);
		return AP_ERR_SOCKET;
	}

	// Disable the rp_filter; this is very important, because the
 	// rp_filter would screw up everything. Packets are coming in on
 	// tun0, but outgoing packets would, according to THE DEFAULT ROUTING
 	// TABLE (not the one we define in route.sh) not use tun0, too. So,
 	// the kernel thinks our packets are spoofed and drops them silently.
	snprintf(buf, SYS_BUFSIZE, "echo \"0\" > /proc/sys/net/ipv4/conf/%s/rp_filter", g_device);
	if(system(buf) == -1) {
		print_error("system(): Failed to disable Reverse Path Filter on %s. Aborting.\n", g_device);
		return AP_ERR_SOCKET;
	}

	return AP_ERR_SUCCESS;
} //]]

/** Close the tunnel
 * @return AP_ERR_SUCCESS or AP_ERR_SOCKET if close() fails
 */
static ap_err_code system_close_sock_outgoing(const config_t *conf) {
//[[
	statusmsg("Closing tunnel %s\n", g_device);

	if(close(conf->socket_outgoing) == -1) {
		print_error("close() failed: %s\n", strerror(errno));
		return AP_ERR_SOCKET;
	}

	return AP_ERR_SUCCESS;
} //]]
