// vim:encoding=latin1:foldmethod=marker:foldmarker=//[[,//]]
/*
 * Copyright (c) 2009, Distributed Systems Group, RWTH Aachen
 * All rights reserved.
 */

#ifndef __KEY_H
#define __KEY_H

#include <openssl/dsa.h>

char* key_fingerprint(const DSA *key, char *output);

#endif // __KEY_H
