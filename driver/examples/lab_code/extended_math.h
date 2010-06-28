// vim:encoding=latin1:foldmethod=marker:foldmarker=//[[,//]]
/*
 * Copyright (c) 2009, Distributed Systems Group, RWTH Aachen
 * All rights reserved.
 */

/*!
 * @file extended_math.h Module for different calculations
 *
 * @author Christian Dernehl <christian.dernehl@rwth-aachen.de>
 * @date April 2009
 *
 * This module offers functions for mathematical calculations. Aim
 * of this module is to offer very efficient but also specific
 * functions.
 */

#ifndef __EXTENDED_MATH_H__
#define __EXTENDED_MATH_H__

#include <inttypes.h>

//! The Integer input type used in log_2()
typedef uint32_t INT;
//! The Integer output type used in log_2(). Needs to be at least log_2(INT) in size
typedef uint32_t INT_OUT;

INT_OUT log_2(INT);

INT_OUT exp_2(INT);

#endif
