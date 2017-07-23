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

#include "xgdkrgba.h"

#if ! GTK_CHECK_VERSION(3,0,0)

static void color_to_rgba(const GdkColor *color, GdkRGBA *rgba)
{
	rgba->red   = (gdouble)color->red   / 65535.0;
	rgba->green = (gdouble)color->green / 65535.0;
	rgba->blue  = (gdouble)color->blue  / 65535.0;
}

static void rgba_to_color(const GdkRGBA *rgba, GdkColor *color)
{
	color->red   = rgba->red   * 65535.0;
	color->green = rgba->green * 65535.0;
	color->blue  = rgba->blue  * 65535.0;
}

gboolean gdk_rgba_parse (GdkRGBA *rgba, const gchar *spec)
{
	glong len = g_utf8_strlen (spec, 12);
	if (len % 3 != 0)
		return FALSE;

	GdkColor color;
	rgba_to_color(rgba, &color);
	rgba->alpha = 0;
	return gdk_color_parse(spec, &color);
}

void gtk_color_button_get_rgba(GtkColorButton *button, GdkRGBA *rgba)
{
	GdkColor color;
	gtk_color_button_get_color(button, &color);
	color_to_rgba(&color, rgba);
	rgba->alpha = gtk_color_button_get_alpha(button);
}

void gtk_color_button_set_rgba(GtkColorButton *button, const GdkRGBA *rgba)
{
	GdkColor color;
	rgba_to_color(rgba, &color);
	gtk_color_button_set_color(button, &color);
	gtk_color_button_set_alpha(button, rgba->alpha);
}

#endif
