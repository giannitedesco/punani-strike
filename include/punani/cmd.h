/*
 * This file is part of punani strike
 * Copyright (c) 2012 Steven Dickinson
 * Released under the terms of the GNU GPL version 2
 */
#ifndef _PS_CMD_H_
#define _PS_CMD_H_

#include <stddef.h>

typedef void(cmd_parse_callback_t)(size_t paramc, char **paramv);

void cmd_parse(const char *cmd, size_t max_len);

#endif

