/*
 * Copyright 2011-2015 Rodolfo Ribeiro Gomes
 *
 *  This file is part of XChange.
 *
 *  XChange is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  XChange is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XChange.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef HEXVIEW_H_
#define HEXVIEW_H_

#include <gtk/gtk.h>
#include "file.h"
#include "chartable.h"

#if ! GTK_CHECK_VERSION(3,0,0)
#include "xgdkrgba.h"
#endif

G_BEGIN_DECLS

typedef struct _XChangeHexView XChangeHexView;
typedef struct _XChangeHexViewClass XChangeHexViewClass;

GType xchange_hex_view_get_type ();

#define XCHANGE_TYPE_HEX_VIEW             (xchange_hex_view_get_type ())
#define XCHANGE_HEX_VIEW(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), XCHANGE_TYPE_HEX_VIEW, XChangeHexView))
#define XCHANGE_HEX_VIEW_CLASS(obj)       (G_TYPE_CHECK_CLASS_CAST ((obj), XCHANGE_HEX_VIEW,  XChangeHexViewClass))
#define XCHANGE_IS_HEX_VIEW(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), XCHANGE_TYPE_HEX_VIEW))
#define XCHANGE_IS_HEX_VIEW_CLASS(obj)    (G_TYPE_CHECK_CLASS_TYPE ((obj), XCHANGE_TYPE_HEX_VIEW))
#define XCHANGE_HEX_VIEW_GET_CLASS        (G_TYPE_INSTANCE_GET_CLASS ((obj), XCHANGE_TYPE_HEX_VIEW, XChangeHexViewClass))

GtkWidget *xchange_hex_view_new(XChangeFile *xf);

gboolean xchange_hex_view_load_file(XChangeHexView *xchange_hex_view, XChangeFile *xf);
gboolean xchange_hex_view_save_file(XChangeHexView *xchange_hex_view, const char *filename);
void xchange_hex_view_close_file(XChangeHexView *xchange_hex_view);
size_t xchange_hex_view_get_file_size(XChangeHexView *xchange_hex_view);

gboolean xchange_hex_view_load_table_file(XChangeHexView *xchange_hex_view, gchar *filename, gchar *encoding);
void xchange_hex_view_set_table(XChangeHexView *xchange_hex_view, XChangeTable *xt);

void xchange_hex_view_goto(XChangeHexView *xchange_hex_view, off_t offset);

void xchange_hex_view_set_editable(XChangeHexView *xchange_hex_view, gboolean editable);
gboolean xchange_hex_view_is_editable(const XChangeHexView *xchange_hex_view);

void xchange_hex_view_set_edition_mode(XChangeHexView *xchange_hex_view, gboolean insertion);
gboolean xchange_hex_view_is_insertion_mode(const XChangeHexView *xchange_hex_view);

gboolean xchange_hex_view_undo(XChangeHexView *xchange_hex_view);
gboolean xchange_hex_view_redo(XChangeHexView *xchange_hex_view);


void xchange_hex_view_show_offset_panel(XChangeHexView *xchange_hex_view);
void xchange_hex_view_hide_offset_panel(XChangeHexView *xchange_hex_view);
void xchange_hex_view_set_offset_panel_visibility(XChangeHexView *xchange_hex_view, gboolean visible);
gboolean xchange_hex_view_is_offset_panel_visible(const XChangeHexView *xchange_hex_view);

void xchange_hex_view_show_text_panel(XChangeHexView *xchange_hex_view);
void xchange_hex_view_hide_text_panel(XChangeHexView *xchange_hex_view);
void xchange_hex_view_set_text_panel_visibility(XChangeHexView *xchange_hex_view, gboolean visible);
gboolean xchange_hex_view_is_text_panel_visible(const XChangeHexView *xchange_hex_view);


void xchange_hex_view_set_font_size(XChangeHexView *xchange_hex_view, gint font_size);
void xchange_hex_view_set_byte_font_name(XChangeHexView *xchange_hex_view, const gchar *font_name);
void xchange_hex_view_set_text_font_name(XChangeHexView *xchange_hex_view, const gchar *font_name);
void xchange_hex_view_set_font_color(XChangeHexView *xchange_hex_view, const GdkRGBA color);
void xchange_hex_view_set_font_color_default(XChangeHexView *xchange_hex_view);

void xchange_hex_view_set_byte_row_length(XChangeHexView *xchange_hex_view, gushort byte_row_length);

void xchange_hex_view_set_null_character_replacement(XChangeHexView *xchange_hex_view, const gchar *character);

const XChangeFile *xchange_hex_view_get_file(XChangeHexView *xchange_hex_view);
const XChangeTable *xchange_hex_view_get_table(XChangeHexView *xchange_hex_view);

gchar *xchange_hex_view_get_selected_text(XChangeHexView *xchange_hex_view, size_t *size);
guchar *xchange_hex_view_get_selected_bytes(XChangeHexView *xchange_hex_view, size_t *size);
gboolean xchange_hex_view_get_selection_bounds(XChangeHexView *xchange_hex_view, off_t *from, off_t *to);
gboolean xchange_hex_view_get_has_selection(const XChangeHexView *xchange_hex_view);
void xchange_hex_view_select(XChangeHexView *xchange_hex_view, off_t from, size_t size);
void xchange_hex_view_delete_selection(XChangeHexView *xchange_hex_view);
void xchange_hex_view_set_selection_color(XChangeHexView *xchange_hex_view, const GdkRGBA color);
void xchange_hex_view_set_selection_color_default(XChangeHexView *xchange_hex_view);

off_t xchange_hex_view_get_cursor(const XChangeHexView *xchange_hex_view);
void xchange_hex_view_place_cursor(XChangeHexView *xchange_hex_view, off_t offset);
void xchange_hex_view_set_cursor_background_color(XChangeHexView *xchange_hex_view, const GdkRGBA color);
void xchange_hex_view_set_cursor_background_color_default(XChangeHexView *xchange_hex_view);
void xchange_hex_view_set_cursor_foreground_color(XChangeHexView *xchange_hex_view, const GdkRGBA color);
void xchange_hex_view_set_cursor_foreground_color_default(XChangeHexView *xchange_hex_view);

//gchar *xchange_hex_view_get_text(XChangeHexView *xchange_hex_view, off_t offset, size_t size);
guchar *xchange_hex_view_get_bytes(XChangeHexView *xchange_hex_view, off_t offset, size_t size);
gboolean xchange_hex_view_insert_bytes(XChangeHexView *xchange_hex_view, const uint8_t *bytes, off_t offset, size_t size);
gboolean xchange_hex_view_replace_bytes(XChangeHexView *xchange_hex_view, const uint8_t *bytes, off_t offset, size_t size);
gboolean xchange_hex_view_delete_bytes(XChangeHexView *xchange_hex_view, off_t offset, size_t size);

G_END_DECLS

#endif /* HEXVIEW_H_ */
