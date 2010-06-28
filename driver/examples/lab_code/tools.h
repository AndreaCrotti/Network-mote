// vim:encoding=latin1:foldmethod=marker:foldmarker=//[[,//]]
/*
 * Copyright (c) 2009, Distributed Systems Group, RWTH Aachen
 * All rights reserved.
 */

#ifndef __TOOLS_H__
#define __TOOLS_H__

/** Wrapper function for printing error messages (use makros in tools.h!)
 * @param	file	file name in which the error occured
 * @param	line	line number the error occured in
 * @param	format	format string of the error message and its arguments (printf syntax)
 */
#define print_error_prefix(a...) _print_error(__FILE__, __LINE__, ## a)
#define print_error_noprefix(a...) _print_error(0,0, ##a)
#define print_error print_error_prefix

/* placeholder for ... nothing */
#define NULLFUNC() 1!=1
/* match context and level */
#define AP_MSG_P(level, context, format) (level <= AP_MSG_LVL && (context == AP_MSG_CTX || AP_MSG_CTX == AP_MSG_CTX_ALL)) ? statusmsg(format) : NULLFUNC()
#define AP_MSG_F(level, context, format, ...) (level <= AP_MSG_LVL && (context == AP_MSG_CTX || AP_MSG_CTX == AP_MSG_CTX_ALL)) ? statusmsg(format, __VA_ARGS__) : NULLFUNC()

void _print_error(const char *file, const unsigned int line, const char *format, ...);
void print_usage(const char *me);
void statusmsg(const char *format, ...);

#endif // __TOOLS_H__
