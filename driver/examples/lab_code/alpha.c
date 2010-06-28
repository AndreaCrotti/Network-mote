// vim:encoding=latin1:foldmethod=marker:foldmarker=//[[,//]]
/*
 * Copyright (c) 2009, Distributed Systems Group, RWTH Aachen
 * All rights reserved.
 */

/**
 * @file	alpha.c
 * @brief	Main file for alpha daemon
 */

#include "system.h"

#include <errno.h>
#include <netinet/ip.h>
#include <sys/un.h>
#include <string.h>

#ifdef DEBUG_TUN
 #include <netdb.h>
#endif

#include "application.h"
#include "tools.h"
#include "signal.h"
#include "control.h"
#include "association.h"
#include "xmalloc.h"
#include "timemanager.h"

#include "alphacontrol.h"

/*************************** GLOBAL VARIABLES *******************************/

int main(int argc, char *argv[]) {

	config_t conf = {
		/* socket_incoming */	0,
		/* socket_outgoing */	0,
		/* socket_internal */	0,
		/* mtu */		TUN_MAX_PACKET_SIZE,
		/* port */		0,
		/* autoroute */		true,
		/* daemon */		false,
		/* config_file */	NULL,
		/* connect_secret */	0,
		/* private_key */	NULL,
		/* num_alpha_n */	0,
		/* num_alpha_c*/	0,
		/* num_alpha_m */	0,
		/* num_alpha_z */	0,
		/* alpha_sec_mode */	0,
		/* hchain_length */	0
	};

#ifndef HAVE_ON_EXIT
	// Yes, this looks weird. Function declaration inside main().
	// We use this so we can have a void/void function which can use the conf
	// data structure without using global variables
	// We need this because Mac OS X does not support on_exit(), only atexit()
	void on_exit_workaround(void) {
		byebye(0, &conf);
	}
#endif

	// Getopt
	//[[
	int c;
	bool override_autoroute = false;
	bool daemonize = false;

	while((c = getopt(argc,argv,"l:n:c:am:hd")) != -1) {
		switch(c) {
			case 'l':
				conf.hchain_length = atoi(optarg);
				break;
			case 'n':
				conf.num_alpha_n = atoi(strsep(&optarg, ":"));
				conf.num_alpha_c = atoi(strsep(&optarg, ":"));
				conf.num_alpha_m = atoi(strsep(&optarg, ":"));
				break;
			case 'c':
				conf.config_file = xmalloc(strlen(optarg)+1);
				strcpy(conf.config_file, optarg);
				break;
			case 'a':
				conf.autoroute = false;
				override_autoroute = true;
				break;
			case 'd':
				daemonize = true;
				break;
			case 'h':
			default:
				print_usage(argv[0]);
				return EXIT_SUCCESS;
				break;
			}
	}
	//]]

	if(daemonize) {
		pid_t pid = fork();

		if(pid == -1) {
			// Something went wrong
			print_error("Daemonizing failed! Exiting.\n");
			exit(EXIT_FAILURE);
		} else if(pid != 0) {
			// Everything went fine, this is the father process
			printf("alpha daemon started (process id: %d)\n", pid);
			exit(EXIT_SUCCESS);
		} else {
			// Everything went fine, this is the child process
			conf.daemon = true;
		}
	}

	// read global config
	app_read_config(&conf, override_autoroute);

	// We will need some random numbers...
	srand(getpid() * time(0));

	// set the secret for return-connect check
	conf.connect_secret = rand();

	// Set signal handler
#ifndef HAVE_ON_EXIT
	signal_init(&conf, on_exit_workaround);
#else
	signal_init(&conf, NULL);
#endif

	// Open tun device
	if(system_sockets_init(&conf) != AP_ERR_SUCCESS) {
		print_error("Cannot initialize the packet filtering functions or sockets.\n");
		exit(EXIT_FAILURE);
	}

	host_read_from_file(&conf);

	// Setup for select()
	struct timeval select_timeout;
	int ready, maxfd;
	fd_set fds;
	maxfd = (conf.socket_outgoing > conf.socket_incoming) ? conf.socket_outgoing : conf.socket_incoming;
	maxfd = (conf.socket_internal > maxfd) ? conf.socket_internal : maxfd;

	AP_MSG_P(AP_MSG_LVL_DEFAULT, AP_MSG_CTX_SYS, "Waiting for incoming events.\n");

	// Main loop
	statusmsg("\n");
	while(1) {

		unsigned int i;

		// Check if a client has packets buffered which we should send (if we are ready)
		//[[
		for(i=0; i<hosts_len(); i++) {
			if(host_get_state(i, ASS_TRANS_MODE_HANDSHAKE) == ASS_STATE_READY &&
				host_get_queue_size(i) > 0
			) {
				host_distribute_packets(i);
			}

			host_process_packets(&conf, i);
		}



		//]]

		// initialize read-fd-set
		FD_ZERO(&fds);
		if(conf.socket_outgoing > 0) FD_SET(conf.socket_outgoing, &fds);
		if(conf.socket_incoming > 0) FD_SET(conf.socket_incoming, &fds);
		if(conf.socket_internal > 0) FD_SET(conf.socket_internal, &fds);

		// Set timeout again, because this gets overwritten by select()
		// to indicate how much time passed if there is no timeout.
		// meaning: if there is no timeout the first time, this will
		// be zero (and the next call to select() will not block,
		// therefore: 100% of the cpu time will be used), unless
		// we update this every iteration
		select_timeout.tv_sec = HANDSHAKE_TIMEOUT;
		select_timeout.tv_usec = 0;

		// Check for timeouts
		//[[
		for(i=0; i<hosts_len(); i++) {
			struct timeval timestamp;
			int mode_handshake = host_get_state(i, ASS_TRANS_MODE_HANDSHAKE);
			host_get_timestamp(i, &timestamp);
			bool timeout_ex = timemanager_timeout_exceeded(timestamp);

			if(mode_handshake == ASS_STATE_SENT_CONNECT_WAIT_RETURN_CONNECT && timeout_ex) {
				statusmsg("<%d> CONNECT timed out after %d seconds. Retransmitting.\n", i, HANDSHAKE_TIMEOUT);
				host_set_timestamp(i, HANDSHAKE_TIMEOUT, 0);
				ap_protocol_send_connect(&conf, i);

			} else if(mode_handshake == ASS_STATE_SENT_SYN_WAIT_ACK && timeout_ex) {
				// If the handshake SYN timed out, send CONNECT again since we may have already passed the timeslot
				statusmsg("<%d> SYN timed out after %d seconds. Retransmitting.\n", i, HANDSHAKE_TIMEOUT);
				host_set_timestamp(i, HANDSHAKE_TIMEOUT, 0);
				ap_protocol_send_connect(&conf, i);

			} else if(mode_handshake == ASS_STATE_SENT_ACK_WAIT_ACKACK && timeout_ex) {
				// If we sent ACK, but ACKACK timed out, go into NEW again
#ifdef DEBUG_HANDSHAKE
					statusmsg("<%d> No ACKACK after %d seconds. Aborting handshake.\n", i, HANDSHAKE_TIMEOUT);
#endif
					host_set_state(i, 0, ASS_STATE_NEW);

			} else if(mode_handshake == ASS_STATE_READY) {
				host_handle_timeouts(i, &conf);
			}

			//
			// No, there is nothing missing here for A1 packets. A1 packets
			// are not supposed to get retransmitted! Only S1/S2 (the "active side")
			//

		}
		//]]

		// check which fd in the set is ready for reading
		ready = select(maxfd+1, &fds, NULL, NULL, &select_timeout);

		// something went wrong
		if(ready == -1) {
		//[[
			// If the cause of the error was an interuption
			// by a signal, try again, otherwise die
			if(errno == EINTR) { continue; }

			break;
		} //]]

		// no data ready for reading? try again
		if(ready == 0) { continue; }

		// read incoming IP raw packets from tun interface, if any available
		if(FD_ISSET(conf.socket_outgoing, &fds)) {
		//[[

			unsigned int bytes;
			unsigned char buf[conf.mtu];
			if((bytes = read(conf.socket_outgoing, buf, conf.mtu)) == (unsigned int)-1) {
				print_error("read() from %s failed: %s\n", "Capture device", strerror(errno));
				return EXIT_FAILURE;
			}

			struct ip *packet = (struct ip*)buf;
			int id = host_find(packet->ip_dst);
			char dest[16];
			strncpy(dest, inet_ntoa(packet->ip_dst), 16);

#ifdef DEBUG_TUN
			struct protoent *protocol = getprotobynumber(packet->ip_p);
			char src[16];
			strncpy(src,  inet_ntoa(packet->ip_src), 16);

			char tmp[16];
			if(id>=0) {
				snprintf(tmp,16,"<%d>",id);
			} else {
				snprintf(tmp,16,"<?>");
			}

			statusmsg("%s --%s--> %s packet from %s to %s (%hu bytes, checksum %x)\n",
					tmp, "capture", protocol->p_name, src, dest, ntohs(packet->ip_len), ntohs(packet->ip_sum));
#endif

			// Drop packets to unknown hosts
			if(id == -1) {
#ifdef DEBUG_TUN
				print_error("Host %s is not a known destination host. Ignoring packet.\n", dest);
#endif
				continue;
			}

			// If the handshake has not been initiated yet, do so
			if(host_get_state(id,ASS_TRANS_MODE_HANDSHAKE) == ASS_STATE_NEW) {
				host_set_timestamp(id, HANDSHAKE_TIMEOUT, 0);
				ap_protocol_send_connect(&conf, id);
				host_set_state(id, ASS_TRANS_MODE_HANDSHAKE, ASS_STATE_SENT_CONNECT_WAIT_RETURN_CONNECT);
			}

			// Buffer the current packet and send it later, when we are in CS_READY
			host_enqueue_packet(id, buf, bytes);

		} //]]

		// read incoming packets from udp socket, if any available
		if(FD_ISSET(conf.socket_incoming, &fds)) {
		//[[

			char buf[UDP_MAX_PACKET_SIZE];
			struct sockaddr_in remote;
			memset(&remote, 0, sizeof(struct sockaddr_in));
			socklen_t fromlen = sizeof(struct sockaddr);

			unsigned int bytes = recvfrom(conf.socket_incoming, buf, UDP_MAX_PACKET_SIZE, 0, (struct sockaddr*)&remote, &fromlen);

			if(bytes == (unsigned int)-1) {
				print_error("recvfrom(): %s\n", strerror(errno));
				continue;
			}

			alpha_packet_t *packet = (alpha_packet_t*)buf;

			int id = host_find(remote.sin_addr);

#ifdef DEBUG_NET
			char tmp[16];

			if(id>=0) {
				snprintf(tmp,16,"<%d>",id);
			} else {
				snprintf(tmp,16,"<?>");
			}
			statusmsg("%s Received packet (type %u, %d bytes) from %s:%d\n", tmp, packet->type, bytes, inet_ntoa(remote.sin_addr), ntohs(remote.sin_port));
#endif

			if(packet->type == PACKET_CONNECT) {
				ap_protocol_handle_connect(&conf, &remote, &fromlen);
				continue;
			} else if(packet->type == PACKET_SYN) {
				if(!ap_protocol_verify_syn(&conf, (alpha_packet_syn_t*) packet, &remote)) {
					continue;
				}

				// the host is unknown to us, but it is using a verified IP. let's create it
				if (id ==-1) {
					id = host_add(&conf, inet_ntoa(remote.sin_addr), remote.sin_addr, NULL);
					statusmsg("<%d> Host %s created\n", id, inet_ntoa(remote.sin_addr));
				}
			}

			if(id==-1) {
				print_error("Host %s is not a known host. Ignoring packet.\n", inet_ntoa(remote.sin_addr));
				continue;
			}

			// Check if the packet has a valid type
			if(packet->type < PACKET_LOWEST || packet->type > PACKET_HIGHEST) {
				print_error("<%d> Received packet has invalid type (%u). Dropping.\n", id, packet->type);
				continue;
			}

			// Check if the packet is as long as it should be
			switch(packet->type) {
				case PACKET_SYN:
					if(bytes < sizeof(alpha_packet_syn_t)) {
						print_error("<%d> Packet is too small to be a SYN packet. Ignoring.\n", id);
						continue;
					}
					break;
				case PACKET_ACK:
					if(bytes < sizeof(alpha_packet_ack_t)) {
						print_error("<%d> Packet is too small to be an ACK packet. Ignoring.\n", id);
						continue;
					}
					break;
				case PACKET_ACKACK:
					if(bytes < sizeof(alpha_packet_ackack_t)) {
						print_error("<%d> Packet is too small to be an ACKACK packet. Ignoring.\n", id);
						continue;
					}
					break;
				case PACKET_S1:
					if(bytes < sizeof(alpha_packet_s1_t)) {
						print_error("<%d> Packet is too small to be a S1 packet. Ignoring.\n", id);
						continue;
					}
					break;
				case PACKET_A1:
					if(bytes < sizeof(alpha_packet_a1_t)) {
						print_error("<%d> Packet is too small to be a A1 packet. Ignoring.\n", id);
						continue;
					}
					break;
				case PACKET_S2:
					if(bytes < sizeof(alpha_packet_s2_t)) {
						print_error("<%d> Packet is too small to be a S2 packet. Ignoring.\n", id);
						continue;
					}
					break;
				case PACKET_RETURN_CONNECT:
					if(bytes < sizeof(alpha_packet_return_connect_t)) {
						print_error("<%d> Packet is too small to be a RETURN CONNECT packet. Ignoring.\n", id);
						continue;
					}
					break;

			}

			// Process the packet
			ap_protocol_process_udp_packet(&conf, id, packet, bytes);

		} //]]

		// read incoming packets from Unix domain socket, if any available
		if(FD_ISSET(conf.socket_internal, &fds)) {
		//[[

			char buf[INTERNAL_SOCK_MAX_SIZE];
			struct sockaddr_un from;
			socklen_t fromlen = sizeof(from);
			int bytes = recvfrom(conf.socket_internal, buf, (size_t)INTERNAL_SOCK_MAX_SIZE, 0, (struct sockaddr*)&from, &fromlen);
			if(bytes == -1) {
				print_error("Failed to receive alpha control request from socket: %s\n", strerror(errno));
				continue;
			}

			internal_packet *packet = (internal_packet*)buf;

			statusmsg("Received an Alpha control message!\n");

			switch(packet->type) {
				case INTERNAL_TERM:
					statusmsg("Alpha control wants us to die.\n");
					exit(EXIT_SUCCESS);
					break;
				case INTERNAL_VERSION:
					statusmsg("Alpha control wants to know our version number.\n");
					control_send_version(&conf,&from,fromlen);
					break;
				case INTERNAL_CLIENTLIST:
					statusmsg("Alpha control wants to know our clients.\n");
					control_send_clientlist(&conf,&from,fromlen);
					break;
				case INTERNAL_RELOAD:
					statusmsg("Alpha control wants us to reload our config file.\n");
					app_read_config(&conf, override_autoroute);
					host_read_from_file(&conf);
					break;
				default:
					statusmsg("Alpha control is talking non-sense (command %u unknown). Ignoring.\n", packet->type);
					break;
			}

		}
		//]]

		//
		// Dont add anything after here, this could be skipped due to continue's
		//

	}

	if(ready == -1) {
	//[[
		print_error("select(): %s\n", strerror(errno));
		return EXIT_FAILURE;
	} //]]

	return EXIT_SUCCESS;
}

