/*
 * Remmina - The GTK+ Remote Desktop Client
 * Copyright (C) 2010-2011 Vic Lee 
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, 
 * Boston, MA 02111-1307, USA.
 */

#ifndef __REMMINA_RDP_H__
#define __REMMINA_RDP_H__

#include "common/remmina_plugin.h"
#include <freerdp/freerdp.h>
#include <freerdp/channels/channels.h>
#include <freerdp/codec/color.h>
#include <freerdp/codec/rfx.h>
#include <freerdp/gdi/gdi.h>
#include <freerdp/gdi/dc.h>
#include <freerdp/gdi/region.h>
#include <gdk/gdkx.h>

typedef struct rf_context rfContext;

#define LOCK_BUFFER(t)	  	if (t) {CANCEL_DEFER} pthread_mutex_lock(&rfi->mutex);
#define UNLOCK_BUFFER(t)	pthread_mutex_unlock(&rfi->mutex); if (t) {CANCEL_ASYNC}

#define GET_DATA(_rfi)		(rfContext*) g_object_get_data(G_OBJECT(_rfi), "plugin-data")

#define DEFAULT_QUALITY_0	0x6f
#define DEFAULT_QUALITY_1	0x07
#define DEFAULT_QUALITY_2	0x01
#define DEFAULT_QUALITY_9	0x80

extern RemminaPluginService* remmina_plugin_service;

struct rf_pointer
{
	rdpPointer pointer;
	GdkCursor* cursor;
};
typedef struct rf_pointer rfPointer;

struct rf_bitmap
{
	rdpBitmap bitmap;
	Pixmap pixmap;
	cairo_surface_t* surface;
};
typedef struct rf_bitmap rfBitmap;

struct rf_glyph
{
	rdpGlyph glyph;
	Pixmap pixmap;
};
typedef struct rf_glyph rfGlyph;

struct rf_context
{
	rdpContext _p;

	RemminaProtocolWidget* protocol_widget;

	/* main */
	pthread_t thread;
	pthread_mutex_t mutex;
	gboolean scale;
	gboolean user_cancelled;

	GMutex* gmutex;
	GCond* gcond;

//	RDP_PLUGIN_DATA rdpdr_data[5];
//	RDP_PLUGIN_DATA drdynvc_data[5];
//	RDP_PLUGIN_DATA rdpsnd_data[5];
	gchar rdpsnd_options[20];

	RFX_CONTEXT* rfx_context;

	gboolean sw_gdi;
	GtkWidget* drawing_area;
	gint scale_width;
	gint scale_height;
	gdouble scale_x;
	gdouble scale_y;
	guint scale_handler;
	gboolean use_client_keymap;

	HGDI_DC hdc;
	gint srcBpp;
	GdkDisplay* display;
	GdkVisual* visual;
	cairo_surface_t* surface;
	cairo_format_t cairo_format;
	gint bpp;
	gint width;
	gint height;
	gint scanline_pad;
	gint* colormap;
	HCLRCONV clrconv;
	UINT8* primary_buffer;

	guint object_id_seq;
	GHashTable* object_table;

	GAsyncQueue* ui_queue;
	guint ui_handler;

	GArray* pressed_keys;
	GAsyncQueue* event_queue;
	gint event_pipe[2];

	GAsyncQueue* clipboard_queue;
	UINT32 format;
	gboolean clipboard_wait;
	gulong clipboard_handler;
};

typedef enum
{
	REMMINA_RDP_EVENT_TYPE_SCANCODE,
	REMMINA_RDP_EVENT_TYPE_MOUSE
} RemminaPluginRdpEventType;

struct remmina_plugin_rdp_event
{
	RemminaPluginRdpEventType type;
	union
	{
		struct
		{
			BOOL up;
			BOOL extended;
			UINT8 key_code;
		} key_event;
		struct
		{
			UINT16 flags;
			UINT16 x;
			UINT16 y;
		} mouse_event;
	};
};
typedef struct remmina_plugin_rdp_event RemminaPluginRdpEvent;

typedef enum
{
	REMMINA_RDP_UI_UPDATE_REGION = 0,
	REMMINA_RDP_UI_CONNECTED,
	REMMINA_RDP_UI_CURSOR,
	REMMINA_RDP_UI_RFX,
	REMMINA_RDP_UI_NOCODEC,
	REMMINA_RDP_UI_CLIPBOARD
} RemminaPluginRdpUiType;

typedef enum
{
	REMMINA_RDP_UI_CLIPBOARD_FORMATLIST,
	REMMINA_RDP_UI_CLIPBOARD_GET_DATA,
	REMMINA_RDP_UI_CLIPBOARD_SET_DATA
} RemminaPluginRdpUiClipboardType;

typedef enum
{
	REMMINA_RDP_POINTER_NEW,
	REMMINA_RDP_POINTER_FREE,
	REMMINA_RDP_POINTER_SET,
	REMMINA_RDP_POINTER_NULL,
	REMMINA_RDP_POINTER_DEFAULT
} RemminaPluginRdpUiPointerType;

struct remmina_plugin_rdp_ui_object
{
	RemminaPluginRdpUiType type;
	union
	{
		struct
		{
			gint x;
			gint y;
			gint width;
			gint height;
		} region;
		struct
		{
			rfPointer* pointer;
			RemminaPluginRdpUiPointerType type;
		} cursor;
		struct
		{
			gint left;
			gint top;
			RFX_MESSAGE* message;
		} rfx;
		struct
		{
			gint left;
			gint top;
			gint width;
			gint height;
			UINT8* bitmap;
		} nocodec;
		struct
		{
			RemminaPluginRdpUiClipboardType type;
			GtkTargetList* targetlist;
			UINT32 format;
		} clipboard;
	};
};
typedef struct remmina_plugin_rdp_ui_object RemminaPluginRdpUiObject;

void rf_init(RemminaProtocolWidget* gp);
void rf_uninit(RemminaProtocolWidget* gp);
void rf_get_fds(RemminaProtocolWidget* gp, void** rfds, int* rcount);
BOOL rf_check_fds(RemminaProtocolWidget* gp);
void rf_queue_ui(RemminaProtocolWidget* gp, RemminaPluginRdpUiObject* ui);
void rf_object_free(RemminaProtocolWidget* gp, RemminaPluginRdpUiObject* obj);

#endif

