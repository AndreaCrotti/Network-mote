// vim:encoding=latin1:foldmethod=marker:foldmarker=//[[,//]]
/*
 * Copyright (c) 2009, Distributed Systems Group, RWTH Aachen
 * All rights reserved.
 */

#ifndef __CONTROL_H__
#define __CONTROL_H__

ap_err_code control_send_version(const config_t *conf, struct sockaddr_un *to, socklen_t tolen);
ap_err_code control_send_clientlist(const config_t *conf, struct sockaddr_un *to, socklen_t tolen);

#endif // __CONTROL_H__
