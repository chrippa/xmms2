/*  XMMS2 - X Music Multiplexer System
 *  Copyright (C) 2003	Peter Alm, Tobias Rundstr�m, Anders Gustafsson
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




#ifndef _XMMS_OUTPUT_H_
#define _XMMS_OUTPUT_H_

#include <glib.h>


#ifdef XMMS_OS_LINUX /* ALSA might be default now a days? */
#define XMMS_OUTPUT_DEFAULT "oss"
#elif XMMS_OS_OPENBSD
#define XMMS_OUTPUT_DEFAULT "sun"
#elif XMMS_OS_SOLARIS
#define XMMS_OUTPUT_DEFAULT "sun"
#elif XMMS_OS_DARWIN
#define XMMS_OUTPUT_DEFAULT "coreaudio"
#endif

#define XMMS_OUTPUT_DEFAULT_BUFFERSIZE "131072"

/*
 * Type definitions
 */

typedef struct xmms_output_St xmms_output_t;

#include "xmms/config.h"
#include "xmms/plugin.h"

typedef enum {
	XMMS_OUTPUT_STATUS_PLAY,
	XMMS_OUTPUT_STATUS_STOP,
	XMMS_OUTPUT_STATUS_PAUSE,
} xmms_output_status_t; /** @todo RENAME */

/*
 * Output plugin methods
 */

typedef void (*xmms_output_status_method_t) (xmms_output_t *output, xmms_output_status_t status);
typedef void (*xmms_output_write_method_t) (xmms_output_t *output, gchar *buffer, gint len);
typedef void (*xmms_output_destroy_method_t) (xmms_output_t *output);
typedef gboolean (*xmms_output_open_method_t) (xmms_output_t *output);
typedef gboolean (*xmms_output_new_method_t) (xmms_output_t *output);
typedef gboolean (*xmms_output_mixer_get_method_t) (xmms_output_t *output, gint *left, gint *right);
typedef gboolean (*xmms_output_mixer_set_method_t) (xmms_output_t *output, gint left, gint right);
typedef void (*xmms_output_flush_method_t) (xmms_output_t *output);
typedef void (*xmms_output_close_method_t) (xmms_output_t *output);
typedef guint (*xmms_output_samplerate_set_method_t) (xmms_output_t *output, guint rate);
typedef guint (*xmms_output_buffersize_get_method_t) (xmms_output_t *output);

/*
 * Public function prototypes
 */

xmms_plugin_t *xmms_output_plugin_get (xmms_output_t *output);
gpointer xmms_output_private_data_get (xmms_output_t *output);
void xmms_output_private_data_set (xmms_output_t *output, gpointer data);

gboolean xmms_output_volume_get (xmms_output_t *output, gint *left, gint *right);

void xmms_output_flush (xmms_output_t *output);
gint xmms_output_read (xmms_output_t *output, char *buffer, gint len);

GList *xmms_output_stats (xmms_output_t *output, GList *list); 

#endif
