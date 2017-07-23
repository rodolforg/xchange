/*
 * Copyright 2017 Rodolfo Ribeiro Gomes
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
 
#ifndef HEXCHANGE_GDKRGBA_H_
#define HEXCHANGE_GDKRGBA_H_

#include <gtk/gtk.h>

#if ! GTK_CHECK_VERSION(3,0,0)

struct GdkRGBA
{
	// GdkColor
	gdouble red;
	gdouble green;
	gdouble blue;
	// Alpha
	gdouble alpha;
};

typedef struct GdkRGBA GdkRGBA;

gboolean gdk_rgba_parse(GdkRGBA *rgba, const gchar *spec);

void gtk_color_button_get_rgba(GtkColorButton *button, GdkRGBA *rgba);
void gtk_color_button_set_rgba(GtkColorButton *button, const GdkRGBA *rgba);
#endif

#endif // HEXCHANGE_GDKRGBA_H_

