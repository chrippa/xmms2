/*  XMMS2 - X Music Multiplexer System
 *  Copyright (C) 2003-2012 XMMS2 Team
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

#include "xmms/xmms_xformplugin.h"
#include "xmms/xmms_log.h"
#include "xmms/xmms_medialib.h"
#include "xmms/xmms_bindata.h"
#include "utils/jsonism.h"

#include <curl/curl.h>
#include <glib.h>
#include <glib/gprintf.h>

#define GMUSIC_DEVICE_ID "2"
#define GMUSIC_DEVICE_LOG_ID "2"

#define GMUSIC_AUTH_ACCOUNT "GOOGLE"
#define GMUSIC_AUTH_SERVICE "sj"
#define GMUSIC_AUTH_SOURCE "XMMS2/" XMMS_VERSION
#define GMUSIC_AUTH_URL "https://www.google.com/accounts/ClientLogin"

#define GMUSIC_PLAY_URL "https://android.clients.google.com/music/mplay?songid=%s"
#define GMUSIC_API_BASE "https://www.googleapis.com/sj/v1beta1"
#define GMUSIC_API_TRACK GMUSIC_API_BASE "/tracks/%s"

#define GMUSIC_CURL_CONNECT_TIMEOUT 15
#define GMUSIC_CURL_READ_TIMEOUT 15

#define GMUSIC_PROPERTY_CREATED "created"
#define GMUSIC_PROPERTY_LAST_MODIFIED "lastmodified"


/*
 * Type definitions
 */

typedef struct xmms_googlemusic_data_St {
	CURL *curl;
} xmms_googlemusic_data_t;


/*
 * Function definitions
 */

static gboolean xmms_googlemusic_plugin_setup (xmms_xform_plugin_t *xform_plugin);
static gboolean xmms_googlemusic_init (xmms_xform_t *xform);
static void xmms_googlemusic_config_changed (xmms_object_t *object, xmmsv_t *data, gpointer userdata);
static void xmms_googlemusic_destroy (xmms_xform_t *xform);

static xmmsv_t * xmms_googlemusic_http (xmms_xform_t *xform, const gchar *url, xmmsv_t *headers, xmmsv_t *form);
static xmmsv_t * xmms_googlemusic_api_request (xmms_xform_t *xform, const gchar *url);
static gboolean xmms_googlemusic_authenticate (xmms_xform_t *xform);
static gboolean xmms_googlemusic_get_song (xmms_xform_t *xform, const gchar *songid);
static void xmms_googlemusic_set_tag (const gchar *key, xmmsv_t *val, void *data);

static gboolean xmms_googlemusic_set_tag_created (xmms_xform_t *xform, const gchar *key,
                                                  const gchar *value, gsize length);
static gboolean xmms_googlemusic_set_tag_duration (xmms_xform_t *xform, const gchar *key,
                                                   const gchar *value, gsize length);
static gboolean xmms_googlemusic_set_tag_lastmodified (xmms_xform_t *xform, const gchar *key,
                                                       const gchar *value, gsize length);
static gboolean xmms_googlemusic_set_tag_size (xmms_xform_t *xform, const gchar *key,
                                               const gchar *value, gsize length);


/*
 * Metadata mappings
 */

static const xmms_xform_metadata_basic_mapping_t basic_mappings[] = {
	{ "artist",                XMMS_MEDIALIB_ENTRY_PROPERTY_ARTIST       },
	{ "album",                 XMMS_MEDIALIB_ENTRY_PROPERTY_ALBUM        },
	{ "albumArtist",           XMMS_MEDIALIB_ENTRY_PROPERTY_ALBUM_ARTIST },
	{ "beatsPerMinute",        XMMS_MEDIALIB_ENTRY_PROPERTY_BPM          },
	{ "comment",               XMMS_MEDIALIB_ENTRY_PROPERTY_COMMENT      },
	{ "composer",              XMMS_MEDIALIB_ENTRY_PROPERTY_COMPOSER     },
	{ "genre",                 XMMS_MEDIALIB_ENTRY_PROPERTY_GENRE        },
	{ "discNumber",            XMMS_MEDIALIB_ENTRY_PROPERTY_PARTOFSET    },
	{ "title",                 XMMS_MEDIALIB_ENTRY_PROPERTY_TITLE        },
	{ "totalDiscCount",        XMMS_MEDIALIB_ENTRY_PROPERTY_TOTALSET     },
	{ "totalTrackCount",       XMMS_MEDIALIB_ENTRY_PROPERTY_TOTALTRACKS  },
	{ "trackNumber",           XMMS_MEDIALIB_ENTRY_PROPERTY_TRACKNR      },
	{ "id",                    XMMS_MEDIALIB_ENTRY_PROPERTY_TRACK_ID     },
	{ "year",                  XMMS_MEDIALIB_ENTRY_PROPERTY_YEAR         },
};

static const xmms_xform_metadata_mapping_t mappings[] = {
	{ "creationTimestamp",     xmms_googlemusic_set_tag_created      },
	{ "durationMillis",        xmms_googlemusic_set_tag_duration     },
	{ "lastModifiedTimestamp", xmms_googlemusic_set_tag_lastmodified },
	{ "estimatedSize",         xmms_googlemusic_set_tag_size         },
};


/*
 * Global variables
 */

static gchar xmms_googlemusic_authtoken[1024];
static GStaticMutex xmms_googlemusic_authtoken_lock = G_STATIC_MUTEX_INIT;


/*
 * Plugin header
 */

XMMS_XFORM_PLUGIN ("googlemusic",
                   "Google Music song handler",
                   XMMS_VERSION,
                   "Authentication and metadata helper",
                   xmms_googlemusic_plugin_setup);

static gboolean
xmms_googlemusic_plugin_setup (xmms_xform_plugin_t *xform_plugin)
{
	xmms_xform_methods_t methods;

	XMMS_XFORM_METHODS_INIT (methods);
	methods.init = xmms_googlemusic_init;
	methods.destroy = xmms_googlemusic_destroy;

	xmms_xform_plugin_methods_set (xform_plugin, &methods);

	xmms_xform_plugin_indata_add (xform_plugin,
	                              XMMS_STREAM_TYPE_MIMETYPE,
	                              "application/x-url",
	                              XMMS_STREAM_TYPE_URL,
	                              "googlemusic://*",
	                              XMMS_STREAM_TYPE_END);

	xmms_xform_plugin_config_property_register (xform_plugin,
	                                            "username", "name@gmail.com",
	                                            xmms_googlemusic_config_changed,
	                                            NULL);

	xmms_xform_plugin_config_property_register (xform_plugin,
	                                            "password", "password",
	                                            xmms_googlemusic_config_changed,
	                                            NULL);

	xmms_xform_plugin_metadata_mapper_init (xform_plugin, basic_mappings,
	                                        G_N_ELEMENTS (basic_mappings),
	                                        mappings,
	                                        G_N_ELEMENTS (mappings));

	return TRUE;
}


/*
 * Member functions
 */

static gboolean
xmms_googlemusic_init (xmms_xform_t *xform)
{
	const gchar *url, *songid;
	xmms_googlemusic_data_t *data;

	g_return_val_if_fail (xform, FALSE);

	data = g_new0 (xmms_googlemusic_data_t, 1);
	data->curl = curl_easy_init ();
	xmms_xform_private_data_set (xform, data);

 	url = xmms_xform_indata_get_str (xform, XMMS_STREAM_TYPE_URL);
	songid = url + 14;

	/* Lock mutex while creating a global authtoken */
	g_static_mutex_lock (&xmms_googlemusic_authtoken_lock);
	if (!xmms_googlemusic_authtoken[0]) {
		if (!xmms_googlemusic_authenticate (xform)) {
			XMMS_DBG ("Failed to authenticate");

			g_static_mutex_unlock (&xmms_googlemusic_authtoken_lock);

			return FALSE;
		}
	}
	g_static_mutex_unlock (&xmms_googlemusic_authtoken_lock);

	/* Query API for song metadata */
	if (!xmms_googlemusic_get_song (xform, songid)) {
		return FALSE;
	}

	/**
	 * Only fetch stream url when there is output expected.
	 *
	 * This is because Google has a limit on the amount of streams
	 * that can be requested per month and we don't want to waste
	 * it on collecting metadata from http and mp3 xforms (no ID3
	 * tags in the stream) when rehashing the medialib.
	 *
	 */
	if (!xmms_xform_isrehash (xform)) {
		const gchar *streamurl;
		gchar playurl[256];
		xmmsv_t *result;

		g_snprintf (playurl, sizeof (playurl), GMUSIC_PLAY_URL, songid);
		result = xmms_googlemusic_api_request (xform, playurl);

		if (!xmmsv_get_string (result, &streamurl)) {
			XMMS_DBG ("Failed to resolve stream URL");

			xmmsv_unref (result);

			return FALSE;
		}

		/* Forward stream URL to a http xform */
		xmms_xform_outdata_type_add (xform,
		                              XMMS_STREAM_TYPE_MIMETYPE,
		                             "application/x-url",
		                             XMMS_STREAM_TYPE_URL,
		                             streamurl,
		                             XMMS_STREAM_TYPE_END);

		xmmsv_unref (result);
	} else {
		/* End the xform chain here */
		xmms_xform_outdata_type_add (xform,
		                             XMMS_STREAM_TYPE_MIMETYPE,
		                             "audio/pcm",
		                             XMMS_STREAM_TYPE_END);
	}

	return TRUE;
}


static void
xmms_googlemusic_config_changed (xmms_object_t *object, xmmsv_t *data,
                                 gpointer userdata)
{
	/* Reset the global authtoken */
	g_static_mutex_lock (&xmms_googlemusic_authtoken_lock);
	memset (xmms_googlemusic_authtoken, 0, sizeof (xmms_googlemusic_authtoken));
	g_static_mutex_unlock (&xmms_googlemusic_authtoken_lock);
}

static void
xmms_googlemusic_destroy (xmms_xform_t *xform)
{
	xmms_googlemusic_data_t *data;

	g_return_if_fail (xform);

	data = xmms_xform_private_data_get (xform);
	g_return_if_fail (data);

	curl_easy_cleanup (data->curl);
	g_free (data);
}

static size_t
curl_data_callback (void *ptr, size_t size, size_t nmemb, void *userdata)
{
	gint len;
	GString *buffer;

	buffer = (GString *) userdata;
	len = size * nmemb;

	g_string_append_len (buffer, ptr, len);

	return len;
}

static struct curl_slist *
xmmsv_to_curl_header_list (xmmsv_t *dict)
{
	struct curl_slist *headers = NULL;
	const gchar *key, *strval;
	gchar header[1024];
	xmmsv_dict_iter_t *it;
	xmmsv_t *val;

	g_return_val_if_fail (dict, NULL);
	g_return_val_if_fail (xmmsv_is_type (dict, XMMSV_TYPE_DICT), NULL);

	for (xmmsv_get_dict_iter (dict, &it); xmmsv_dict_iter_valid (it); xmmsv_dict_iter_next (it)) {
		xmmsv_dict_iter_pair (it, &key, &val);

		if (xmmsv_get_string (val, &strval)) {
			g_snprintf (header, sizeof (header), "%s: %s", key, strval);
			headers = curl_slist_append (headers, header);
		}
	}

	return headers;
}

static struct curl_httppost *
xmmsv_to_curl_form (xmmsv_t *dict)
{
	struct curl_httppost *form = NULL, *last = NULL;
	const gchar *key, *strval;
	xmmsv_dict_iter_t *it;
	xmmsv_t *val;

	g_return_val_if_fail (dict, NULL);
	g_return_val_if_fail (xmmsv_is_type (dict, XMMSV_TYPE_DICT), NULL);

	for (xmmsv_get_dict_iter (dict, &it); xmmsv_dict_iter_valid (it); xmmsv_dict_iter_next (it)) {
		xmmsv_dict_iter_pair (it, &key, &val);

		if (xmmsv_get_string (val, &strval)) {
			curl_formadd (&form, &last, CURLFORM_COPYNAME, key,
			              CURLFORM_COPYCONTENTS, strval, CURLFORM_END);
		}
	}

	return form;
}


static xmmsv_t *
xmms_googlemusic_http (xmms_xform_t *xform, const gchar *url, xmmsv_t *headers,
                       xmmsv_t *form)
{
	GString *buffer;
	gint res;
	xmmsv_t *result = NULL;
	xmms_googlemusic_data_t *data;
	struct curl_slist *cheaders = NULL;
	struct curl_httppost *cform = NULL;

	g_return_val_if_fail (xform, NULL);

	data = xmms_xform_private_data_get (xform);
	g_return_val_if_fail (data, NULL);

	curl_easy_reset (data->curl);
	buffer = g_string_new (NULL);

	if (headers) {
		cheaders = xmmsv_to_curl_header_list (headers);

		curl_easy_setopt (data->curl, CURLOPT_HTTPHEADER, cheaders);
	}

	if (form) {
		cform = xmmsv_to_curl_form (form);

		curl_easy_setopt (data->curl, CURLOPT_HTTPPOST, cform);
	}

	curl_easy_setopt (data->curl, CURLOPT_URL, url);
	curl_easy_setopt (data->curl, CURLOPT_HEADER, 0);
	curl_easy_setopt (data->curl, CURLOPT_FOLLOWLOCATION, 0);
	curl_easy_setopt (data->curl, CURLOPT_NOPROGRESS, 1);
	curl_easy_setopt (data->curl, CURLOPT_CONNECTTIMEOUT, GMUSIC_CURL_CONNECT_TIMEOUT);
	curl_easy_setopt (data->curl, CURLOPT_LOW_SPEED_TIME, GMUSIC_CURL_READ_TIMEOUT);
	curl_easy_setopt (data->curl, CURLOPT_LOW_SPEED_LIMIT, 1);
	curl_easy_setopt (data->curl, CURLOPT_WRITEDATA, buffer);
	curl_easy_setopt (data->curl, CURLOPT_WRITEFUNCTION, curl_data_callback);

	res = curl_easy_perform (data->curl);

	if (res != CURLE_OK) {
		XMMS_DBG ("curl error: %s", curl_easy_strerror (res));
	} else {
		const gchar *redir;

		curl_easy_getinfo (data->curl, CURLINFO_REDIRECT_URL, &redir);

		if (redir) {
			result = xmmsv_new_string (redir);
		} else {
			const gchar *contenttype;

			curl_easy_getinfo (data->curl, CURLINFO_CONTENT_TYPE, &contenttype);

			if (g_str_has_prefix (contenttype, "application/json")) {
				result = xmmsv_from_json (buffer->str);
			} else if (g_str_has_prefix (contenttype, "text/plain")) {
				result = xmmsv_new_string (buffer->str);
			} else {
				result = xmmsv_new_bin ((const guchar *) buffer->str, buffer->len);
			}
		}
	}

	if (cheaders)
		curl_slist_free_all (cheaders);

	if (cform)
		curl_formfree (cform);

	g_string_free (buffer, TRUE);

	return result;
}

static gboolean
xmms_googlemusic_authenticate (xmms_xform_t *xform)
{
	xmmsv_t *form, *result;
	xmms_config_property_t *val;
	const gchar *username, *password;
	gboolean success;

	g_return_val_if_fail (xform, FALSE);

	val = xmms_xform_config_lookup (xform, "username");
	username = xmms_config_property_get_string (val);

	val = xmms_xform_config_lookup (xform, "password");
	password = xmms_config_property_get_string (val);

	form = xmmsv_new_dict ();

	xmmsv_dict_set_string (form, "Email", username);
	xmmsv_dict_set_string (form, "Passwd", password);
	xmmsv_dict_set_string (form, "accountType", GMUSIC_AUTH_ACCOUNT);
	xmmsv_dict_set_string (form, "source", GMUSIC_AUTH_SOURCE);
	xmmsv_dict_set_string (form, "service", GMUSIC_AUTH_SERVICE);

	success = FALSE;
	result = xmms_googlemusic_http (xform, GMUSIC_AUTH_URL, NULL, form);

	if (result) {
		const gchar *body, *start, *end;
		gchar *authtoken;

		if (xmmsv_get_string (result, &body)) {
			start = g_strstr_len (body, -1, "Auth=");

			if (start) {
				end = g_strstr_len (start, -1, "\n");
				authtoken = g_strndup (start, end - start);
				success = TRUE;

				g_snprintf (xmms_googlemusic_authtoken, sizeof (xmms_googlemusic_authtoken),
				            "GoogleLogin %s", authtoken);

				g_free (authtoken);
			}
		}

		xmmsv_unref (result);
	}

	xmmsv_unref (form);

	return success;
}

static xmmsv_t *
xmms_googlemusic_api_request (xmms_xform_t *xform, const gchar *url)
{
	xmmsv_t *headers, *result;

	g_return_val_if_fail (xform, NULL);
	g_return_val_if_fail (url, NULL);

	headers = xmmsv_new_dict ();

	xmmsv_dict_set_string (headers, "Authorization", xmms_googlemusic_authtoken);
	xmmsv_dict_set_string (headers, "X-Device-ID", GMUSIC_DEVICE_ID);
	xmmsv_dict_set_string (headers, "X-Device-Logging-ID", GMUSIC_DEVICE_LOG_ID);

	result = xmms_googlemusic_http (xform, url, headers, NULL);

	xmmsv_unref (headers);

	return result;
}

static void
xmms_googlemusic_set_tag (const gchar *key, xmmsv_t *val, gpointer data)
{
	xmms_xform_t *xform;
	gint ival;
	const gchar *strval;
	gchar tmp[32];

	g_return_if_fail (key);
	g_return_if_fail (val);
	g_return_if_fail (data);

	xform = (xmms_xform_t *) data;

	switch (xmmsv_get_type (val)) {
	case XMMSV_TYPE_INT32:
		if (xmmsv_get_int (val, &ival)) {
			g_snprintf (tmp, sizeof (tmp), "%d", ival);

			xmms_xform_metadata_mapper_match (xform, key, tmp, 0);
		}
		break;
	case XMMSV_TYPE_STRING:
		if (xmmsv_get_string (val, &strval)) {
			xmms_xform_metadata_mapper_match (xform, key, strval, 0);
		}
		break;
	default:
		break;
	}
}

static gboolean
xmms_googlemusic_set_tag_created (xmms_xform_t *xform, const gchar *key,
                                  const gchar *value, gsize length)
{
	return xmms_xform_metadata_set_str (xform, GMUSIC_PROPERTY_CREATED, value);
}

static gboolean
xmms_googlemusic_set_tag_duration (xmms_xform_t *xform, const gchar *key,
                                   const gchar *value, gsize length)
{
	return xmms_xform_metadata_parse_number (xform, XMMS_MEDIALIB_ENTRY_PROPERTY_DURATION,
	                                         value, 0);
}

static gboolean
xmms_googlemusic_set_tag_lastmodified (xmms_xform_t *xform, const gchar *key,
                                        const gchar *value, gsize length)
{
	return xmms_xform_metadata_set_str (xform, GMUSIC_PROPERTY_LAST_MODIFIED, value);
}

static gboolean
xmms_googlemusic_set_tag_size (xmms_xform_t *xform, const gchar *key,
                               const gchar *value, gsize length)
{
	return xmms_xform_metadata_parse_number (xform, XMMS_MEDIALIB_ENTRY_PROPERTY_SIZE,
	                                         value, 0);
}

static gboolean
xmms_googlemusic_get_song (xmms_xform_t *xform, const gchar *songid)
{
	xmmsv_t *val, *info;
	gchar apiurl[256];

	g_return_val_if_fail (xform, FALSE);
	g_return_val_if_fail (songid, FALSE);

	g_snprintf (apiurl, sizeof (apiurl), GMUSIC_API_TRACK, songid);
	info = xmms_googlemusic_api_request (xform, apiurl);

	if (!info) {
		XMMS_DBG("Failed to get song info");

		return FALSE;
	}

	if (!xmmsv_is_type (info, XMMSV_TYPE_DICT)) {
		XMMS_DBG("Song info is not in JSON format");

		xmmsv_unref (info);

		return FALSE;
	}

	if (xmmsv_dict_get (info, "error", &val)) {
		const gchar *msg;
		gint code;

		if (xmmsv_dict_entry_get_int (val, "code", &code) &&
		    xmmsv_dict_entry_get_string (val, "message", &msg)) {

			XMMS_DBG("Error while fetching metadata: %s (%d)", msg, code);
		}

		xmmsv_unref (info);

		return FALSE;
	}

	xmmsv_dict_foreach (info, xmms_googlemusic_set_tag, xform);

	xmmsv_unref (info);

	return TRUE;
}

