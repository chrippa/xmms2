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

#include "udata_packs.h"


struct pack_infos_playlist_St {
	cli_infos_t *infos;
	gchar *playlist;     /* owned by this struct */
};


/** Pack cli_infos_t and playlist name in a struct, to be used as udata. */
pack_infos_playlist_t *
pack_infos_playlist (cli_infos_t *infos, gchar *playlist)
{
	pack_infos_playlist_t *pack = g_new0 (pack_infos_playlist_t, 1);
	pack->infos = infos;
	pack->playlist = g_strdup (playlist);
	return pack;
}

/** Extract infos and playlist from the pack. The pack must still be freed. */
void
unpack_infos_playlist (pack_infos_playlist_t *pack, cli_infos_t **infos,
                       gchar **playlist)
{
	*infos = pack->infos;
	*playlist = pack->playlist;
}

/** Free the pack memory. */
void
free_infos_playlist (pack_infos_playlist_t *pack)
{
	g_free (pack->playlist);
	g_free (pack);
}


struct pack_infos_playlist_pos_St {
	cli_infos_t *infos;
	gchar *playlist;     /* owned by this struct */
	gint pos;
};

/** Pack cli_infos_t, playlist name and pos in a struct, to be used as udata. */
pack_infos_playlist_pos_t *
pack_infos_playlist_pos (cli_infos_t *infos, gchar *playlist, gint pos)
{
	pack_infos_playlist_pos_t *pack = g_new0 (pack_infos_playlist_pos_t, 1);
	pack->infos = infos;
	pack->playlist = g_strdup (playlist);
	pack->pos = pos;
	return pack;
}

/** Extract infos, playlist and pos from the pack. The pack must still be freed. */
void
unpack_infos_playlist_pos (pack_infos_playlist_pos_t *pack, cli_infos_t **infos,
                           gchar **playlist, gint *pos)
{
	*infos = pack->infos;
	*playlist = pack->playlist;
	*pos = pack->pos;
}

/** Free the pack memory. */
void
free_infos_playlist_pos (pack_infos_playlist_pos_t *pack)
{
	g_free (pack->playlist);
	g_free (pack);
}


struct pack_infos_playlist_config_St {
	cli_infos_t *infos;
	gchar *playlist;     /* owned by this struct */
	gint history;
	gint upcoming;
	xmmsc_coll_type_t type;
	gchar *input;        /* owned by this struct */
};

/** Pack cli_infos_t & the rest in a struct, to be used as udata. */
pack_infos_playlist_config_t *
pack_infos_playlist_config (cli_infos_t *infos, gchar *playlist, gint history,
                            gint upcoming, xmmsc_coll_type_t type, gchar *input)
{
	pack_infos_playlist_config_t *pack = g_new0 (pack_infos_playlist_config_t, 1);
	pack->infos = infos;
	pack->playlist = g_strdup (playlist);
	pack->history = history;
	pack->upcoming = upcoming;
	pack->type = type;
	pack->input = g_strdup (input);
	return pack;
}

/** Extract infos & the rest from the pack. The pack must still be freed. */
void
unpack_infos_playlist_config (pack_infos_playlist_config_t *pack,
                              cli_infos_t **infos, gchar **playlist,
                              gint *history, gint *upcoming,
                              xmmsc_coll_type_t *type, gchar **input)
{
	*infos = pack->infos;
	*playlist = pack->playlist;
	*history = pack->history;
	*upcoming = pack->upcoming;
	*type = pack->type;
	*input = pack->input;
}

/** Free the pack memory. */
void
free_infos_playlist_config (pack_infos_playlist_config_t *pack)
{
	g_free (pack->playlist);
	g_free (pack->input);
	g_free (pack);
}
