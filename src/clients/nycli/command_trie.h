/*  XMMS2 - X Music Multiplexer System
 *  Copyright (C) 2003-2007 XMMS2 Team
 *
 *  PLUGINS ARE NOT CONSIDERED TO BE DERIVED WORK !!!
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 */

#ifndef __COMMAND_TRIE_H__
#define __COMMAND_TRIE_H__

#include <xmmsclient/xmmsclient.h>

#include <glib.h>

#include "main.h"

command_trie_t* command_trie_alloc ();
command_trie_t* command_trie_new (gchar c);
void command_trie_free (command_trie_t *trie);
void command_trie_fill (command_trie_t* trie, command_setup_func commandlist[]);
gboolean command_trie_insert (command_trie_t* trie, const gchar *string, command_exec_func cmd, gboolean needconn, const argument_t flags[]);
command_action_t* command_trie_find_leaf_action (command_trie_t *trie);
command_action_t* command_trie_find (command_trie_t *trie, const gchar *input);

#endif /* __COMMAND_TRIE_H__ */