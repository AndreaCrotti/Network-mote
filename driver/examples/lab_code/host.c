// vim:encoding=latin1:foldmethod=marker:foldmarker=//[[,//]]
/*
 * Copyright (c) 2009, Distributed Systems Group, RWTH Aachen
 * All rights reserved.
 */

/**
 * @file	host.h
 * @brief	Wrapper functions for working with the clients array
 */

/******************************** INCLUDES **********************************/

#include "system.h"

#include <netdb.h>
#include <openssl/pem.h>
#include <openssl/engine.h>
#include <libgen.h>
#include <assert.h>
#include <sys/time.h>

#include "application.h"
#include "tools.h"
#include "xmalloc.h"
#include "list.h"
#include "digest.h"
#include "alpha_n.h"
#include "alpha_c.h"
#include "alpha_m.h"
#include "key.h"
#include "control_association.h"
#include "timemanager.h"

/************************ FILE-GLOBAL VARIABLES ******************************/

struct s_clients clients = {0,NULL};

/************************ INLINE FUNCTIONS ***********************************/
inline uint32_t host_get_challenge(const unsigned int client_id) {
//[[
	assert(client_id < clients.len);
	return clients.c[client_id].challenge;
} //]]

inline void host_set_challenge(const unsigned int client_id, uint32_t challenge) {
//[[
	assert(client_id < clients.len);
	clients.c[client_id].challenge = challenge;
} //]]

/************************ LOCAL FUNCTIONS ******************************/

//! Initializes the associations data structure and the default association
/*!
 * The datastrucutre holding links to the associations used by this host is
 * created and additionally the default association is created and put into
 * this datastrucutre.
 *
 * @param[in] client_id The client whose associations should be initialized
 * @return AP_ERR_NOMEM if no memory is there or AP_ERR_SUCCESS
 */
static ap_err_code host_init_associations(const uint32_t client_id, const config_t *conf) {
//[[
	assert(client_id < clients.len);


	const size_t size_associations_array = sizeof(association_t*) * ASS_MAX_NUMBER;

	// init array for associations (pseudo hash table)

	// yes we calculate here the size of the pointer!
	clients.c[client_id].associations_out = xmalloc(size_associations_array);

	// set all pointers to zero!
	memset(clients.c[client_id].associations_out, 0, size_associations_array);

	// yes we calculate here the size of the pointer!
	clients.c[client_id].associations_in = xmalloc(size_associations_array);

	memset(clients.c[client_id].associations_in, 0, size_associations_array);

	// init free associations pseudo ring buffer
	clients.c[client_id].free_associations = ring_buffer_new(ASS_MAX_NUMBER, sizeof(association_t*));
	clients.c[client_id].free_associations_it = ring_buffer_iterator_new(clients.c[client_id].free_associations);

	// init default ass
	clients.c[client_id].default_association = xmalloc(sizeof(alpha_n_ass_t));
	if(clients.c[client_id].default_association == NULL) {
		return AP_ERR_NOMEM;
	}

	ap_err_code result;
	result = alpha_n_ass_init((alpha_n_ass_t*)clients.c[client_id].default_association, conf, 0, client_id, ASS_DIRECTION_BIDIRECTIONAL);
	if(result != AP_ERR_SUCCESS) {
		host_free(client_id);
		return result;
	}

	host_add_association(client_id, clients.c[client_id].default_association);

	clients.c[client_id].new_def_ass = NULL;


	return AP_ERR_SUCCESS;
} //]]

//! Frees the associations data structure
/*!
 * Frees the associations datastructure AND its content
 * @param[in] client_id the client whose associations should be freed
 * @return AP_ERR_STATE if any associations type is not recognized
 */
static ap_err_code host_free_associations(const uint32_t client_id) {
//[[
	assert(client_id < clients.len);
	int i;
	//from one since the default association takes 0 (and is freed later)
	for(i = 1; i < ASS_MAX_NUMBER; ++i) {
		association_free(clients.c[client_id].associations_in[i]);
		association_free(clients.c[client_id].associations_out[i]);
	}
	//free the default association
	association_free(clients.c[client_id].default_association);
	xfree(clients.c[client_id].associations_in);
	xfree(clients.c[client_id].associations_out);
	ring_buffer_iterator_free(clients.c[client_id].free_associations_it);
	ring_buffer_free(clients.c[client_id].free_associations);
	return AP_ERR_SUCCESS;
} //]]

/** Reinit the associations after restart of opposing client
 * This simply frees and inits all the association which has to happen
 * when the opposing client restarted
 * @param	client_id	the client which restarted
 */
inline ap_err_code host_reset_associations(const uint32_t client_id, const config_t *conf) {
//[[
	host_free_associations(client_id);
	host_init_associations(client_id, conf);
	return AP_ERR_SUCCESS;
} //]]

//! Getter for a association from a host
/*!
 * Gets a specific association from a host. The returned association can only be used to READ data, (e.g. is only used for incoming data).
 *
 * @param[in] client_id the host in the client array
 * @param[in] association_id the association identifier
 *
 * @return NULL if association does not exist, otherwise the association
 */
inline association_t* host_get_incoming_association(const unsigned int client_id, const unsigned int association_id) {
//[[
	assert(client_id < clients.len);
	if(association_id < ASS_MAX_NUMBER) { 
		return clients.c[client_id].associations_in[association_id];
	}
	return NULL;
	
} //]]

//! Getter for a association from a host
/*!
 * Gets a specific association from a host. The returned association can only be used to WRITE data, (e.g. is only used for outgoing data).
 *
 * @param[in] client_id the host in the client array
 * @param[in] association_id the association identifier
 *
 * @return NULL if association does not exist, otherwise the association
 */
inline association_t* host_get_outgoing_association(const unsigned int client_id, const unsigned int association_id) {
//[[
	assert(client_id < clients.len);
	if(association_id < ASS_MAX_NUMBER) { 
		return clients.c[client_id].associations_out[association_id];
	}
	return NULL;
} //]]

//! Returns the default alpha N association
/*!
 * @param[in] client_id the id of the client in the clients array
 * @return NULL if default association is not set, otherwise the association
 *
 * @note This function should rather be used for bootstrapping
 */
inline struct alpha_n_ass* host_get_default_association(const uint32_t client_id) {
//[[
	assert(client_id < clients.len);
	return (alpha_n_ass_t*)clients.c[client_id].default_association;
} //]]

//! Returns a free/ready association or NULL if no association is ready
/*!
 * @param[in] client_id the id of the client in the clients array
 * @return NULL if no association is free, otherwise a pointer to the association
 */
static association_t* host_get_free_association(const uint32_t client_id) {
//[[
	assert(client_id < clients.len);
	association_t** retval = (association_t**)ring_buffer_iterator_get(clients.c[client_id].free_associations_it);
	if(retval == NULL || *retval == NULL) {
		//try to reset
		ring_buffer_iterator_reset(clients.c[client_id].free_associations_it);
		retval = (association_t**)ring_buffer_iterator_get(clients.c[client_id].free_associations_it);
	}
	if(retval == NULL) {
		return NULL;
	} else {
		ring_buffer_iterator_next(clients.c[client_id].free_associations_it);
		return *retval;
	}
} //]]

/************************* CLIENT INIT FUNCTIONS ****************************/

//! Initializes a specific host
/*!
 * Initializes hashchains, etc. The host must
 * of course exist in the host array. Addionally
 * one ALPHA N association is added to the given
 * host.
 *
 * \note Alpha N association is added to the host
 *
 * @param[in] client_id the id of the host in the array
 * @return AP_ERR_SUCCES if ok, otherwise AP_ERR_NOMEM
 *
 * @note this function does NOT set the hosts' address
 */
ap_err_code host_init(unsigned int client_id, const config_t *conf) {
//[[
	assert(client_id < clients.len);
	memset(&clients.c[client_id], 0, sizeof(alpha_client_t));
	clients.c[client_id].delete_me = 0;
	clients.c[client_id].packet_queue = list_new(NULL);
	clients.c[client_id].last_ass_id = (rand() % (ASS_MAX_NUMBER - 1)) + 1;
	if(clients.c[client_id].packet_queue == NULL) {
		return AP_ERR_NOMEM;
	}
	//clients.c[client_id].control_ass_state = CONTROL_ASS_READY;
	ap_err_code result = host_init_associations(client_id, conf);
	clients.c[client_id].packet_timeout.tv_sec = 0;
	clients.c[client_id].packet_timeout.tv_usec = 0;
	return result;
} //]]

//! Frees a host
/*!
 * Frees hashchains, hashtree, associations etc.
 * @param[in] client_id the id of the host in the array
 */
void host_free(unsigned int client_id) {
//[[
	assert(client_id < clients.len);
	if(clients.c[client_id].public_key_file){
		xfree(clients.c[client_id].public_key_file);
	}
	DSA_free(clients.c[client_id].public_key);
	host_free_associations(client_id);
	list_free(clients.c[client_id].packet_queue);
} //]]

//! Removes and frees a outgoing association from the host
/*!
 *  @param[in] client_id the client whose association should be killed
 *  @param[in] association_id the id of the outgoing association
 *
 *  @note Please do not pass 0 as a parameter unless you know what you do
 *  (it might make sense to use 0 (e.g. the default association here) but this is not yet supported)
 */
ap_err_code host_remove_association(const unsigned int client_id, const unsigned int association_id, const unsigned int direction) {
//[[
	assert(client_id < clients.len);
	assert(association_id != 0);
	association_t *ass = NULL;

	if(direction == ASS_DIRECTION_OUTGOING) {
		ass = host_get_outgoing_association(client_id, association_id);
		if(!ass) {
			return AP_ERR_INVALID_ASS;
		}
			clients.c[client_id].associations_out[association_id] = NULL;
			ring_buffer_remove(clients.c[client_id].free_associations, (unsigned char*)&ass);
			ring_buffer_iterator_reset(clients.c[client_id].free_associations_it);
	} else {
		ass = host_get_incoming_association(client_id, association_id);
		if(!ass) {
			return AP_ERR_INVALID_ASS;
		}
		clients.c[client_id].associations_in[association_id] = NULL;
	}
	association_free(ass);

	return AP_ERR_SUCCESS;
} //]]

/************************* CLIENT LIST FUNCTIONS ****************************/

/** Find a clients id in the clients array
 * @param[in]	addr	the IP address of the client to find
 * @return	On success, the clients id is returned. If the client does
 * 		not exist, -1 is returned.
 */
inline int host_find(struct in_addr addr) {
//[[
	unsigned int i;

	for(i=0; i<clients.len; i++) {
		if(clients.c[i].addr.s_addr == addr.s_addr) {
			return i;
		}
	}

	return -1;
} //]]

/** Add a host to the clients array
 * @param[in]	name	the host name of the client (this is only used for status messages)
 * @param[in]	addr	the clients IP address
 * @return	On success, 0 is returned.
 */
int host_add(config_t *conf, const char *name, const struct in_addr addr, char* public_key_file) {
//[[
	unsigned int id = host_find(addr);

	if(id == (unsigned int)-1) {

		statusmsg("Adding host %s (%s)\n", name, inet_ntoa(addr));

		clients.c = xrealloc(clients.c, (clients.len+1) * sizeof(struct alpha_client));

		clients.len++;

		memset(&clients.c[clients.len-1], 0, sizeof(struct alpha_client));
		host_init(clients.len-1, conf);
		clients.c[clients.len-1].addr = addr;
		clients.c[clients.len-1].id = clients.len-1;
		clients.c[clients.len-1].challenge = rand();

		// try to open public key file
		char* public_key_files[2];
		public_key_files[0] = NULL;
		if(public_key_file && strlen(public_key_file) > 0) {
			public_key_files[0] = xmalloc(strlen(public_key_file)+1);
			strncpy(public_key_files[0], public_key_file, strlen(public_key_file)+1);
		}

		// malloc: /path/to + / + hostname + .pem\0
		// this will try to open "hostname.pem" in the same dir as alpha.conf, if the configured keyfile doesn't exist
		// careful! dirname may alter the content of its argument!
		char* sys_conf = xmalloc(strlen(conf->config_file)+1);
		strcpy(sys_conf, conf->config_file);

		char* sys_dir = xmalloc(strlen(dirname(sys_conf))+1);
		strcpy(sys_conf, conf->config_file);
		strcpy(sys_dir, dirname(sys_conf));

		public_key_files[1] = xmalloc(strlen(sys_dir)+1+strlen(name)+5);
		sprintf(public_key_files[1], "%s/%s.pem", sys_dir, name);

		int i;
		FILE* public_key_file_fd = NULL;
		for(i=0; i<2; i++) {

			if(!public_key_files[i]) {
				continue;
			}

			#ifdef DEBUG_PUBKEY
				statusmsg("Trying public-key-file %s\n", public_key_files[i]);
			#endif

			public_key_file_fd = fopen(public_key_files[i], "r");
			if(public_key_file_fd != NULL) {
				DSA* dsa_struct;
				dsa_struct = PEM_read_DSA_PUBKEY(public_key_file_fd, NULL, NULL, NULL);
				if(dsa_struct) {
					clients.c[clients.len-1].public_key = dsa_struct;
					#ifdef DEBUG_PUBKEY
						statusmsg("<%d> DSA pubkey with fingerprint %s\n", clients.len-1, key_fingerprint(dsa_struct, NULL));
					#endif
					fclose(public_key_file_fd);
					break;
				}
				fclose(public_key_file_fd);
			}

		}

		if(clients.c[clients.len-1].public_key == NULL) {
			print_error("No valid DSA pubkey found at any of the usual locations, assuming it's a client\n");
		}

		xfree(public_key_files[0]);
		xfree(public_key_files[1]);
		xfree(sys_conf);
		xfree(sys_dir);
		// Automatically add routes for known hosts
		if(conf->autoroute) {
			statusmsg("Adding %s to routing table.\n", name);
			struct sockaddr_in host;
			host.sin_family = AF_INET;
			host.sin_addr = addr;
			host.sin_port = htons(conf->port);
			memset(host.sin_zero, 0, 8);
			system_addhost(&host, conf);
		}

	} else {

		assert(id < clients.len);

		clients.c[id].delete_me = 0;
		statusmsg("Host %s (%s) is already known. Ignoring.\n", name, inet_ntoa(addr));

	}

	return 0;
} //]]

/** Read known_hosts file, add new hosts and delete no longer existing hosts
 ** (this gets for example executed by the SIGHUP signal handler, which is invoked
 **  by the add_host.pl and del_host.pl scripts)
 *
 * @return	On success, 0 is returned, otherwise -1.
 *
 * \TODO: TH: This function only works with side effects and globals.
 * This is really ugly! Change! (Yes we can!)
 */
int host_read_from_file(config_t *conf) {
//[[
	dictionary* config;

	if(!conf->autoroute) {
		statusmsg("WARNING: Autorouting is turned OFF. You will have to set up the routing yourself!\n");
	}

	config = app_read_config_to_dict(conf);
	if(config == NULL) {
		return -1;
	}

	// Mark all previously known clients as to be deleted
	// (this gets overwritten by host_add(), so afterwards
	// we know all hosts that are not in the known_hosts file!)
	unsigned int i;
	if(clients.c != NULL && clients.len != 0) {

		for(i=0; i<clients.len; i++) {
			statusmsg("Scanning client %d\n", i);
			clients.c[i].delete_me = 1;
		}
	}

	// find out how many [sections] our ini has
	unsigned int config_length = iniparser_getnsec(config);

	// try all the sections and expect each one to be a host
	char* section;
	for(i=0; i<config_length; i++) {
		section = iniparser_getsecname(config, i);
		// except config, which is the general config
		if(section != NULL && strncmp(section,"config", 6)) {

			struct addrinfo *result;
			struct addrinfo hints;
			memset(&hints, 0, sizeof hints);
			hints.ai_family = AF_INET;
			hints.ai_socktype = SOCK_DGRAM;

			// something went wrong. skip this host.
			if(getaddrinfo(section, NULL, &hints, &result) != 0) {
				print_error("Error resolving host `%s'. Ignoring.\n", section);
				continue;
			}

			alpha_mode_t mode;
			char hostname[100], moderequest[100];
			strncpy(hostname, section, 100);
			strncpy(moderequest, section, 100);	// section must not be changed!

			char default_mode[] = "ALPHA_N";
			char* modestring = iniparser_getstring(config, strncat(moderequest, ":mode", 5), default_mode);
			if(!strncmp("ALPHA_C", modestring, 7)) {
				mode = ALPHA_C;
			} else if(!strncmp("ALPHA_M", modestring, 7)) {
				mode = ALPHA_M;
			} else {
				mode = ALPHA_N;
			}

			strncpy(moderequest, section, 100);	// section must not be changed!
			char* public_key_file;
			char empty[] = "";
			public_key_file = iniparser_getstring(config, strncat(moderequest, ":keyfile", 8), empty);

			host_add(conf, hostname, ((struct sockaddr_in*)(result->ai_addr))->sin_addr, public_key_file);

			freeaddrinfo(result);
			// xfree(section);

		}
	}

	// Delete all hosts that are still marked as to be deleted
	for(i=0; i<clients.len; i++) {
		if(clients.c[i].delete_me == 1) {
			statusmsg("Deleting client %s\n", inet_ntoa(clients.c[i].addr));
			statusmsg("NOTE: Routing set up for this client gets NOT deleted automatically (as long as alpha is running)\n");

			// Free the clients packet-queue and other data
			host_free(i);

			unsigned int j;
			for(j=i; j+1<clients.len; j++) {
				clients.c[j] = clients.c[j+1];
			}

			clients.len--;
			clients.c = xrealloc(clients.c, clients.len * sizeof(struct alpha_client));

			if(clients.c == NULL) {
				print_error("xrealloc() failed: %s\n", strerror(errno));
				return -1;
			}
		}
	}

	iniparser_freedict(config);
	return 0;
} //]]

/** Free the memory we alloced() for saving the clients */
void host_free_all(void) {
//[[
	if(clients.c != NULL) {

		// Free the clients packet queue (and everything in it)
		unsigned int i;
		for(i=0; i<clients.len; i++) {
			statusmsg("Freeing client %d\n", i);
			host_free(i);
		}

		xfree(clients.c);
	}
} //]]

// Get pointer to the DSA pubkey structure
inline DSA* host_get_pubkey(const unsigned int client_id) {
//[[
	assert(client_id < clients.len);
	return clients.c[client_id].public_key;
} //]]

/** Wrapper function for reading clients.c[id].state
 * @param[in]	client_id	the clients id in the client array
 * @param[in]	mode		which mode's state? (0=handshake,1=sending,2=receiving)
 * @return	the clients current state
 */
inline int host_get_state(const unsigned int client_id, const int mode) {
//[[
	assert(client_id < clients.len);
	assert(mode >= 0 && mode < ASS_TRANS_MODE_COUNT);
	return association_get_state((association_t*)host_get_default_association(client_id), mode);
} //]]

/** This function writes the state to the default alpha n association
 * @param[in]	client_id	the clients id in the array
 * @param[in]	mode		which mode's state? (0=handshake,1=sending,2=receiving)
 * @param[in]	state		the new state
 *
 * \see association_set_state
 */
inline void host_set_state(const unsigned int client_id, const int mode, const int state) {
//[[
	assert(client_id < clients.len);
	assert(mode >= 0 && mode < ASS_TRANS_MODE_COUNT);
	association_set_state((association_t*)host_get_default_association(client_id), mode, state);
} //]]

/** Wrapper function for reading a clients address
 * @param[in]	client_id	the clients id in the array
 * @return	the clients address as struct in_addr
 */
inline struct in_addr host_get_addr(const unsigned int client_id) {
//[[
	assert(client_id < clients.len);
	return clients.c[client_id].addr;
} //]]

/** Wrapper function for adding elements to the list (see list.c)
 * @param[in]	client_id	the clients id in the array
 * @param[in]	data		the data to be enqueued
 * @param[in]	datalen		the length of the data (in bytes)
 * @return	On success, 0 is returned, -1 otherwise.
 */
inline int host_enqueue_packet(const unsigned client_id, unsigned char *data, const unsigned int datalen) {
//[[
	assert(client_id < clients.len);
	assert(datalen > 0);
	assert(data != NULL);
	assert(clients.c[client_id].packet_queue != NULL);
	list_push_back(clients.c[client_id].packet_queue, data, datalen);
	return 0;
} //]]

//! Enqueues a control packet
/*!
 *  Control packets are special alpha packets which are enqueued in the
 *  default/control association.
 *
 *  @param[in] client_id the id of the client
 *  @param[in] data the control packet
 *  @param[in] datalen the size of the control packet
 */
ap_err_code host_enqueue_control_packet(const unsigned int client_id, unsigned char *data, const unsigned int datalen) {
//[[
	assert(client_id < clients.len);
	assert(data != NULL);
	assert(datalen > 0);
	if(clients.c[client_id].new_def_ass == NULL) {
		return association_add_packet((association_t*)clients.c[client_id].default_association, data, datalen);
	} else {
		return association_add_packet((association_t*)clients.c[client_id].new_def_ass, data, datalen);
	}
} //]]

/** Wrapper function for getting the number of currently pending packets in the clients queue
 * @param	client_id	the clients id
 * @return	The number of packets in the clients queue is returned
 */
inline unsigned int host_get_queue_size(const unsigned int client_id) {
//[[
	assert(client_id < clients.len);
	return list_size(clients.c[client_id].packet_queue);
} //]]

/** Wrapper function for getting the timestamp of a client (used for example for detecting SYN timeouts)
 * @param[in]	client_id	the clients id
 * @return	the clients timeout value
 */
inline void host_get_timestamp(const unsigned int client_id, struct timeval* timestamp) {
//[[
	assert(client_id < clients.len);
	timestamp->tv_sec = clients.c[client_id].timestamp.tv_sec;
	timestamp->tv_usec = clients.c[client_id].timestamp.tv_usec;
} //]]

/** Wrapper function for setting the timestamp of a client (used for example for detecting SYN timeouts)
 * @param[in]	client_id	the clients id
 * @param[in]	timestamp	the timestamp which you want to set
 */
inline void host_set_timestamp(const unsigned int client_id, const unsigned int sec, const unsigned int usec) {
//[[
	assert(client_id < clients.len);
	struct timeval curtime;
	gettimeofday(&curtime, NULL);
	clients.c[client_id].timestamp.tv_sec = curtime.tv_sec + sec;
	clients.c[client_id].timestamp.tv_usec = curtime.tv_usec + usec;
	host_check_and_set_min_timeout(client_id, clients.c[client_id].timestamp.tv_sec, clients.c[client_id].timestamp.tv_usec);
} //]]

/** Wrapper function for getting the length of the clients array
 * @return	the number of known hosts
 */
inline unsigned int hosts_len(void) {
//[[
	return clients.len;
} //]]

//! Distributes packets over the free associations
/*!
 * Packets are distributed over free associations in a FIFO order.
 * If no (more) free association exists, packet are left in the
 * hosts queue.
 *
 * @param[in] client_id the id of the client in the array
 * @return AP_ERR_SUCCESS
 *
 * @see host_process_packets()
 */
ap_err_code host_distribute_packets(const unsigned int client_id) {
//[[
	assert(client_id < clients.len);
	assert(clients.c[client_id].packet_queue != NULL);
	if(list_size(clients.c[client_id].packet_queue) == 0) {
		return AP_ERR_SUCCESS;
	}
	association_t* ass = NULL;
	//size_t packet_size = 0;
	//unsigned char* packet = NULL;
	ass = host_get_free_association(client_id);
	if(ass == NULL) {
		return AP_ERR_SUCCESS;
	}
	//check packet timeout
	//check wether we use timeouts
	struct timeval curtime;
	gettimeofday(&curtime, NULL);
	if(ass_pop_packets(ass, clients.c[client_id].packet_queue) == 0) {
		//check if we use timeouts
		if(clients.c[client_id].packet_timeout.tv_sec != 0) {
			//we use timeouts
			if(curtime.tv_sec > clients.c[client_id].packet_timeout.tv_sec ||
				(curtime.tv_sec == clients.c[client_id].packet_timeout.tv_sec &&
				curtime.tv_usec > clients.c[client_id].packet_timeout.tv_usec)) {

				//timeout reached
				association_flush_queue(ass, clients.c[client_id].packet_queue);
				//reset timestamp (below)
			} else {
				//do not reset timestamp
				return AP_ERR_SUCCESS;
			}
		}

	}
	//reset timestamp
	struct timeval timeout;
	association_get_packet_timeout(ass, &timeout);
	if(timeout.tv_sec + timeout.tv_usec > 0) {
		clients.c[client_id].packet_timeout.tv_sec = curtime.tv_sec + timeout.tv_sec;
		clients.c[client_id].packet_timeout.tv_usec = curtime.tv_usec + timeout.tv_usec;
	}
	return AP_ERR_SUCCESS;
} //]]

//! Processes control packets
/*!
 *  Processes
 */
socklen_t host_process_packets_default_assocation(const config_t* conf, const unsigned int client_id) {
//[[
	assert(client_id < clients.len);
	assert(clients.c[client_id].packet_queue != NULL);
	association_t* def_ass = (association_t*)host_get_default_association(client_id);
	if(association_get_state(def_ass, ASS_TRANS_MODE_HANDSHAKE) != ASS_STATE_READY) {
		return 0;
	}

	if(association_get_state(def_ass, ASS_TRANS_MODE_SENDING) != ASS_STATE_READY) {
		return 0;
	}

	if(association_get_queue_size(def_ass) <= 0) {
		return 0;
	}

	return association_send_s1(def_ass, conf);

} //]]

//! Sends a S1 packet on all associations
/*!
 * S1 packets are sent over these associations, which allow (by their state machine) to
 * to transmit data and where the queue contains at least one element.
 *
 * @param[in] conf the configuration to be used for sending the S1 packet
 * @param[in] client_id the id of the client in the clients array
 * @note the packet is NOT removed from the queue, this is done by the state machine
 * 	because the S1 could get lost and would need to be retransmitted.
 *
 *	@see host_distribute_packets()
 */
socklen_t host_process_packets(const config_t* conf, const unsigned int client_id) {
//[[
	assert(client_id < clients.len);
	assert(clients.c[client_id].packet_queue != NULL);
	if(association_get_state((association_t*)host_get_default_association(client_id), ASS_TRANS_MODE_HANDSHAKE) != ASS_STATE_READY) {
		return 0;
	}

	association_t* ass = NULL;
	association_t** ass_ptr = NULL;

	size_t bytes_send = 0;
	size_t total_bytes_send = 0;

	host_process_packets_default_assocation(conf, client_id);

	ring_buffer_iterator_t* it = ring_buffer_iterator_new(clients.c[client_id].free_associations);
	ass_ptr = (association_t**)ring_buffer_iterator_get(it);

	while(ass_ptr != NULL) {
		ass = *ass_ptr;
		if(ass == NULL) {
			break;
		}
		if(association_get_queue_size(ass) > 0) {
			bytes_send = association_send_s1(ass, conf);
			if(bytes_send == 0) {
				ass_ptr = (association_t**)ring_buffer_iterator_next(it);
				continue;
			}
			total_bytes_send += bytes_send;

			// remove association from free association data structure
			ring_buffer_remove(clients.c[client_id].free_associations, (unsigned char*)&ass);
		}
		ass_ptr = (association_t**)ring_buffer_iterator_next(it);
	}
	ring_buffer_iterator_free(it);
	return total_bytes_send;
} //]]

//! Checks timeouts for ALL associations
/*!
 * Checks timouts for usual data transmission (S1, A1, S2). We only need to check
 * (by specification) whether we get after a while a A1 packet or not.
 *
 * @param[in] client_id the id from the client in the arrray
 * @param[in] conf the configuration used to resend the S1 packet
 *
 * @return AP_ERR_SUCCESS if all checks return AP_ERR_SUCCESS otherwise the value the last check returned
 */
ap_err_code host_handle_timeouts(const unsigned int client_id, const config_t* conf) {
//[[
	//! \TODO HANDLE TIMEOUTS FOR THE DEFAULT ASS INTERNAL STATE
	assert(client_id < clients.len);
	//only check outgoing associations as we only check timeouts for the active side (e.g. S1)
	unsigned int i;
	association_t* ass;
	ap_err_code result;
	for(i = 0; i < ASS_MAX_NUMBER; ++i) {
		ass = clients.c[client_id].associations_out[i];
		if(ass != NULL) {
			result = association_handle_timeouts(ass, conf);
			if(result != AP_ERR_SUCCESS) {
				return result;
			}
		}
	}
	return AP_ERR_SUCCESS;
} //]]

//! Marks an association as ready
/*!
 * Ready associations are stored in a specific data structure. For convience
 * please also register your association using host_add_association function.
 *
 * @param[in] client_id the id of the client in the clients array
 * @param[in] ass the association to be inserted
 *
 * @see host_distribute_packets
 */
ap_err_code host_insert_ready_association(const unsigned int client_id, association_t* ass) {
//[[
	assert(client_id < clients.len);
	assert(ass != NULL);
	// the default association is not "free" for regular packets
	if(ass->id == 0) {
		return AP_ERR_SUCCESS;
	}
	ring_buffer_insert(clients.c[client_id].free_associations, (unsigned char*)&ass);
	return AP_ERR_SUCCESS;
} //]]

//! Wrapper to handle incoming S1 packets
/*!
 * This function checks the packet and the packets' association id and forwards then
 * the function call to the specific association.
 *
 * Use the function for handling in this order: host_handle_s1_packet -> association_handle_s1 -> alpha_x_ass_handle_s1
 *
 * @param[in] client_id the id of the client, (i.e. on which client the packet came in)
 * @param[in] conf the configuration used to process this packet
 * @param[in] packet the actual ALPHA S1 packet
 * @param[in] payload_size the size of the payload in bytes (does NOT include the size of the alpha_packet_s1)
 * @param[out] valid pre-allocated space to store wheter this packet is valid
 *
 * @return Errorcode, namely AP_ERR_SUCCESS or AP_ERR_STATE
 */
ap_err_code host_handle_s1_packet(const unsigned int client_id, const config_t* conf, const alpha_packet_s1_t* packet, const size_t payload_size, bool* valid) {
//[[
	assert(client_id < clients.len);
	assert(packet != NULL);
	const uint32_t association_id = packet->association_id;
	association_t* ass = host_get_incoming_association(client_id, association_id);
	if(ass == NULL) {
		*valid = false;
		return AP_ERR_INVALID_ASS;
	}
	//if(association_get_state(ass, ASS_TRANS_MODE_RECEIVING) == ASS_STATE_READY) {
		return association_handle_s1(ass, conf, packet, payload_size, valid);
	//} else {
		//return AP_ERR_STATE;
	//}
} //]]

//! Wrapper to handle incoming S2 packets
/*!
 * This function checks the packet and the packets' association id and forwards then
 * the function call to the specific association.
 *
 * Use the function for handling in this order: host_handle_s2_packet -> association_handle_s2 -> alpha_x_ass_handle_s2
 *
 * @param[in] client_id the id of the client, (i.e. on which client the packet came in)
 * @param[in] packet the actual ALPHA S2 packet
 * @param[in] payload the payload of the alpha S2 packet (does NOT include the alpha S2 header) this might include some verfication data (e.g. branches from alpha M)
 * @param[in] payload_len the size of the payload
 * @param[out] valid pre-allocated space to store wheter this packet is valid
 * @param[out] real_payload A pointer to the actual payload (which should be forwarded if this packet is valid)
 * @param[out] real_payload_size Size of the real payload
 * @return Errorcode, namely AP_ERR_SUCCESS or AP_ERR_STATE
 */
ap_err_code host_handle_s2_packet(const unsigned int client_id, const config_t *conf, const alpha_packet_s2_t* packet, unsigned char* payload, const size_t payload_len,
	bool* const valid, unsigned char** real_payload, size_t* const real_payload_size) {
//[[
	assert(client_id < clients.len);
	assert(packet != NULL);
	assert(payload != NULL);
	assert(payload_len > 0);
	assert(valid != NULL);
	const uint32_t association_id = packet->association_id;
	association_t* ass = host_get_incoming_association(client_id, association_id);
	if(ass == NULL) {
		*valid = false;
		return AP_ERR_INVALID_ASS;
	}
	if(association_get_mode(ass) == ALPHA_Z || association_get_state(ass, ASS_TRANS_MODE_RECEIVING) == ASS_STATE_SENT_A1_WAIT_S2) {
		return association_handle_s2(ass, conf, packet, payload, payload_len, valid, real_payload, real_payload_size);
	} else {
		return AP_ERR_STATE;
	}
} //]]

//! Wrapper to handle incoming A1 packets
/*!
 * This function checks the packet and the packets' association id and forwards then
 * the function call to the specific association.
 *
 * Use the function for handling in this order: host_handle_a1_packet -> association_handle_a1
 *
 * @param[in] client_id the id of the client, (i.e. on which client the packet came in)
 * @param[in] conf the configuration used to process this packet
 * @param[in] packet the actual ALPHA A1 packet
 * @param[out] valid pre-allocated space to store wheter this packet is valid
 *
 * @return Errorcode, namely AP_ERR_SUCCESS or AP_ERR_STATE
 */
ap_err_code host_handle_a1_packet(const unsigned int client_id, const config_t* conf, const alpha_packet_a1_t* packet, bool* valid) {
//[[
	assert(client_id < clients.len);
	assert(packet != NULL);

	const uint32_t association_id = packet->association_id;
	association_t* ass = host_get_outgoing_association(client_id, association_id);
	if(ass == NULL) {
		*valid = false;
		return AP_ERR_INVALID_ASS;
	}
	if(association_get_state(ass, ASS_TRANS_MODE_SENDING) == ASS_STATE_SENT_S1_WAIT_A1) {
		return association_handle_a1(ass, conf, packet, valid);
	} else {
		return AP_ERR_STATE;
	}
} //]]

//! Returns a new association ID
/*!
 * @param[in] client_id the client, in the clients array
 * @return Returns a new association ID based the NUMBER OF CURRENTLY RUNNING associations
 */
inline size_t host_get_new_ass_id(const unsigned int client_id) {
//[[
	assert(client_id < clients.len);
	//dont give the 0 as a association id (it is reservered for the default association);
	clients.c[client_id].last_ass_id = ((clients.c[client_id].last_ass_id) % (ASS_MAX_NUMBER - 1)) + 1;
	return clients.c[client_id].last_ass_id;
} //]]

//! Add a association to a host
/*!
 * Add a already created association to a specific host. The association is
 * inserted into the associations data structure
 *
 * @param[in] client_id the client to which the association should be added
 * @param[in] association the association which should be added to the host
 *
 * @note If the association with the id and the direction (outgoing/incoming) already exist its overwritten!
 * @return AP_ERR_SUCCESS
 */
ap_err_code host_add_association(const uint32_t client_id, association_t* ass) {
//[[
	assert(client_id < clients.len);
	assert(ass != NULL);
	const unsigned int id = association_get_id(ass);
	// NO BIDIRECTIONAL ASSS except the control association!
	assert((id == 0 && ass->direction == ASS_DIRECTION_BIDIRECTIONAL)
		|| (id != 0 && ass->direction != ASS_DIRECTION_BIDIRECTIONAL));
	switch(ass->direction) {
		case ASS_DIRECTION_INCOMING:
			clients.c[client_id].associations_in[id] = ass;
			break;
		case ASS_DIRECTION_OUTGOING:
			clients.c[client_id].associations_out[id] = ass;
			break;
		case ASS_DIRECTION_BIDIRECTIONAL:
			clients.c[client_id].associations_in[id] = ass;
			clients.c[client_id].associations_out[id] = ass;
			break;
		default:
			return AP_ERR_STATE;
	}
	return AP_ERR_SUCCESS;
} //]]

//! Gets the new default association
/*!
 * This method is used to agree on a new default association.
 * The "handshake" for new default association requires to run 2 default associations in
 * parallel for a short time. This function returns the one on that will be agreed.
 *
 * @param[in] client_id the id of the client
 *
 * @see host_create_new_default_association()
 */
inline alpha_n_ass_t* host_get_new_default_association(const unsigned int client_id) {
//[[
	assert(client_id < clients.len);
	return clients.c[client_id].new_def_ass;
} //]]

//! Generates a new default association
/*!
 * This function creates and mallocs a new default association. This association
 * is only initializes but the opposing client has yet not agreed on this association,
 * therefore some kind of handshake needs to be applied.
 *
 * @param[in] client_id
 *
 * @see host_update_default_association(), host_get_new_default_association()
 */
alpha_n_ass_t* host_create_new_default_association(const unsigned int client_id, const config_t *conf) {
//[[
	assert(client_id < clients.len);
	if(clients.c[client_id].new_def_ass != NULL) {
		association_free((association_t*)clients.c[client_id].new_def_ass);
	}
	clients.c[client_id].new_def_ass = malloc(sizeof(alpha_n_ass_t));
	alpha_n_ass_init(clients.c[client_id].new_def_ass, conf, 0, client_id, ASS_DIRECTION_BIDIRECTIONAL);
	return clients.c[client_id].new_def_ass;
} //]]

//! Updates the default association
/*!
 * When agreeing on a new association, two instances of a default association run in
 * parallel for a short time. This function sets the second default association,
 * that has been initialized with host_create_new_default_association(), to
 * the current default association and frees the previous default association.
 *
 * @param[in] client_id the id of the client
 *
 * @see host_create_new_default_association()
 */
ap_err_code host_update_default_association(const unsigned int client_id) {
//[[
	assert(client_id < clients.len);
	//remove the old and replace it with the new default association
	association_t* old_ass = clients.c[client_id].default_association;
	association_t* new_ass = (association_t*)host_get_new_default_association(client_id);
	association_move_packet_queue(new_ass, old_ass);
	clients.c[client_id].associations_out[0] = new_ass;
	clients.c[client_id].associations_in[0] = new_ass;
	clients.c[client_id].default_association = new_ass;
	association_free(old_ass);
	clients.c[client_id].new_def_ass = NULL;
	return AP_ERR_SUCCESS;
} //]]

/************************* CLIENT HASH FUNCTIONS ****************************/

/** Wrapper to set a new anchor for a chain
 * @param[in]	client_id	the clients id
 * @param[in]	chain		which chain to use
 * @param[in]	anchor		the new anchor
 */
void host_new_anchor(const unsigned int client_id, const int chain, const unsigned char* anchor) {
//[[
	assert(client_id < clients.len);
	assert(chain == CHAIN_SIGN || chain == CHAIN_ACK);
	alpha_n_ass_t* ass = host_get_default_association(client_id);

	if(chain == CHAIN_SIGN) {
		ring_buffer_read(ass->sign_anchors);
		association_add_sign_element((association_t*)ass, anchor);
	} else {
		ring_buffer_read(ass->ack_anchors);
		association_add_ack_element((association_t*)ass, anchor);
	}
} //]]

/** Wrapper to get the the anchor from the default ALPHA N association to send to the opposing client
 * @param[in]	client_id	the clients id
 * @param[in]	chain		which chain to use
 * @param[out]	digest		place to store the anchor
 * @return	pointer to digest
 *
 * @note You have to allocate memory for the result (digest) yourself!
 * @warning This is a workaround so that alpha can be bootstrapped! Do not use this function for other things
 */
unsigned char* host_generate_anchor(const unsigned int client_id, const int chain, unsigned char* digest) {
//[[
	assert(client_id < clients.len);
	assert(chain == CHAIN_SIGN || chain == CHAIN_ACK);
	alpha_n_ass_t* ass = host_get_default_association(client_id);

	switch(chain) {
		case CHAIN_SIGN:
			memcpy(digest, hchain_current(ass->sign_hash_chain), HASHSIZE);
			break;
		case CHAIN_ACK:
			memcpy(digest, hchain_current(ass->ack_hash_chain), HASHSIZE);
			break;
	}
	return digest;

} //]]

/** Decrement the number of rounds to use when hashing our secret for the opposing client
 * @param[in]	client_id	the clients id
 * @param[in]	chain		which chain-count to decrement
 */
void host_dec_hash_rounds(const unsigned int client_id, const int chain) {
//[[
	assert(client_id < clients.len);
	assert(chain == CHAIN_SIGN || chain == CHAIN_ACK);

	alpha_n_ass_t* ass = host_get_default_association(client_id);

	switch(chain) {
		case CHAIN_SIGN :
			hchain_pop(ass->sign_hash_chain);
			break;
		case CHAIN_ACK :
			hchain_pop(ass->ack_hash_chain);
			break;
	}

} //]]

//! Initializes new chains for the default association
/*!
 * This function initializes chains for the default association only!
 * Do not confuse this function with association_init_new_chains()
 *
 * @param[in] client_id the id of the host/client in the clients array
 * @return 0 if ok, -1 if a error occured
 * @pre initialized host datastructure and initialized default association
 */
int host_init_new_chains(const unsigned int client_id, const config_t *conf) {
//[[
	if(association_init_new_chains((association_t*)host_get_default_association(client_id), conf, ASS_DIRECTION_BIDIRECTIONAL) == AP_ERR_SUCCESS) {
		return 0;
	}
	return -1;
} //]]
//! This functions sets the minimal timeout
/*!
 * Packets may only be allowed to be stored for a certain amount of time in
 * the host queue. To find the minimum timeout, i.e. to know when something
 * needs to be done, this function stores the given timeout if necessary
 * (i.e. if it is minimal).
 *
 * @param[in] client_id the host
 * @param[in] sec the number of seconds of the timeout
 * @param[in] usec the number of milliseconds of the timeout
 *
 * @remarks You DO NOT need to add the current time to the timeout
 */
void host_set_packet_timeout(const unsigned int client_id, const unsigned int sec, const unsigned int usec) {
//[[
	assert(client_id < clients.len);
	struct timeval curtime;
	gettimeofday(&curtime, NULL);
	clients.c[client_id].packet_timeout.tv_sec = curtime.tv_sec + sec;
	clients.c[client_id].packet_timeout.tv_usec = curtime.tv_usec + usec;
	host_check_and_set_min_timeout(client_id, clients.c[client_id].packet_timeout.tv_sec, clients.c[client_id].packet_timeout.tv_usec);
} //]]

//! Checks the minimum timeout FOR ALL timeouts.
/*!
 * To find the minimum timeout, i.e. to know when something
 * needs to be done, this function stores the given timeout if necessary
 * (i.e. if it is minimal).
 *
 * @param[in] client_id the host
 * @param[in] sec the number of seconds of the timeout
 * @param[in] usec the number of milliseconds of the timeout
 */
void host_check_and_set_min_timeout(const unsigned int client_id, const unsigned int sec, const unsigned int usec) {
//[[
	assert(client_id < clients.len);
	assert(clients.c[client_id].min_timeout.tv_usec >= 0 || clients.c[client_id].min_timeout.tv_sec >= 0);
	if( (sec < (unsigned int)clients.c[client_id].min_timeout.tv_sec) ||
		(sec == (unsigned int)clients.c[client_id].min_timeout.tv_sec &&
		usec < (unsigned int)clients.c[client_id].min_timeout.tv_usec)) {

		// update
		clients.c[client_id].min_timeout.tv_sec = sec;
		clients.c[client_id].min_timeout.tv_usec = usec;
	}
} //]]

//! Checks if the minimal timeout FOR ALL timeouts
/*!
 * @param[in] client_id the host that should be checked
 * @see host_check_and_set_min_timeout()
 */
inline bool host_check_min_timeout_exceed(const unsigned int client_id) {
//[[
	assert(client_id < clients.len);
	return (timemanager_timeout_exceeded(clients.c[client_id].min_timeout));
} //]]

/************************* HOST HANDSHAKE HELPERS ****************************/

// Save the address hash from the RETURN_CONNECT
inline void host_set_addr_hash(const unsigned int client_id, unsigned char *addr_hash) {
//[[
	assert(client_id < clients.len);
	memcpy(clients.c[client_id].addr_hash, addr_hash, HASHSIZE);
} //]]

// Get the address hash we saved from the RETURN_CONNECT
inline unsigned char* host_get_addr_hash(const unsigned int client_id) {
//[[
	assert(client_id < clients.len);
	return clients.c[client_id].addr_hash;
} //]]

/* @brief Wrapper to get the anchors of the opposing client for returning them */
inline void host_prepare_ack_packet(const unsigned int client_id, unsigned char *return_sign_anchor, unsigned char *return_ack_anchor) {
//[[
	assert(client_id < clients.len);

	alpha_n_ass_t* ass = host_get_default_association(client_id);
	memcpy(return_sign_anchor, ring_buffer_const_read(ass->sign_anchors), HASHSIZE);
	memcpy(return_ack_anchor, ring_buffer_const_read(ass->ack_anchors), HASHSIZE);
} //]]

/* @brief Debug function to work with the challenge-reponse */
void host_print_challenge_reponse(const challenge_response_t *cr) {
//[[
	char sa_buf[HASHSIZE*2+1], aa_buf[HASHSIZE*2+1], rsa_buf[HASHSIZE*2+1], raa_buf[HASHSIZE*2+1];
	statusmsg("<?> CR: %u | %.*s... | %.*s... | %.*s... | %.*s...\n", cr->challenge,
		DIGEST_PRINTLEN, digeststr(cr->sign_anchor, sa_buf),
		DIGEST_PRINTLEN, digeststr(cr->ack_anchor, aa_buf),
		DIGEST_PRINTLEN, digeststr(cr->return_sign_anchor, rsa_buf),
		DIGEST_PRINTLEN, digeststr(cr->return_ack_anchor, raa_buf)
	);

} //]]

// Sign the challenge the host sent us with his SYN
unsigned char* host_sign_ack(const config_t* conf, unsigned char* message, size_t message_size, unsigned char* signature) {
//[[
	if (conf->private_key != NULL) {
		unsigned int siglen;
		if(!DSA_sign(0, message, message_size, signature, &siglen, conf->private_key)) {
			print_error("<?> There was an error signing an ACK packet!\n");
		}
		#ifdef DEBUG_PUBKEY
		//statusmsg("<?> Signing ACK packet, siglen was %d, DSA size is %d\n", siglen, DSA_size(conf->private_key));
		#endif
		return signature;
	}

	return NULL;
} //]]

/* @brief recreate the challenge_response, hash it and then verify the signature of the hashed block */
bool host_verify_ack(const unsigned int client_id, const alpha_packet_ack_t *packet) {
//[[
	challenge_response_t challenge_response;

	challenge_response.challenge = htonl(host_get_challenge(client_id));
	memcpy(challenge_response.sign_anchor, packet->sign_anchor, HASHSIZE);
	memcpy(challenge_response.ack_anchor, packet->ack_anchor, HASHSIZE);

	association_t *ass = (association_t*)host_get_default_association(client_id);
	memcpy(challenge_response.return_sign_anchor, hchain_current(ass->sign_hash_chain), HASHSIZE);
	memcpy(challenge_response.return_ack_anchor, hchain_current(ass->ack_hash_chain), HASHSIZE);

	unsigned char challenge_response_sig[HASHSIZE];
	create_digest((unsigned char *) &challenge_response, sizeof(challenge_response), challenge_response_sig);

	#ifdef DEBUG_DIGEST //[[
	char ackbuf[2*HASHSIZE+1];
	statusmsg("<%d> Digest of challenge_response is %.*s...\n", client_id, DIGEST_PRINTLEN, digeststr(challenge_response_sig, ackbuf));
	#endif //]]

	return host_verify_sig(client_id, (unsigned char *) &challenge_response_sig, sizeof(challenge_response_sig), (unsigned char *) &(packet->challenge_response));
} //]]

/* @brief verify a signature of some chunk of data using DSA and the public key of the client */
bool host_verify_sig(const unsigned int client_id, const unsigned char* message, const size_t message_size, unsigned char* signature) {
//[[
	unsigned int verify_success;
	verify_success = DSA_verify(0, message, message_size, signature, SIGSIZE, clients.c[client_id].public_key);
		if(verify_success == (unsigned int)-1) {
			#ifdef DEBUG_PUBKEY
			print_error("<%d> Error verifying signature\n", client_id);
			#endif
			return false;
		} else if(verify_success == 0) {
			#ifdef DEBUG_PUBKEY
			statusmsg("<%d> Wrong signature\n", client_id);
			#endif
			return false;
		};
	return true;
} //]]
