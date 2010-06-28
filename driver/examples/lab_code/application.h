// vim:encoding=latin1:foldmethod=marker:foldmarker=//[[,//]]
/*
 * Copyright (c) 2009, Distributed Systems Group, RWTH Aachen
 * All rights reserved.
 */

/*!
 * @file application.h
 * @brief This module stores and manages necessary application data, such as the current ALPHA running mode.
 *
 * @author Christian Dernehl <christian.dernehl@rwth-aachen.de>
 */

#ifndef __APPLICATION_H_
#define __APPLICATION_H_

#include "alpha.h"
#include "lib/iniparser.h"

int app_read_config(config_t* conf, bool override_autoroute);
dictionary* app_read_config_to_dict(config_t* conf);

#endif // __APPLICATION_H_
