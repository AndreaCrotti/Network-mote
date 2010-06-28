// vim:encoding=latin1:foldmethod=marker:foldmarker=//[[,//]]
/*
 * Copyright (c) 2009, Distributed Systems Group, RWTH Aachen
 * All rights reserved.
 */

/*!
 * @file application.c
 * Implementation of application.h functions.
 *	@see application.h
 */

#include <errno.h>
#include <inttypes.h>
#include <openssl/pem.h>

#include "packet.h"
#include "application.h"
#include "tools.h"
#include "xmalloc.h"
#include "key.h"
#include "lib/iniparser.h"
#include "system.h"

// Read the config into the conf variables 
int app_read_config(config_t* conf, bool override_autoroute) {
//[[
	dictionary* config;
	config = app_read_config_to_dict(conf);
	if(config == NULL) {
		print_error("Could not find config...\n");
		return -1;
	}

	char key[] = "config";
	if(!iniparser_find_entry(config, key)) {
		print_error("Could not find [config] in config-file\n");
		return -1;
	}

	int port = iniparser_getint(config, "config:port", -1);
	unsigned int mtu = iniparser_getint(config, "config:mtu", -1);
	char empty[] = "";
	char* keyfile = iniparser_getstring(config, "config:keyfile", empty);
	char alphalog[] = "alpha.log";
	char* log_file = iniparser_getstring(config, "config:logfile", alphalog);
	if(!(conf->num_alpha_n + conf->num_alpha_z + conf->num_alpha_m + conf->num_alpha_c)) {
		conf->num_alpha_n = iniparser_getint(config, "config:init_alpha_n", 1);
		conf->num_alpha_c = iniparser_getint(config, "config:init_alpha_c", 0);
		conf->num_alpha_m = iniparser_getint(config, "config:init_alpha_m", 0);
		conf->num_alpha_z = iniparser_getint(config, "config:init_alpha_z", 0);
	}

	if(!(conf->num_alpha_n + conf->num_alpha_z + conf->num_alpha_m + conf->num_alpha_c)) {
		conf->num_alpha_n = 1;
	}

	conf->alpha_m_sec_mode = iniparser_getint(config, "config:alpha_m_sec_mode", 1);

	if(!conf->hchain_length) {
		conf->hchain_length = iniparser_getint(config, "config:hashchain_length", 10000);
	}

	if(conf->daemon) {
		errno=0;
		freopen(log_file, "a", stdout);
		freopen(log_file, "a", stderr);
		setbuf(stdout, NULL);
		setbuf(stderr, NULL);
		// this is the first spot we should try to output stuff
		if(errno) {
			freopen(DEFAULT_LOGFILE, "a", stdout);
			freopen(DEFAULT_LOGFILE, "a", stderr);
			print_error("Logging to %s failed: %s\n", log_file, strerror(errno));
		}
	}
	puts("");
	statusmsg("------- ALPHA %s -------\n", VERSION);


	// now try to load the private keyfile
	FILE* private_key_file_fd;
	private_key_file_fd = fopen(keyfile, "r");
	if(private_key_file_fd != NULL) {
		DSA* dsa_struct = NULL;
		dsa_struct = PEM_read_DSAPrivateKey(private_key_file_fd, NULL, NULL, NULL);
		if(dsa_struct != NULL) {
			conf->private_key = dsa_struct;
			#ifdef DEBUG_PUBKEY
				statusmsg("Private key with fingerprint %s\n", key_fingerprint(dsa_struct, NULL));
			#endif
		}
	}
	fclose(private_key_file_fd);


	if(port > 0) {
		if(conf->port > 0 && port != conf->port) {
			statusmsg("Port number changed from %d to %d\n", conf->port, port);
		}
		conf->port = (unsigned short) port;
	} else {
		conf->port = PORT;
	}

	if(mtu > 0) {
		if(mtu != conf->mtu) {
			statusmsg("MTU changed from %d to %d, changing interface\n", conf->mtu, mtu);
			if(conf->socket_outgoing) {
				conf->mtu = mtu;
				system_set_mtu(conf->mtu);
			}
		}
		conf->mtu = mtu;
	} else {
		conf->mtu = TUN_MAX_PACKET_SIZE;
		print_error("No mtu found in config file. Using default mtu %d\n", conf->mtu);
	}

	if(!override_autoroute) {
		conf->autoroute = iniparser_getboolean(config, "config:autoroute", 1);
	}

	statusmsg("Read config-file %s successfully\n", conf->config_file);
	iniparser_freedict(config);
	return 0;
} //]]

// Try different locations to find a config file and read it into a dict
dictionary* app_read_config_to_dict(config_t* conf) {
//[[
	dictionary* config = NULL;

	// We will use three possible locations for the config files
	char* config_files[3];

	// Custom location (if given)
	if(conf->config_file) {
		config_files[0] = xmalloc(strlen(conf->config_file)+1);
		strcpy(config_files[0], conf->config_file);
	} else {
		config_files[0] = NULL;
	}

	// Current directory
	config_files[1] = xmalloc(strlen(CONFIG)+1);
	strcpy(config_files[1], CONFIG);

	// sytem-wide config
	config_files[2] = xmalloc(strlen(CONFIGPATH) + 1 + strlen(CONFIG) + 1);
	sprintf(config_files[2], "%s/%s", CONFIGPATH, CONFIG);

	int i;
	for(i=0; i<3; i++) {
		if(config_files[i] == NULL) continue;
		// statusmsg("Reading configuration file: `%s'\n", config_files[i]);
		if((config = iniparser_load(config_files[i])) != NULL) {
			conf->config_file = xrealloc(conf->config_file, strlen(config_files[i])+1);
			strcpy(conf->config_file, config_files[i]);
			break;
		}
		print_error("Failed to open %s: %s\n", config_files[i], strerror(errno));
	}

	if(config_files[0]) xfree(config_files[0]);
	xfree(config_files[1]);
	xfree(config_files[2]);

	if(config == NULL) {
		print_error("No suitable config found\n");
		return NULL;
	}

	return config;

} //]]
