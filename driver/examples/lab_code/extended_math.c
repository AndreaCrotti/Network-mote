// vim:encoding=latin1:foldmethod=marker:foldmarker=//[[,//]]
/*
 * Copyright (c) 2009, Distributed Systems Group, RWTH Aachen
 * All rights reserved.
 */

/*!
 * @file extended_math.c See extended_math.h for documentation
 * @author Christian Dernehl <christian.dernehl@rwth-aachen.de>
 * @date April 2009
 */

#include "extended_math.h"

//! Calculates the Log_2 of a given variable x
/*!
 * Performs a binary search on the binary representation of
 * x, thus calculating basically floor(log(x)/log(2)) but
 * more efficient. The complexity of this function is
 * O(log(n)) where n is the number of bits in a given
 * Integer.
 *
 * @param x a unsigned integer
 */
INT_OUT log_2(INT x) {
//[[

	// this is the shift which we are currently performing, its decremented by half in each step
	// x bytes = 8 * y bits we need half, so shift = x * 8 / 2 = x * 4 = x << 2
	register INT shift = sizeof(INT) << 2;

	// this is our current value which we compare to our input
	register INT mask = 1 << shift;

	// this is the total shift and thus log_2(x) in the end
	register INT total_shift = shift;

	while (mask != x) {
		shift = shift >> 1; // shift /= 2;

		if(shift == 0) {
			if(mask > x)
				total_shift--;
			return total_shift;
		}

		if(mask > x) {
			// search on the right side
			mask = mask >> shift;
			total_shift -= shift;
		} else {
			// search on the left side
			mask = mask << shift;
			total_shift += shift;
		}

	}

	return total_shift;

} //]]

inline INT_OUT exp_2(INT x) {
//[[
	return 1 << x;
} //]]
