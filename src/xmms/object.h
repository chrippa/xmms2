#ifndef __XMMS_OBJECT_H__
#define __XMMS_OBJECT_H__

#include <glib.h>

#define XMMS_OBJECT_MID 0x00455574

typedef struct xmms_object_St {
	guint32 id;
	GHashTable *methods;
	GMutex *mutex;
	struct xmms_object_St *parent;
} xmms_object_t;

typedef void (*xmms_object_handler_t) (xmms_object_t *object, gconstpointer data, gpointer userdata);

#define XMMS_OBJECT(p) ((xmms_object_t *)p)
#define XMMS_IS_OBJECT(p) (XMMS_OBJECT (p)->id == XMMS_OBJECT_MID)

void xmms_object_init (xmms_object_t *object);
void xmms_object_cleanup (xmms_object_t *object);

void xmms_object_parent_set (xmms_object_t *object, xmms_object_t *parent);

void xmms_object_connect (xmms_object_t *object, const gchar *method,
						  xmms_object_handler_t handler, gpointer userdata);
void xmms_object_disconnect (xmms_object_t *object, const gchar *method,
							 xmms_object_handler_t handler);

void xmms_object_emit (xmms_object_t *object, const gchar *method,
					   gconstpointer data);

#endif /* __XMMS_OBJECT_H__ */
