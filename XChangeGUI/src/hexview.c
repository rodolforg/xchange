/*
 * hexview.c
 *
 *  Created on: 27/01/2011
 *      Author: rodolfo
 */

#include "file.h"
#include "chartable.h"
#include "hexview.h"
#include <gdk/gdkkeysyms.h>
#include <pango/pangocairo.h>

#include <string.h> // For Miniedition...

#define MARGIN_OFFSET_TEXT 3
#define LINE_SPACEMENT 5
#define TEXT_SPACEMENT 10
#define OFFSET_PANEL_SPACEMENT 5

#define VISUAL_CARACTERE_NULO "_"

#define PADRAO_COR_SELECAO_R 0.5
#define PADRAO_COR_SELECAO_G 0.5
#define PADRAO_COR_SELECAO_B 0.95
#define PADRAO_COR_SELECAO_A 0.5

#define PADRAO_COR_FONTE_R 0.0
#define PADRAO_COR_FONTE_G 0.0
#define PADRAO_COR_FONTE_B 0.0
#define PADRAO_COR_FONTE_A 1.0

#define PADRAO_COR_FUNDO_CURSOR_R 0.95
#define PADRAO_COR_FUNDO_CURSOR_G 0.95
#define PADRAO_COR_FUNDO_CURSOR_B 0.95
#define PADRAO_COR_FUNDO_CURSOR_A 0.7

#define PADRAO_COR_CONTORNO_CURSOR_R 0.8
#define PADRAO_COR_CONTORNO_CURSOR_G 0.8
#define PADRAO_COR_CONTORNO_CURSOR_B 0.8
#define PADRAO_COR_CONTORNO_CURSOR_A 1.0

#define PADRAO_TAMANHO_FONTE 16
#define PADRAO_NOME_FONTE "Mono"
#define PADRAO_BYTES_POR_LINHA 16

struct _XChangeHexView
{
	GtkDrawingArea parent;

	/* private */
	// Info about data to display
	XChangeFile * xf;
	off_t fileoffset;
	uint8_t *bytes;
	size_t byte_buffer_size;

	gboolean table_loaded;
	XChangeTable * xt;

	// Display info
	int8_t view_offset;
	float font_size;
	gchar *font_face;
	gchar *text_font_face;
	cairo_pattern_t *font_color;
	gushort byte_row_length;
	off_t cursor_position;
	gboolean cursor_position_half;
	gboolean cursor_visible;
	cairo_pattern_t *cursor_foreground_color;
	cairo_pattern_t *cursor_background_color;
	cairo_pattern_t *selection_color;

	off_t selection_start;
	off_t selection_end;

	// Scroll info
	GtkAdjustment * hadjustment;
	GtkAdjustment * vadjustment;

	// computed info
	gint offset_panel_width;
	gint character_width;
	gboolean cursor_in_blink;
	guint64 virtual_w, virtual_h;
	gint lines_shown;
	gint byte_panel_width;
	gint *bytes_skipped; // Bytes ignored from line beginning in order to best print
	PangoFontDescription *font_description;
	PangoFontDescription *text_font_description;

	// Features
	gboolean editable;
	gboolean overwrite;
	gboolean show_offset_panel;
	gboolean show_text_panel;
	gchar *null_char_replacer;
};

struct _XChangeHexViewClass
{
	GtkDrawingAreaClass parent_class;

	gboolean (*set_scroll_adjustments)(GtkWidget *xchange_hex_view,
			GtkAdjustment *hadjustment, GtkAdjustment *vadjustment);

	guint cursor_moved_signal;
	void (*cursor_moved)(GtkWidget *xchange_hex_view, gpointer data);

	guint edition_mode_changed_signal;
	void (*editon_mode_changed)(GtkWidget *xchange_hex_view, gpointer data);

	guint selection_changed_signal;
	void (*selection_changed)(GtkWidget *xchange_hex_view, gpointer data);

	guint changed_signal;
	void (*changed)(GtkWidget *xchange_hex_view, gpointer data);
};

G_DEFINE_TYPE(XChangeHexView, xchange_hex_view, GTK_TYPE_DRAWING_AREA);

static gboolean
xchange_hex_view_expose(GtkWidget *hexv, GdkEventExpose *event);

static gboolean
xchange_hex_view_configure(GtkWidget *hexv, GdkEventConfigure *event);

static void
xchange_hex_view_destroy(GtkObject *xchange_hex_view);

static gboolean
xchange_hex_view_button_press(GtkWidget *hexv, GdkEventButton *event);

static gboolean
xchange_hex_view_button_release(GtkWidget *hexv, GdkEventButton *event);

static gboolean
xchange_hex_view_motion(GtkWidget *hexv, GdkEventMotion *event);

static gboolean
xchange_hex_view_key_press(GtkWidget *hexv, GdkEventKey *event);

static void
draw(GtkWidget *widget, cairo_t *cr);

static void
font_dimention_changed(XChangeHexView *hexv);

static void xchange_hex_view_delete(XChangeHexView *xchange_hex_view, off_t origem, size_t qtd);

static gboolean
xchange_hex_view_set_scroll_adjustments(GtkWidget *xchange_hex_view,
		GtkAdjustment *hadjustment, GtkAdjustment *vadjustment);

static void
update_scroll_bounds(XChangeHexView *xchange_hex_view);


#if ! GTK_CHECK_VERSION(2,18,0) || __WIN32__
void gtk_widget_get_allocation (GtkWidget *widget, GtkAllocation *allocation)
{
	GTK_IS_WIDGET(widget);
	*allocation = widget->allocation;
}
#endif


/* VOID:OBJECT (./gmarshal.list:43) */
#define g_marshal_value_peek_object(v)   (v)->data[0].v_pointer
static void
g_cclosure_marshal_VOID__OBJECT_OBJECT (GClosure *closure,
		GValue *return_value,// G_GNUC_UNUSED,
		guint n_param_values,
		const GValue *param_values,
		gpointer invocation_hint,// G_GNUC_UNUSED,
		gpointer marshal_data)
{
	typedef void (*GMarshalFunc_VOID__OBJECT_OBJECT) (gpointer data1,
			gpointer arg_1,
			gpointer arg_2,
			gpointer data2);
	register GMarshalFunc_VOID__OBJECT_OBJECT callback;
	register GCClosure *cc = (GCClosure*) closure;
	register gpointer data1, data2;

	g_return_if_fail (n_param_values == 3);

	if (G_CCLOSURE_SWAP_DATA (closure))
	{
		data1 = closure->data;
		data2 = g_value_peek_pointer (param_values + 0);
	}
	else
	{
		data1 = g_value_peek_pointer (param_values + 0);
		data2 = closure->data;
	}
	callback = (GMarshalFunc_VOID__OBJECT_OBJECT) (marshal_data ? marshal_data : cc->callback);

	callback (data1,
			g_marshal_value_peek_object (param_values + 1),
			g_marshal_value_peek_object (param_values + 2),
			data2);
}

static void xchange_hex_view_class_init(XChangeHexViewClass *class)
{
	GtkWidgetClass *widget_class;
	GtkObjectClass *object_class;

	widget_class = GTK_WIDGET_CLASS(class);
	object_class = (GtkObjectClass *) class;

	widget_class->expose_event = xchange_hex_view_expose;
	widget_class->configure_event = xchange_hex_view_configure;
	widget_class->button_press_event = xchange_hex_view_button_press;
	widget_class->button_release_event = xchange_hex_view_button_release;
	widget_class->motion_notify_event = xchange_hex_view_motion;
	widget_class->key_press_event = xchange_hex_view_key_press;
	class->set_scroll_adjustments = xchange_hex_view_set_scroll_adjustments;

	guint set_scroll_adjustments_signal = g_signal_new(
			"set-scroll-adjustments", G_TYPE_FROM_CLASS(object_class),
			G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION, G_STRUCT_OFFSET(
					XChangeHexViewClass, set_scroll_adjustments), NULL, NULL,
			g_cclosure_marshal_VOID__OBJECT_OBJECT, G_TYPE_NONE, 2,
			GTK_TYPE_ADJUSTMENT, GTK_TYPE_ADJUSTMENT);
	widget_class->set_scroll_adjustments_signal = set_scroll_adjustments_signal;

	guint cursor_moved_signal = g_signal_new("cursor-moved", G_TYPE_FROM_CLASS(
			object_class), G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
			G_STRUCT_OFFSET(XChangeHexViewClass, cursor_moved), NULL, NULL,
			g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);
	class->cursor_moved_signal = cursor_moved_signal;

	guint edition_mode_changed_signal = g_signal_new("edition-mode-changed", G_TYPE_FROM_CLASS(
			object_class), G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
			G_STRUCT_OFFSET(XChangeHexViewClass, editon_mode_changed), NULL, NULL,
			g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);
	class->edition_mode_changed_signal = edition_mode_changed_signal;

	guint selection_changed_signal = g_signal_new("selection-changed", G_TYPE_FROM_CLASS(
			object_class), G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
			G_STRUCT_OFFSET(XChangeHexViewClass, selection_changed), NULL, NULL,
			g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);
	class->selection_changed_signal = selection_changed_signal;

	guint changed_signal = g_signal_new("changed", G_TYPE_FROM_CLASS(
			object_class), G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
			G_STRUCT_OFFSET(XChangeHexViewClass, changed), NULL, NULL,
			g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);
	class->edition_mode_changed_signal = changed_signal;

	object_class->destroy = xchange_hex_view_destroy;
}

static void xchange_hex_view_init(XChangeHexView *hexv)
{
	hexv->fileoffset = 0;
	hexv->bytes = NULL;
	hexv->byte_buffer_size = 1024;

	hexv->table_loaded = TRUE;
	hexv->xt = xchange_table_create_from_encoding("ISO-8859-1");

	hexv->view_offset = 0;
	hexv->font_size = PADRAO_TAMANHO_FONTE;
	hexv->font_face = NULL;//g_strdup(PADRAO_NOME_FONTE);
	hexv->text_font_face = NULL;//g_strdup(PADRAO_NOME_FONTE);
	hexv->font_color = cairo_pattern_create_rgba(PADRAO_COR_FONTE_R, PADRAO_COR_FONTE_G, PADRAO_COR_FONTE_B, PADRAO_COR_FONTE_A);
	cairo_pattern_reference (hexv->font_color);
	// cairo_pattern_status (hexv->font_color)
	hexv->offset_panel_width = 20;
	hexv->cursor_position = 0;
	hexv->cursor_position_half = FALSE;
	hexv->cursor_visible = TRUE;
	hexv->cursor_foreground_color = cairo_pattern_create_rgba(PADRAO_COR_CONTORNO_CURSOR_R, PADRAO_COR_CONTORNO_CURSOR_G, PADRAO_COR_CONTORNO_CURSOR_B, PADRAO_COR_CONTORNO_CURSOR_A);
	cairo_pattern_reference (hexv->cursor_foreground_color);
	hexv->cursor_background_color = cairo_pattern_create_rgba(PADRAO_COR_FUNDO_CURSOR_R, PADRAO_COR_FUNDO_CURSOR_G, PADRAO_COR_FUNDO_CURSOR_B, PADRAO_COR_FUNDO_CURSOR_A);
	cairo_pattern_reference (hexv->cursor_background_color);
	hexv->byte_row_length = PADRAO_BYTES_POR_LINHA;

	hexv->selection_color = cairo_pattern_create_rgba(PADRAO_COR_SELECAO_R, PADRAO_COR_SELECAO_G, PADRAO_COR_SELECAO_B, PADRAO_COR_SELECAO_A);
	cairo_pattern_reference (hexv->selection_color);
	// cairo_pattern_status (hexv->selection_color)

	hexv->selection_start = -1;
	hexv->selection_end = 0;

	hexv->virtual_w = 0;
	hexv->virtual_h = 0;

	hexv->lines_shown = 0;
	hexv->byte_panel_width = 20;
	hexv->bytes_skipped = NULL;

	hexv->font_description = NULL;
	hexv->text_font_description = NULL;


	hexv->hadjustment = NULL;
	hexv->vadjustment = NULL;

	hexv->editable = TRUE;
	hexv->overwrite = TRUE;
	hexv->show_offset_panel = TRUE;
	hexv->show_text_panel = TRUE;

	hexv->null_char_replacer = g_strdup(VISUAL_CARACTERE_NULO);
}

GtkWidget *
xchange_hex_view_new(XChangeFile *xf)
{
	GtkWidget *new = g_object_new(XCHANGE_TYPE_HEX_VIEW, NULL);
	XChangeHexView * hexv = XCHANGE_HEX_VIEW(new);

	if (!xchange_hex_view_load_file(hexv, xf))
	{
		g_object_unref(new);
		return NULL;
	}

	gtk_widget_set_events(new, GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK
			| GDK_KEY_PRESS_MASK | GDK_BUTTON1_MOTION_MASK);
#if GTK_CHECK_VERSION(2, 18, 0) && ! __WIN32__
	gtk_widget_set_can_focus(new, TRUE);
#else // FIXME: Na verdade, gtk < 2.18
	GTK_WIDGET_SET_FLAGS (new, GTK_CAN_FOCUS);
#endif

	hexv->font_face = g_strdup(PADRAO_NOME_FONTE);
	hexv->text_font_face = g_strdup(PADRAO_NOME_FONTE);
	hexv->font_description = pango_font_description_new ();
	pango_font_description_set_family (hexv->font_description, hexv->font_face);
	//pango_layout_set_single_paragraph_mode(layout, TRUE);
	hexv->text_font_description = pango_font_description_new ();
	pango_font_description_set_family (hexv->text_font_description, hexv->text_font_face);
	xchange_hex_view_set_font_size(XCHANGE_HEX_VIEW(new), hexv->font_size); // FIXME: qq mudança na fonte...


	font_dimention_changed(hexv);
	gtk_widget_set_size_request(new, hexv->virtual_w, -1); // TODO: Calcular virtual_h
	return new;
}

gboolean xchange_hex_view_load_file(XChangeHexView *xchange_hex_view,
		XChangeFile *xf)
{
	XChangeHexView *hexv = XCHANGE_HEX_VIEW(xchange_hex_view);
	if (hexv->xf != NULL)
		xchange_hex_view_close_file(hexv);

	hexv->fileoffset = 0;
	hexv->view_offset = 0;

	if (xf != NULL)
	{
		if (hexv->bytes != NULL)
			free(hexv->bytes);
		hexv->bytes = malloc(hexv->byte_buffer_size);
		if (hexv->bytes == NULL)
		{
			return FALSE;
		}
		if (xchange_get_bytes(xf, 0, hexv->bytes, hexv->byte_buffer_size) >= 0)
			hexv->xf = xf;
		else
		{
			free(hexv->bytes);
			hexv->bytes = NULL;
			return FALSE;
		}
	}

	gtk_widget_queue_draw(GTK_WIDGET(hexv));
	update_scroll_bounds(hexv);
	if (hexv->selection_start != -1 || hexv->selection_end != -1)
	{
		hexv->selection_start = -1;
		hexv->selection_end = -1;
		g_signal_emit_by_name(xchange_hex_view, "selection-changed");
	}
	hexv->cursor_position = 0;

	g_signal_emit_by_name(GTK_WIDGET(hexv), "changed");
	return TRUE;
}

/*
 * Save the file contents. If filename is not NULL, the behaviour is "save as".
 */
gboolean xchange_hex_view_save_file(XChangeHexView *xchange_hex_view, const char *filename)
{
	XChangeHexView * hexv = XCHANGE_HEX_VIEW(xchange_hex_view);
	if (filename == NULL)
	{
		return xchange_save(hexv->xf) > 0;
	}
	return xchange_save_as(hexv->xf, filename) > 0;
}

void xchange_hex_view_close_file(XChangeHexView *xchange_hex_view)
{
	XChangeHexView * hexv = XCHANGE_HEX_VIEW(xchange_hex_view);
	if (hexv->xf == NULL)
		return;
	free(hexv->bytes);
	hexv->bytes = NULL;

	if (hexv->selection_start != -1 || hexv->selection_end != -1)
	{
		hexv->selection_start = -1;
		hexv->selection_end = -1;
		g_signal_emit_by_name(xchange_hex_view, "selection-changed");
	}

	hexv->virtual_w = 0;
	hexv->virtual_h = 0;

	xchange_close(hexv->xf);
	hexv->xf = NULL;

	hexv->fileoffset = 0;
	off_t cursor_inicial = hexv->cursor_position;
	int cursor_pedaco_inicial = hexv->cursor_position_half;
	hexv->cursor_position = 0;
	hexv->cursor_position_half = 0;
	if (cursor_inicial != hexv->cursor_position || cursor_pedaco_inicial != hexv->cursor_position_half)
		g_signal_emit_by_name(hexv, "cursor-moved");


	gtk_widget_queue_draw(GTK_WIDGET(hexv));
	update_scroll_bounds(hexv);
	g_signal_emit_by_name(GTK_WIDGET(hexv), "changed");
}

size_t xchange_hex_view_get_file_size(XChangeHexView *xchange_hex_view)
{
	XChangeHexView * hexv = XCHANGE_HEX_VIEW(xchange_hex_view);
	if (hexv == NULL)
		return (size_t) -1;
	return xchange_get_size(hexv->xf);
}

static void xchange_hex_view_destroy(GtkObject *object)
{
	XChangeHexView * hexv = XCHANGE_HEX_VIEW(object);
	free(hexv->bytes);
	hexv->bytes = NULL;
	if (hexv->table_loaded)
		xchange_table_close(hexv->xt);
	hexv->xt = NULL;

	cairo_pattern_destroy(hexv->selection_color);
	hexv->selection_color = NULL;

	free(hexv->bytes_skipped);
	hexv->bytes_skipped = NULL;
	hexv->lines_shown = 0;

	hexv->hadjustment = NULL;
	hexv->vadjustment = NULL;

	g_free(hexv->font_face);
	hexv->font_face = NULL;
	pango_font_description_free(hexv->font_description);
	hexv->font_description = NULL;
	g_free(hexv->text_font_face);
	hexv->text_font_face = NULL;
	pango_font_description_free(hexv->text_font_description);
	hexv->text_font_description = NULL;
	cairo_pattern_destroy(hexv->font_color);
	hexv->font_color = NULL;

	cairo_pattern_destroy(hexv->cursor_foreground_color);
	hexv->cursor_foreground_color = NULL;
	cairo_pattern_destroy(hexv->cursor_background_color);
	hexv->cursor_background_color = NULL;

	g_free(hexv->null_char_replacer);
	hexv->null_char_replacer = NULL;
}

static void calcula_tamanho_buffer(XChangeHexView *hexv)
{
	if (hexv == NULL)
		return;
	GtkWidget *widget = GTK_WIDGET(hexv);
	GtkAllocation allocation;
	gtk_widget_get_allocation(widget, &allocation);
	gint nlines = allocation.height
			/ (hexv->font_size + LINE_SPACEMENT);
	size_t new_size = (nlines + 2) * hexv->byte_row_length;
	if (new_size != hexv->byte_buffer_size)
	{
		uint8_t *new_buffer = realloc(hexv->bytes, new_size);
		if (new_buffer != NULL)
		{
			hexv->bytes = new_buffer;
			if (hexv->byte_buffer_size < new_size)
			{
				xchange_get_bytes(hexv->xf, hexv->fileoffset, hexv->bytes,
						new_size);
			}
			hexv->byte_buffer_size = new_size;
		}
	}
}

static void compute_lines_shown(XChangeHexView * hexv)
{
	GtkWidget *widget = GTK_WIDGET(hexv);
	GtkAllocation allocation;
	gtk_widget_get_allocation(widget, &allocation);
	int new_lines_shown = (allocation.height - hexv->view_offset)/((int)hexv->font_size + LINE_SPACEMENT);
	if ((allocation.height - hexv->view_offset) % ((int)hexv->font_size + LINE_SPACEMENT) != 0)
		new_lines_shown ++;
	if (hexv->view_offset != 0)
		new_lines_shown ++;
	gint* new_bytes_skipped = g_try_realloc(hexv->bytes_skipped, new_lines_shown*sizeof(int));
	if (new_bytes_skipped != NULL)
	{
		hexv->lines_shown = new_lines_shown;
		hexv->bytes_skipped = new_bytes_skipped;
		int n;
		for (n = 0; n < new_lines_shown; n++)
			new_bytes_skipped[n] = 0;
	}
}

static gboolean xchange_hex_view_configure(GtkWidget *widget,
		GdkEventConfigure *event)
{
	XChangeHexView * hexv = XCHANGE_HEX_VIEW(widget);
	update_scroll_bounds(hexv);
	calcula_tamanho_buffer(hexv);

	// Computes number of lines shown
	compute_lines_shown(hexv);
	return FALSE;
}

static gboolean xchange_hex_view_expose(GtkWidget *hexv, GdkEventExpose *event)
{
	cairo_t *cr;
	cr = gdk_cairo_create(gtk_widget_get_window(hexv));

	cairo_rectangle(cr, event->area.x, event->area.y, event->area.width,
			event->area.height);
	cairo_clip(cr);

	draw(hexv, cr);

	cairo_destroy(cr);

	update_scroll_bounds(XCHANGE_HEX_VIEW(hexv));

	return FALSE;
}

/*
 static gboolean carregar_fonte_face()
 {
 static const cairo_user_data_key_t key;

 font_face = cairo_ft_font_face_create_for_ft_face (ft_face, 0);
 status = cairo_font_face_set_user_data (font_face, &key,
 ft_face, (cairo_destroy_func_t) FT_Done_Face);
 if (status) {
 cairo_font_face_destroy (font_face);
 FT_Done_Face (ft_face);
 return ERROR;
 }
 }
 */

static void draw_offset_panel(GtkWidget *widget, cairo_t *cr)
{
	XChangeHexView *hexv = XCHANGE_HEX_VIEW(widget);
	GtkAllocation allocation;
	gtk_widget_get_allocation(widget, &allocation);

	float offset_width = hexv->offset_panel_width;
	// Draw Offset panel background
	cairo_set_source_rgb(cr, 0.95, 0.95, 0.95);
	cairo_rectangle(cr, 0, 0, offset_width, allocation.height);
	cairo_fill(cr);
	// Draw Offset panel boundary
	cairo_set_source_rgb(cr, 0.8, 0.8, 0.8);
	cairo_rectangle(cr, offset_width, 0, 3, allocation.height);
	cairo_stroke(cr);

	// Font color
	gdouble cor_r, cor_g, cor_b, cor_a;
	if (cairo_pattern_get_rgba (hexv->font_color, &cor_r, &cor_g, &cor_b, &cor_a) == CAIRO_STATUS_SUCCESS)
		cairo_set_source_rgba(cr, cor_r, cor_g, cor_b, cor_a);
	else
		cairo_set_source_rgba(cr, PADRAO_COR_FONTE_R, PADRAO_COR_FONTE_G, PADRAO_COR_FONTE_B, PADRAO_COR_FONTE_A);

	int n;
	char buffer[8 * 2]; // 8 bytes x 2 dígitos hexadecimais/byte
	off_t off = hexv->fileoffset;
	size_t filesize = xchange_get_size(hexv->xf);
	for (n = hexv->font_size + hexv->view_offset; n < allocation.height
			+ hexv->font_size && off < filesize; n += hexv->font_size + LINE_SPACEMENT)
	{
		cairo_move_to(cr, MARGIN_OFFSET_TEXT, n);
		sprintf(buffer, "%08lX", off);
		cairo_show_text(cr, buffer);
		/*
		PangoLayout *layout = pango_cairo_create_layout (cr);
		pango_layout_set_font_description (layout, hexv->font_description);
		pango_layout_set_text (layout, buffer, -1);
		pango_cairo_show_layout_line(cr, pango_layout_get_line (layout, 0));
		g_object_unref(layout);
		*/
		off += hexv->byte_row_length;
	}

}

static GdkPoint offset_para_pontoxy(const XChangeHexView *hexv, off_t offset)
{
	GdkPoint p;
	gint character_width = hexv->character_width;
	gint position = offset - hexv->fileoffset;
	//	g_print("position: %i offset: %i file:%i \n", position, offset, hexv->fileoffset);
	gint cursor_i = position % hexv->byte_row_length;
	if (position < 0)
	{
		cursor_i = hexv->byte_row_length + cursor_i;
	}
	gint cursor_j = position / hexv->byte_row_length;
	if (position < 0)
		cursor_j--;

	p.x = (3 * cursor_i) * (character_width);
	if (hexv->show_offset_panel)
		p.x += hexv->offset_panel_width + OFFSET_PANEL_SPACEMENT;

	p.y = LINE_SPACEMENT / 2 + cursor_j * (hexv->font_size + LINE_SPACEMENT)
			+ hexv->view_offset;
	return p;
}

static GdkPoint cursor_offset_para_pontoxy(const XChangeHexView *hexv)
{
	GdkPoint p;
	p = offset_para_pontoxy(hexv, hexv->cursor_position);
	p.x += (hexv->cursor_position_half ? 1 : 0) * (hexv->character_width);
	return p;
}

static gboolean get_text_cursor_ij(const XChangeHexView *hexv, off_t offset, int *i, int *j)
{
	if (!XCHANGE_IS_HEX_VIEW(hexv))
		return FALSE;

	gint position = offset - hexv->fileoffset;
	size_t filesize = xchange_get_size(hexv->xf);
	if (offset == -1 || offset > filesize || filesize == -1)
		return FALSE;

	gint cursor_j = position / hexv->byte_row_length;
	if (position < 0)
		cursor_j--;

	gint cursor_i = position % hexv->byte_row_length;
	if (position < 0)
	{
		cursor_i = hexv->byte_row_length + cursor_i;
	}

	if (cursor_j >= 0 && cursor_j < hexv->lines_shown)
	{
		if (cursor_i < hexv->bytes_skipped[cursor_j])
		{
			cursor_i = hexv->byte_row_length-1; // FIXME!
			cursor_j --;
			cursor_i -= hexv->bytes_skipped[cursor_j];
		}
		else
		{
			cursor_i -= hexv->bytes_skipped[cursor_j];
		}
	}

/*	resp_texto = xchange_table_print_best_stringUTF8(hexv->xt,
									&hexv->bytes[cursor_j*hexv->byte_row_length + hexv->bytes_skipped[cursor_j]],
									max_size_to_read,
									NULL, min_size_to_read, &read);
*/
	if (i != NULL)
		*i = cursor_i;
	if (j != NULL)
		*j = cursor_j;
	return TRUE;
}

static char *get_line_text(const XChangeHexView *hexv, off_t offset, gboolean fullline, gint* length)
{
	if (!XCHANGE_IS_HEX_VIEW(hexv))
		return NULL;

	gint cursor_i, cursor_j;
	if (!get_text_cursor_ij(hexv, offset, &cursor_i, &cursor_j))
		return NULL;

//	g_print("offset: %i i: %i j: %i\n", offset, cursor_i, cursor_j);

	gint position = (offset - hexv->fileoffset)%(hexv->byte_row_length);
	size_t filesize = xchange_get_size(hexv->xf);
	if (cursor_j >=0 && position < hexv->byte_buffer_size)
	{
		int qtd_bytes_linha = hexv->byte_row_length - hexv->bytes_skipped[cursor_j];
		if (position + qtd_bytes_linha > filesize)
			qtd_bytes_linha = filesize - position;
		char *buffer_texto = malloc((hexv->byte_row_length) * 10); // FIXME!!! 10?!
		if (buffer_texto == NULL)
			return NULL;
// TODO: cursor_i não é útil?
		size_t max_size_to_read = filesize - (hexv->fileoffset + cursor_j*hexv->byte_row_length + hexv->bytes_skipped[cursor_j]);
		size_t min_size_to_read = fullline ? hexv->byte_row_length-hexv->bytes_skipped[cursor_j] : cursor_i;
		if (min_size_to_read > max_size_to_read)
			min_size_to_read = max_size_to_read;
		size_t read = 0;
		int resp_texto;
		// FIXME: Check if hexv->bytes[POS] is inside bounds
		gint pos_buffer = cursor_j*hexv->byte_row_length;
		if (cursor_j < hexv->lines_shown)
			pos_buffer += hexv->bytes_skipped[cursor_j];
		else
			g_print("Trouble coming...\n");
		if (pos_buffer > hexv->byte_buffer_size)
			g_print("Trouble...\n");
		if (fullline)
		{
			resp_texto = xchange_table_print_best_stringUTF8(hexv->xt,
								&hexv->bytes[pos_buffer],
								max_size_to_read,
								buffer_texto, min_size_to_read, &read);
		}
		else
			resp_texto = xchange_table_print_stringUTF8(hexv->xt,
								&hexv->bytes[pos_buffer],
								min_size_to_read,
								buffer_texto);

		if (resp_texto >= 0)
		{
			buffer_texto[resp_texto] = 0;
		}
		else
		{
			buffer_texto[0] = 0;
		}
		if (length != NULL)
			*length = resp_texto;
		return buffer_texto;
	}
	return NULL;
}

static gboolean get_text_dimention(const XChangeHexView *hexv, gboolean text_panel, const char *texto, cairo_text_extents_t* text_size)
{
	gboolean success = FALSE;
	cairo_t *cr;
	cairo_surface_t * surface = cairo_image_surface_create(CAIRO_FORMAT_A1, 0,
			0);
	if (cairo_surface_status(surface) == CAIRO_STATUS_SUCCESS)
	{
		cr = cairo_create(surface);
		if (cairo_status(cr) == CAIRO_STATUS_SUCCESS)
		{
			//cairo_set_font_size(cr, hexv->font_size);
			//cairo_select_font_face(cr, hexv->font_face, CAIRO_FONT_SLANT_NORMAL, 0);//CAIRO_FONT_WEIGHT_BOLD);
			PangoLayout *layout = pango_cairo_create_layout (cr);
			pango_layout_set_font_description (layout, text_panel ? hexv->text_font_description : hexv->font_description);
			pango_layout_set_single_paragraph_mode(layout, TRUE);
			pango_layout_set_text (layout, texto, -1);
			PangoRectangle ink, logical;
			pango_layout_get_pixel_extents(layout, &ink, &logical);
			text_size->x_bearing = PANGO_LBEARING(ink);
			text_size->y_bearing = PANGO_ASCENT(ink);
			text_size->width = logical.width; // FIXME
			text_size->height = logical.height; // FIXME
			text_size->x_advance = PANGO_RBEARING(logical);
			text_size->y_advance = PANGO_DESCENT(logical);
			g_object_unref (layout);
			//cairo_text_extents(cr, texto, text_size);
			cairo_destroy(cr);
			success = TRUE;
		}
		cairo_surface_destroy(surface);
	}
	return success;
}

static gchar *replace_null_char(const XChangeHexView *hexv, const char *texto, int length, int *new_length)
{
	gchar *novo_texto;
	gint novo_comprimento;
	gint comprimento_null;

	if (!XCHANGE_IS_HEX_VIEW(hexv))
		return NULL;
	if (texto == NULL)
		return NULL;

	// Descobre novo comprimento
	const gchar *replacer = hexv->null_char_replacer == NULL? VISUAL_CARACTERE_NULO : hexv->null_char_replacer;
	comprimento_null = strlen(replacer);
	novo_comprimento = length;
	int n;
	for (n = 0; n < length; n++)
		if (texto[n]==0)
			novo_comprimento+=comprimento_null - 1;

	// Aloca espaço para o novo texto
	novo_texto = g_try_malloc0(novo_comprimento+1);
	if (novo_texto == NULL)
		return NULL;

	// Copia substituindo
	int d = 0;
	for (n = 0; n < length; n++)
	{
		if (texto[n]==0)
		{
			memcpy(&novo_texto[d], replacer, comprimento_null);
			d += comprimento_null;
		}
		else
			novo_texto[d++] = texto[n];
	}

	if (new_length != NULL)
		*new_length = novo_comprimento+1;
	return novo_texto;
}

static gboolean get_line_text_size(const XChangeHexView *hexv, off_t offset, gboolean fullline, cairo_text_extents_t* text_size)
{
	gboolean success = FALSE;
	gint length;
	// FIXME: Check if hexv->bytes[POS] is inside bounds
	char *texto = get_line_text(hexv, offset, fullline, &length);
	if (texto == NULL)
		return FALSE;

	char *novo_texto = replace_null_char(hexv, texto, length, NULL);
	g_free(texto);
	if (novo_texto == NULL)
		return FALSE;

	success = get_text_dimention(hexv, TRUE, novo_texto, text_size);
	g_free(novo_texto);
	return success;
}

/**
 * Posição do arquivo para coordenadas no widget
 *
 * Se @offset estiver fora do campo visual, aproxima para o caso de uma fonte monoespaçada
 *  e que a carreira de bytes corresponda ao texto.
 */
static GdkPoint offset_para_pontoxy_texto(const XChangeHexView *hexv, off_t offset)
{
	GdkPoint p;
	p.x = 0;
	p.y = 0;
	if (!XCHANGE_IS_HEX_VIEW(hexv))
		return p;


	if (offset > hexv->fileoffset + hexv->byte_buffer_size)
	{
		// Out of bounds of interest...
		// Just thinking it's a monospaced font and the entire row of bytes is the row of text
		p = offset_para_pontoxy(hexv, offset);
		p.x += TEXT_SPACEMENT + hexv->byte_panel_width;
		return p;
		/*
		gint cursor_j;
		cursor_j = offset - hexv->fileoffset;
		cursor_j %= (hexv->byte_row_length);

		p.y = LINE_SPACEMENT / 2 + cursor_j * (hexv->font_size + LINE_SPACEMENT)
				+ hexv->view_offset;
		return p; // TODO: Return (-1,-1)?
		*/
	}

	cairo_text_extents_t text_size;
	if (get_line_text_size(hexv,offset,FALSE, &text_size))
	{
		p.x = text_size.x_advance;
	}
	else
	{
		p.x = 0;
	}


	if (hexv->show_offset_panel)
		p.x += hexv->offset_panel_width + OFFSET_PANEL_SPACEMENT;
	p.x += hexv->byte_panel_width + TEXT_SPACEMENT;

	gint cursor_i, cursor_j;
	if (!get_text_cursor_ij(hexv, offset, &cursor_i, &cursor_j))
		return p;


	p.y = LINE_SPACEMENT / 2 + cursor_j * (hexv->font_size + LINE_SPACEMENT)
			+ hexv->view_offset;
	return p;
}

static GdkPoint get_next_text_cursor_position(const XChangeHexView *hexv, off_t atual)
{
	GdkPoint pi, po;
	off_t proximo_offset = atual;
	size_t filesize = xchange_get_size(hexv->xf);
	/*po.x = -1;
	po.y = -1;
	if (filesize == (size_t)-1)
		return po;
		*/
	pi = offset_para_pontoxy_texto(hexv, atual);
	do
	{
		po = offset_para_pontoxy_texto(hexv, proximo_offset++);
	}while(po.x == pi.x && proximo_offset <= filesize);
	return po;
}

static GdkRectangle cursor_offset_para_retangulo_texto(const XChangeHexView *hexv)
{
	GdkRectangle r;
	off_t offset = hexv->cursor_position;
	GdkPoint pt = offset_para_pontoxy_texto(XCHANGE_HEX_VIEW(hexv), offset);
	// Search for previous point (locating starting byte)
	if (offset > 0)
	{
		off_t offset_anterior = offset;
		GdkPoint pt_anterior;
		do
		{
			offset_anterior --;
			pt_anterior = offset_para_pontoxy_texto(XCHANGE_HEX_VIEW(hexv), offset_anterior);
		} while (pt_anterior.x == pt.x && pt_anterior.y == pt.y);
// FIXME: poisção cursor texto
//		g_print("Anterior: %lu Atual: %lu\n", offset_anterior, offset);
		if (offset_anterior != offset)
		{
			offset = offset_anterior +1;
			pt = offset_para_pontoxy_texto(XCHANGE_HEX_VIEW(hexv), offset);
		}
	}
	GdkPoint pt1 = offset_para_pontoxy_texto(XCHANGE_HEX_VIEW(hexv), offset+1);
	double tw = hexv->character_width;
	GdkPoint pt2 = get_next_text_cursor_position(hexv, offset);

	if (offset != 0)
		if (pt1.x == pt.x && pt2.y == pt.y)
		{
			pt2 = pt;
			pt = offset_para_pontoxy_texto(XCHANGE_HEX_VIEW(hexv), offset-1);
			offset--;
		}

	if (pt2.x > pt.x)
		tw = pt2.x - pt.x;
	else
	{
		cairo_text_extents_t text_size;
		if (get_line_text_size(hexv,offset,TRUE, &text_size))
		{
			tw = 0;
			if (hexv->show_offset_panel)
				tw += hexv->offset_panel_width + OFFSET_PANEL_SPACEMENT;
			tw += hexv->byte_panel_width + TEXT_SPACEMENT;

			tw += text_size.x_advance;

			tw -= pt.x;
			//g_print("\twidth= %.2lf tw=%.2lf pt.x=%i\n", text_size.x_advance, tw, pt.x);
		}
		else
			tw = hexv->virtual_w - pt.x;

		if (tw < 0)
			tw =hexv->character_width;
	}
	//g_print(" tw=%.2lf pt.x=%i\n", tw, pt.x);
	r.x = pt.x;
	r.y = pt.y;
	r.width = tw;
	r.height = hexv->font_size;
	return r;
}

static void draw_selection(GtkWidget *widget, cairo_t *cr)
{
	XChangeHexView *hexv = XCHANGE_HEX_VIEW(widget);

	off_t menor, maior;
	if (!xchange_hex_view_get_selection_bounds(hexv, &menor, &maior))
		return;

	if (maior < hexv->fileoffset || hexv->fileoffset + hexv->byte_buffer_size < menor)
		return;

	GdkPoint p1 = offset_para_pontoxy(hexv, menor);
	GdkPoint p2 = offset_para_pontoxy(hexv, maior);
	gint ch = hexv->font_size;

	GdkPoint p1t = offset_para_pontoxy_texto(hexv, menor);
	GdkPoint p2t = offset_para_pontoxy_texto(hexv, maior);
	GdkPoint p2tb = get_next_text_cursor_position(hexv, maior);

	gdouble cor_r, cor_g, cor_b, cor_a;
	if (cairo_pattern_get_rgba (hexv->selection_color, &cor_r, &cor_g, &cor_b, &cor_a) == CAIRO_STATUS_SUCCESS)
		cairo_set_source_rgba(cr, cor_r, cor_g, cor_b, cor_a);
	else
		cairo_set_source_rgba(cr, PADRAO_COR_SELECAO_R, PADRAO_COR_SELECAO_G, PADRAO_COR_SELECAO_B, PADRAO_COR_SELECAO_A);
	if (p1.y == p2.y)
	{
		// Same line
		cairo_rectangle(cr, p1.x, p1.y,
				p2.x - p1.x + hexv->character_width * 2, ch);
		cairo_fill(cr);

		// Text
		if (p2tb.y == p2t.y)
		{
			if (p2t.x != p1t.x)
				cairo_rectangle(cr, p1t.x, p1t.y,
						p2t.x - p1t.x, ch); // TODO: Acrescentar largura do byte selecionado...
			else
				cairo_rectangle(cr, p1t.x, p1t.y,
						p2tb.x - p1t.x, ch); // TODO: Acrescentar largura do byte selecionado...
		}
		else
			cairo_rectangle(cr, p1t.x, p1t.y,
						hexv->virtual_w - p1t.x, ch); // TODO: Acrescentar largura do byte selecionado...
		cairo_fill(cr);
	}
	else
	{
		// First line
		if (p1.y + ch >= 0)
		{
			cairo_rectangle(cr, p1.x, p1.y, hexv->character_width * 3
					* (hexv->byte_row_length - ((menor - hexv->fileoffset)
							% hexv->byte_row_length)) - hexv->character_width
					/ 2, ch);
			cairo_fill(cr);

			// TODO: Text selection first line (precisa criar função cursor->ponto no texto e vice-versa)
			// Lembrar que deve ignorar os bytes ignorados da linha
			cairo_rectangle(cr, p1t.x, p1t.y, hexv->virtual_w-p1t.x, ch);
						cairo_fill(cr);
		}
		int left = OFFSET_PANEL_SPACEMENT +
				(hexv->show_offset_panel ? hexv->offset_panel_width : 0);

		// Intermediate lines
		if (p2.y - p1.y > hexv->font_size + LINE_SPACEMENT)
		{
			cairo_rectangle(cr, left, p1.y + hexv->font_size,
					hexv->byte_panel_width
							- hexv->character_width / 2, p2.y - p1.y
							- hexv->font_size - LINE_SPACEMENT);
			cairo_fill(cr);

			// Text selection
			cairo_rectangle(cr, left + hexv->byte_panel_width + TEXT_SPACEMENT, p1.y + hexv->font_size,
								hexv->virtual_w - (left + hexv->byte_panel_width + TEXT_SPACEMENT), p2.y - p1.y
										- hexv->font_size - LINE_SPACEMENT);
			cairo_fill(cr);
		}

		// Last line
		cairo_rectangle(cr, left, p2.y - LINE_SPACEMENT, p2.x + 2
				* hexv->character_width - left, ch + LINE_SPACEMENT);
		cairo_fill(cr);
		// TODO: Text selection last line (precisa criar função cursor->ponto no texto e vice-versa)
		// Lembrar que deve ignorar os bytes ignorados da linha
		if (p2tb.y == p2t.y)
			cairo_rectangle(cr, left + hexv->byte_panel_width + TEXT_SPACEMENT, p2t.y - LINE_SPACEMENT, p2tb.x - (left + hexv->byte_panel_width + TEXT_SPACEMENT), ch + LINE_SPACEMENT);// TODO: Acrescentar largura do byte selecionado...
		else
			cairo_rectangle(cr, left + hexv->byte_panel_width + TEXT_SPACEMENT, p2t.y - LINE_SPACEMENT, hexv->virtual_w - (left + hexv->byte_panel_width + TEXT_SPACEMENT), ch + LINE_SPACEMENT);// TODO: Acrescentar largura do byte selecionado...
		cairo_fill(cr);
	}
}

static void draw_cursor(GtkWidget *widget, cairo_t *cr)
{
	// Draw cursor
	XChangeHexView *hexv = XCHANGE_HEX_VIEW(widget);
	GtkAllocation allocation;
	gtk_widget_get_allocation(widget, &allocation);
	GdkPoint p = cursor_offset_para_pontoxy(hexv);


	if (p.y < 0-hexv->font_size || p.y > allocation.height)
		return;

	gint cw = hexv->character_width + 1;
	gint ch = hexv->font_size;

	if (hexv->overwrite)
	{
		gdouble cor_r, cor_g, cor_b, cor_a;
		if (cairo_pattern_get_rgba (hexv->cursor_background_color, &cor_r, &cor_g, &cor_b, &cor_a) == CAIRO_STATUS_SUCCESS)
			cairo_set_source_rgba(cr, cor_r, cor_g, cor_b, cor_a);
		else
			cairo_set_source_rgba(cr, PADRAO_COR_FUNDO_CURSOR_R, PADRAO_COR_FUNDO_CURSOR_G, PADRAO_COR_FUNDO_CURSOR_B, PADRAO_COR_FUNDO_CURSOR_A);
		cairo_rectangle(cr, p.x, p.y, cw, ch);
		cairo_fill_preserve(cr);
		if (cairo_pattern_get_rgba (hexv->cursor_foreground_color, &cor_r, &cor_g, &cor_b, &cor_a) == CAIRO_STATUS_SUCCESS)
			cairo_set_source_rgba(cr, cor_r, cor_g, cor_b, cor_a);
		else
			cairo_set_source_rgba(cr, PADRAO_COR_CONTORNO_CURSOR_R, PADRAO_COR_CONTORNO_CURSOR_G, PADRAO_COR_CONTORNO_CURSOR_B, PADRAO_COR_CONTORNO_CURSOR_A);
		cairo_stroke(cr);

		// Text cursor
		// TODO: text cursor
		// Lembrar que deve ignorar os bytes ignorados da linha
		if (hexv->show_text_panel)
		{
			GdkRectangle rt = cursor_offset_para_retangulo_texto(hexv);
			if (cairo_pattern_get_rgba (hexv->cursor_background_color, &cor_r, &cor_g, &cor_b, &cor_a) == CAIRO_STATUS_SUCCESS)
				cairo_set_source_rgba(cr, cor_r, cor_g, cor_b, cor_a);
			else
				cairo_set_source_rgba(cr, PADRAO_COR_FUNDO_CURSOR_R, PADRAO_COR_FUNDO_CURSOR_G, PADRAO_COR_FUNDO_CURSOR_B, PADRAO_COR_FUNDO_CURSOR_A);
			cairo_rectangle(cr, rt.x, rt.y, rt.width, rt.height);
			cairo_fill_preserve(cr);
			if (cairo_pattern_get_rgba (hexv->cursor_foreground_color, &cor_r, &cor_g, &cor_b, &cor_a) == CAIRO_STATUS_SUCCESS)
				cairo_set_source_rgba(cr, cor_r, cor_g, cor_b, cor_a);
			else
				cairo_set_source_rgba(cr, PADRAO_COR_CONTORNO_CURSOR_R, PADRAO_COR_CONTORNO_CURSOR_G, PADRAO_COR_CONTORNO_CURSOR_B, PADRAO_COR_CONTORNO_CURSOR_A);
			cairo_stroke(cr);
		}
	}
	else
	{
		if (!hexv->cursor_in_blink)
		{
			cairo_set_source_rgba(cr, PADRAO_COR_CONTORNO_CURSOR_R, PADRAO_COR_CONTORNO_CURSOR_G, PADRAO_COR_CONTORNO_CURSOR_B, PADRAO_COR_CONTORNO_CURSOR_A);
			cairo_move_to(cr, p.x, p.y);
			cairo_line_to(cr, p.x, p.y + ch);
			cairo_stroke(cr);

			// Text cursor
			// TODO: text cursor
			// Lembrar que deve ignorar os bytes ignorados da linha
			if (hexv->show_text_panel)
			{
				GdkPoint pt = offset_para_pontoxy_texto(hexv, hexv->cursor_position); // FIXME: função própria
				cairo_move_to(cr, pt.x, pt.y);
				cairo_line_to(cr, pt.x, pt.y + ch);
				cairo_stroke(cr);
			}
		}
	}
}

static void draw(GtkWidget *widget, cairo_t *cr)
{
	XChangeHexView *hexv = XCHANGE_HEX_VIEW(widget);
	GtkAllocation allocation;
	gtk_widget_get_allocation(widget, &allocation);

	gdouble cor_r, cor_g, cor_b, cor_a;

	if (hexv->xf == NULL || xchange_get_size(hexv->xf) == (size_t)-1)
		return;
	float offset_width = 0;
	cairo_set_font_size(cr, hexv->font_size);
	//cairo_select_font_face(cr, hexv->font_face, CAIRO_FONT_SLANT_NORMAL, 0);//CAIRO_FONT_WEIGHT_BOLD);

	cairo_translate(cr, -gtk_adjustment_get_value(hexv->hadjustment), 0);

	if (hexv->show_offset_panel)
	{
		draw_offset_panel(widget, cr);
		offset_width = hexv->offset_panel_width;
	}

	// Draw Byte panel background
	cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
	cairo_rectangle(cr, offset_width + OFFSET_PANEL_SPACEMENT, 0, allocation.width
			- offset_width + gtk_adjustment_get_value(hexv->hadjustment),
			allocation.height);
	cairo_fill(cr);

	// Draw selection
	if (hexv->selection_start != -1)
		draw_selection(widget, cr);

	// Draw cursor
	if (hexv->editable || hexv->cursor_visible)
	{
		draw_cursor(widget, cr);
	}

	// Font color
	if (cairo_pattern_get_rgba (hexv->font_color, &cor_r, &cor_g, &cor_b, &cor_a) == CAIRO_STATUS_SUCCESS)
		cairo_set_source_rgba(cr, cor_r, cor_g, cor_b, cor_a);
	else
		cairo_set_source_rgba(cr, PADRAO_COR_FONTE_R, PADRAO_COR_FONTE_G, PADRAO_COR_FONTE_B, PADRAO_COR_FONTE_A);


	double max_x = 0.0, max_y = 0.0;

	if (hexv->xf != NULL)
	{
		off_t off = 0;
		int n;

		int text_bytes_to_skip = 0;

		char buffer[hexv->byte_row_length * 3];
		size_t filesize = xchange_get_size(hexv->xf);

		gint altura = allocation.height;
//		gdk_drawable_get_size(gtk_widget_get_parent_window(
//				gtk_widget_get_parent(widget)), NULL, &altura);

		int linha_n = 0;
		int left_x = offset_width != 0 ? offset_width + OFFSET_PANEL_SPACEMENT : 0;

		PangoLayout *layout = pango_cairo_create_layout (cr);
		pango_layout_set_font_description (layout, hexv->font_description);
		pango_layout_set_single_paragraph_mode(layout, TRUE);

		PangoLayout *text_layout = pango_cairo_create_layout (cr);
		pango_layout_set_font_description (text_layout, hexv->text_font_description);
		pango_layout_set_single_paragraph_mode(text_layout, TRUE);

		for (n = hexv->font_size + hexv->view_offset; n < altura
				+ hexv->font_size; n += hexv->font_size + LINE_SPACEMENT)
		{
			int c;
			for (c = 0; c < hexv->byte_row_length && c + off + hexv->fileoffset < filesize; c++)
			{
				// fixme: "byte" width
				cairo_move_to(cr, left_x + c
						* (hexv->character_width * 3), n);
				if (c + off + hexv->fileoffset < filesize)
				{
					sprintf(buffer, "%02hhX", hexv->bytes[c + off]);
					//cairo_show_text(cr, buffer);
					pango_layout_set_text (layout, buffer, -1);
					//pango_cairo_show_layout (cr, layout);
					pango_cairo_show_layout_line (cr, pango_layout_get_line (layout, 0));
				}
			}
			// Desenha texto equivalente
			if (hexv->show_text_panel)
			{
				cairo_move_to(cr, left_x
						+ hexv->byte_panel_width + TEXT_SPACEMENT, n);
				int qtd_bytes_linha = hexv->byte_row_length - text_bytes_to_skip;
				if (hexv->fileoffset + off + hexv->byte_row_length >= xchange_get_size(hexv->xf))
					qtd_bytes_linha = xchange_get_size(hexv->xf) - (hexv->fileoffset + off);
				int largest_entry = xchange_table_get_largest_entry_length(hexv->xt, FALSE);
				if (largest_entry <= 0)
					largest_entry = 4;
				char buffer_texto[qtd_bytes_linha * largest_entry];
				//buffer_texto[0] = 0;
				//memset(buffer_texto, 0, qtd_bytes_linha*5);

				size_t lidos;
				int resp_texto = xchange_table_print_best_stringUTF8(hexv->xt,
						&hexv->bytes[off + text_bytes_to_skip], qtd_bytes_linha
								>= xchange_get_size(hexv->xf)
								- (hexv->fileoffset + off) ? qtd_bytes_linha
								: xchange_get_size(hexv->xf)
										- (hexv->fileoffset + off),
						buffer_texto, qtd_bytes_linha, &lidos);

				//g_print("resp: %i lidos %zi texto: %s\n", resp_texto, lidos, buffer_texto);
				if (linha_n >= hexv->lines_shown)
					g_print("Estranho... Número de linhas (%i, era para ser menor que %i!\n", linha_n, hexv->lines_shown);
				else
				{
					hexv->bytes_skipped[linha_n] = text_bytes_to_skip;
				}
				if (resp_texto >= 0)
				{
					text_bytes_to_skip = lidos - qtd_bytes_linha;
					buffer_texto[resp_texto] = 0;
				}
				else
					buffer_texto[0] = 0;

				// Substitui bytes nulos para algo visual...
				gint novo_resp_texto;
				gchar *novo_texto = replace_null_char(hexv, buffer_texto, resp_texto, &novo_resp_texto);
				/*
				int it;
				for (it = 0; it < resp_texto; it++)
					if (buffer_texto[it]==0)
						buffer_texto[it]=VISUAL_CARACTERE_NULO;
				*/

				//if (resp_texto > 0)
				/*
				cairo_set_source_rgb(cr, 0, 0, 0);
				cairo_rectangle(cr, 0, 0, hexv->bytes_skipped[linha_n]*hexv->character_width, hexv->font_size);
				cairo_set_source_rgb(cr, 0, 0, 0);
				cairo_move_to(cr, left_x + hexv->bytes_skipped[linha_n]*hexv->character_width + hexv->byte_panel_width + TEXT_SPACEMENT, n);
				*/

				//cairo_show_text(cr, buffer_texto);
				pango_layout_set_text (text_layout, novo_texto, novo_resp_texto);
				g_free(novo_texto);
				//pango_cairo_show_layout (cr, layout);
				pango_cairo_show_layout_line (cr, pango_layout_get_line (text_layout, 0));

				/*
				if (cairo_has_current_point(cr))
				{
					double x, y;
					cairo_get_current_point(cr, &x, &y);
					g_print("current point: %lf,%lf\n", x, y);
				}
				else
					g_print("no current point\n");
*/
			}
				// Data for virtual dimension size computation
				double x, y;
				int w;
				cairo_get_current_point(cr, &x, &y);
				pango_layout_get_pixel_size(text_layout, &w, NULL);
				if (max_x < x + w)
					max_x = x + w;
				if (max_y < y)
					max_y = y;
			//}

			linha_n ++;
			off += hexv->byte_row_length;
			if (hexv->fileoffset + off >= filesize)
				break;
		}
		g_object_unref (layout);
		g_object_unref (text_layout);


		// Computes virtual dimension sizes
		glong nlines = filesize / hexv->byte_row_length;
		if (filesize % hexv->byte_row_length != 0)
			nlines++;
		glong pixels_h = nlines * (hexv->font_size + LINE_SPACEMENT);
		hexv->virtual_w = max_x + 5;
		hexv->virtual_h = pixels_h;
		//gtk_widget_set_size_request(widget, max_x + 5/*offset_width +  5 + hexv->byte_row_length * (hexv->character_width
		//		* 3)*/, 100);
	}

}

static void font_dimention_changed(XChangeHexView *hexv)
{
	// Computes character width
	cairo_text_extents_t text_dimentions;
	get_text_dimention(hexv, FALSE, "99999999", &text_dimentions);
	/*
	cairo_t *cr;
	cairo_surface_t * surface = cairo_image_surface_create(CAIRO_FORMAT_A1, 0, 0);
	cr = cairo_create(surface);


	cairo_set_font_size(cr, hexv->font_size);
	cairo_select_font_face(cr, hexv->font_face, CAIRO_FONT_SLANT_NORMAL, 0);//CAIRO_FONT_WEIGHT_BOLD);
	cairo_text_extents(cr, "99999999", &text_dimentions);

	cairo_surface_destroy(surface);
	cairo_destroy(cr);
	*/
	hexv->character_width = text_dimentions.x_advance / 8;

	// Computes offset panel width
	hexv->offset_panel_width = MARGIN_OFFSET_TEXT + text_dimentions.width + 2
			+ MARGIN_OFFSET_TEXT;

	// Computes byte panel width
	hexv->byte_panel_width = (hexv->byte_row_length) * (hexv->character_width * 3);

	// Computes number of lines shown
	compute_lines_shown(hexv);
	calcula_tamanho_buffer(hexv);
	gtk_widget_queue_draw(GTK_WIDGET(hexv));
}

void xchange_hex_view_set_font_size(XChangeHexView *xchange_hex_view,
		gint font_size)
{
	if (!XCHANGE_IS_HEX_VIEW(xchange_hex_view))
		return;

	XChangeHexView *hexv = XCHANGE_HEX_VIEW(xchange_hex_view);

	if (font_size <= 0)
		font_size = PADRAO_TAMANHO_FONTE;

	hexv->font_size = font_size;

	pango_font_description_set_absolute_size (hexv->font_description, hexv->font_size * PANGO_SCALE);
	pango_font_description_set_absolute_size (hexv->text_font_description, hexv->font_size * PANGO_SCALE);

	font_dimention_changed(hexv);
}

void xchange_hex_view_set_byte_font_name(XChangeHexView *xchange_hex_view, const gchar *font_name)
{
	if (!XCHANGE_IS_HEX_VIEW(xchange_hex_view))
		return;

	XChangeHexView *hexv = XCHANGE_HEX_VIEW(xchange_hex_view);

	if (font_name == NULL)
	{
		font_name = PADRAO_NOME_FONTE;
	}
	// Confere se é diferente
	if (g_strcmp0(hexv->font_face, font_name) != 0)
	{
		// FIXME: Verificar se é monospaced
		//pango_context_list_families();
		//if (pango_font_family_is_monospace())
		{
			g_free(hexv->font_face);
			hexv->font_face = g_strdup(font_name); // FIXME: Duplicate?
			pango_font_description_set_family (hexv->font_description, hexv->font_face);
			font_dimention_changed(hexv); // FIXME: Inject font_description set_family?
		}
	}

}

void xchange_hex_view_set_text_font_name(XChangeHexView *xchange_hex_view, const gchar *font_name)
{
	if (!XCHANGE_IS_HEX_VIEW(xchange_hex_view))
		return;

	XChangeHexView *hexv = XCHANGE_HEX_VIEW(xchange_hex_view);

	if (font_name == NULL)
	{
		font_name = PADRAO_NOME_FONTE;
	}
	// Confere se é diferente
	if (g_strcmp0(hexv->text_font_face, font_name) != 0)
	{
		g_free(hexv->text_font_face);
		hexv->text_font_face = g_strdup(font_name); // FIXME: Duplicate?
		pango_font_description_set_family (hexv->text_font_description, hexv->text_font_face);
		font_dimention_changed(hexv); // FIXME: Inject font_description set_family?
	}
}

static cairo_pattern_t *converte_cor_cairo(const GdkColor *color, guint16 alpha)
{
	return cairo_pattern_create_rgba((gdouble)color->red/65536.0f,
			(gdouble)color->green/65536.0f, (gdouble)color->blue/65536.0f, (gdouble)alpha/65536.0f);
}

void xchange_hex_view_set_font_color(XChangeHexView *xchange_hex_view, GdkColor color, guint16 alpha)
{
	if (!XCHANGE_IS_HEX_VIEW(xchange_hex_view))
		return;

	cairo_pattern_destroy(xchange_hex_view->font_color);
	xchange_hex_view->font_color = converte_cor_cairo(&color, alpha);
	cairo_pattern_reference (xchange_hex_view->font_color);
	gtk_widget_queue_draw(GTK_WIDGET(xchange_hex_view));
}

void xchange_hex_view_set_font_color_default(XChangeHexView *xchange_hex_view)
{
	if (!XCHANGE_IS_HEX_VIEW(xchange_hex_view))
		return;

	cairo_pattern_destroy(xchange_hex_view->font_color);
	xchange_hex_view->font_color = cairo_pattern_create_rgba(PADRAO_COR_FONTE_R,
				PADRAO_COR_FONTE_G, PADRAO_COR_FONTE_B, PADRAO_COR_FONTE_A);
	cairo_pattern_reference (xchange_hex_view->font_color);
	gtk_widget_queue_draw(GTK_WIDGET(xchange_hex_view));
}

void xchange_hex_view_set_cursor_foreground_color(XChangeHexView *xchange_hex_view, GdkColor color, guint16 alpha)
{
	if (!XCHANGE_IS_HEX_VIEW(xchange_hex_view))
		return;

	cairo_pattern_destroy(xchange_hex_view->cursor_foreground_color);
	xchange_hex_view->cursor_foreground_color = converte_cor_cairo(&color, alpha);
	cairo_pattern_reference (xchange_hex_view->cursor_foreground_color);
	gtk_widget_queue_draw(GTK_WIDGET(xchange_hex_view));
}

void xchange_hex_view_set_cursor_foreground_color_default(XChangeHexView *xchange_hex_view)
{
	if (!XCHANGE_IS_HEX_VIEW(xchange_hex_view))
		return;

	cairo_pattern_destroy(xchange_hex_view->cursor_foreground_color);
	xchange_hex_view->cursor_foreground_color = cairo_pattern_create_rgba(PADRAO_COR_CONTORNO_CURSOR_R,
			PADRAO_COR_CONTORNO_CURSOR_G, PADRAO_COR_CONTORNO_CURSOR_B, PADRAO_COR_CONTORNO_CURSOR_A);
	cairo_pattern_reference (xchange_hex_view->cursor_foreground_color);
	gtk_widget_queue_draw(GTK_WIDGET(xchange_hex_view));
}

void xchange_hex_view_set_cursor_background_color(XChangeHexView *xchange_hex_view, GdkColor color, guint16 alpha)
{
	if (!XCHANGE_IS_HEX_VIEW(xchange_hex_view))
		return;

	cairo_pattern_destroy(xchange_hex_view->cursor_background_color);
	xchange_hex_view->cursor_background_color = converte_cor_cairo(&color, alpha);
	cairo_pattern_reference (xchange_hex_view->cursor_background_color);
	gtk_widget_queue_draw(GTK_WIDGET(xchange_hex_view));
}

void xchange_hex_view_set_cursor_background_color_default(XChangeHexView *xchange_hex_view)
{
	if (!XCHANGE_IS_HEX_VIEW(xchange_hex_view))
		return;

	cairo_pattern_destroy(xchange_hex_view->cursor_background_color);
	xchange_hex_view->cursor_background_color = cairo_pattern_create_rgba(PADRAO_COR_FUNDO_CURSOR_R,
			PADRAO_COR_FUNDO_CURSOR_G, PADRAO_COR_FUNDO_CURSOR_B, PADRAO_COR_FUNDO_CURSOR_A);
	cairo_pattern_reference (xchange_hex_view->cursor_background_color);
	gtk_widget_queue_draw(GTK_WIDGET(xchange_hex_view));
}

void xchange_hex_view_set_selection_color(XChangeHexView *xchange_hex_view, GdkColor color, guint16 alpha)
{
	if (!XCHANGE_IS_HEX_VIEW(xchange_hex_view))
		return;

	cairo_pattern_destroy(xchange_hex_view->selection_color);
	xchange_hex_view->selection_color = converte_cor_cairo(&color, alpha);
	cairo_pattern_reference (xchange_hex_view->selection_color);
	gtk_widget_queue_draw(GTK_WIDGET(xchange_hex_view));
}

void xchange_hex_view_set_selection_color_default(XChangeHexView *xchange_hex_view)
{
	if (!XCHANGE_IS_HEX_VIEW(xchange_hex_view))
		return;

	cairo_pattern_destroy(xchange_hex_view->selection_color);
	xchange_hex_view->selection_color = cairo_pattern_create_rgba(PADRAO_COR_SELECAO_R,
			PADRAO_COR_SELECAO_G, PADRAO_COR_SELECAO_B, PADRAO_COR_SELECAO_A);
	cairo_pattern_reference (xchange_hex_view->selection_color);
	gtk_widget_queue_draw(GTK_WIDGET(xchange_hex_view));
}

static gboolean pisca_cursor(gpointer data)
{
	XChangeHexView *hexv = XCHANGE_HEX_VIEW(data);
	if (hexv->overwrite)
		return FALSE;
	hexv->cursor_in_blink = !hexv->cursor_in_blink;
	gtk_widget_queue_draw(GTK_WIDGET(hexv)); // FIXME _area
	return TRUE;
}

static gboolean avanca_cursor(XChangeHexView *hexv, int qtd, gboolean ignora_half)
{
	off_t cursor_inicial = hexv->cursor_position;
	off_t novo_cursor = hexv->cursor_position;
	size_t filesize = xchange_get_size(hexv->xf);
	if (filesize == (size_t)-1 || !filesize)
		return FALSE;
	if (!ignora_half)
	{
		if (hexv->cursor_position_half || !hexv->overwrite)
		{
			if (hexv->cursor_position < filesize)
			{
				novo_cursor++;
				hexv->cursor_position_half = FALSE;
			}

		}
		else if (hexv->overwrite && !hexv->cursor_position_half)
		{
			if (hexv->cursor_position < filesize)
			{
				hexv->cursor_position_half = TRUE;
			}
		}
	}
	else
	{
		novo_cursor += qtd;
		if (novo_cursor >= filesize)
			novo_cursor = filesize - 1;
		hexv->cursor_position_half = FALSE;
	}

	GtkAllocation allocation;
	gtk_widget_get_allocation(GTK_WIDGET(hexv), &allocation);
	int altura = allocation.height - (hexv->view_offset);
	int qtd_linhas_bytes = altura / (hexv->font_size + LINE_SPACEMENT);
	if (altura % ((int) hexv->font_size + LINE_SPACEMENT) == 0)
		qtd_linhas_bytes++;

	if (novo_cursor >= hexv->fileoffset + qtd_linhas_bytes
			* hexv->byte_row_length)
	{
		//xchange_hex_view_goto(hexv, hexv->cursor_position);
		int lines_to_go = (novo_cursor - cursor_inicial)/(hexv->byte_row_length);
		if (hexv->vadjustment != NULL)
			gtk_adjustment_set_value(hexv->vadjustment, gtk_adjustment_get_value(hexv->vadjustment) + lines_to_go*(hexv->font_size + LINE_SPACEMENT));
		else
			;//TODO: Se não houver barra de rolagem vertical...
	}

	// Rolagem horizontal...
	if (hexv->hadjustment != NULL)
	{
		int new_xscroll = ((novo_cursor - hexv->fileoffset) % hexv->byte_row_length) * hexv->character_width * 3;

		if (hexv->show_offset_panel)
			new_xscroll += hexv->offset_panel_width + OFFSET_PANEL_SPACEMENT;
		gdouble current_xscroll = gtk_adjustment_get_value(hexv->hadjustment);

		if (new_xscroll < current_xscroll || (new_xscroll + hexv->character_width*(3-1)) - current_xscroll > allocation.width)
			gtk_adjustment_set_value(hexv->hadjustment, new_xscroll);
	}


	hexv->cursor_position = novo_cursor;
	if (cursor_inicial != novo_cursor)
		g_signal_emit_by_name(hexv, "cursor-moved");

	return cursor_inicial != novo_cursor;
}

static gboolean retrocede_cursor(XChangeHexView *hexv, int qtd,
		gboolean ignora_half)
{
	off_t cursor_inicial = hexv->cursor_position;
	if (!ignora_half)
	{
		if (!hexv->cursor_position_half)
		{
			if (hexv->cursor_position > 0)
			{
				hexv->cursor_position--;
				if (hexv->overwrite)
					hexv->cursor_position_half = TRUE;
			}
		}
		else if (hexv->overwrite && hexv->cursor_position_half)
			hexv->cursor_position_half = FALSE;
	}
	else
	{
		hexv->cursor_position -= qtd;
		if (hexv->cursor_position < 0)
			hexv->cursor_position = 0;
		hexv->cursor_position_half = FALSE;
	}

	if (hexv->cursor_position < hexv->fileoffset || ( (hexv->cursor_position < hexv->fileoffset + hexv->byte_row_length) && hexv->view_offset != 0))
	{
		//xchange_hex_view_goto(hexv, hexv->cursor_position);
		int lines_to_go = (cursor_inicial - hexv->cursor_position)/(hexv->byte_row_length);
		if (hexv->vadjustment != NULL)
			gtk_adjustment_set_value(hexv->vadjustment, gtk_adjustment_get_value(hexv->vadjustment) - lines_to_go*(hexv->font_size + LINE_SPACEMENT));
		else
			; // TODO: Rolagem vertical se não houver barras de rolagem
	}

	// Rolagem horizontal...
	if (hexv->hadjustment != NULL)
	{
		int new_xscroll = ((hexv->cursor_position - hexv->fileoffset) % hexv->byte_row_length) * hexv->character_width * 3;

		if (hexv->show_offset_panel)
			new_xscroll += hexv->offset_panel_width + OFFSET_PANEL_SPACEMENT;
		gdouble current_xscroll = gtk_adjustment_get_value(hexv->hadjustment);

		GtkWidget *widget = GTK_WIDGET(hexv);

		GtkAllocation allocation;
		gtk_widget_get_allocation(widget, &allocation);
		if (new_xscroll < current_xscroll || new_xscroll - current_xscroll > allocation.width)
			gtk_adjustment_set_value(hexv->hadjustment, new_xscroll);
	}

	if (cursor_inicial != hexv->cursor_position)
		g_signal_emit_by_name(hexv, "cursor-moved");

	return cursor_inicial != hexv->cursor_position;
}

static int pontoxy_para_offset(const GtkWidget * widget, int x, int y)
{
	if (widget == NULL)
		return -1;
	XChangeHexView *hexv = XCHANGE_HEX_VIEW(widget);

	int nx = x;

	if (hexv->show_offset_panel)
		nx -= hexv->offset_panel_width + OFFSET_PANEL_SPACEMENT;

	if (hexv->hadjustment)
	{
		gdouble hscroll = gtk_adjustment_get_value(hexv->hadjustment);
		nx += hscroll;
	}

	nx = nx / (hexv->character_width) / 3;

	if (nx >= hexv->byte_row_length) // Out of scope
		return -1;//nx = hexv->byte_row_length - 1;

	int ny = y - hexv->view_offset;
	ny = ny / (hexv->font_size + LINE_SPACEMENT);

	int resposta = ny * hexv->byte_row_length + nx;

	if (resposta >= xchange_get_size(hexv->xf) - hexv->fileoffset)
		resposta = -1;
	//	printf("nx: %i  ny: %i - %i\n", nx, ny, resposta);
	return resposta;
}

//FIXME: Completar
static int pontoxy_texto_para_offset(const GtkWidget * widget, int x, int y)
{
	if (widget == NULL)
		return -1;
	XChangeHexView *hexv = XCHANGE_HEX_VIEW(widget);

	gint nx = x;

	if (hexv->show_offset_panel)
		nx -= hexv->offset_panel_width + OFFSET_PANEL_SPACEMENT;
	nx -= hexv->byte_panel_width + TEXT_SPACEMENT;

	if (hexv->hadjustment)
	{
		gdouble hscroll = gtk_adjustment_get_value(hexv->hadjustment);
		nx += hscroll;
	}


	if (nx < 0) // Out of scope
		return -1;


	// Descobre a linha
	gint ny = y - hexv->view_offset;
	ny = ny / (hexv->font_size + LINE_SPACEMENT);

	// Obtém valores mínimos e máximos de offset possíveis
	if (ny > hexv->lines_shown)
		return -1;
	gint off_min = ny * hexv->byte_row_length + hexv->bytes_skipped[ny];
	gint off_max = (ny+1) * hexv->byte_row_length;
	if (ny+1 <= hexv->lines_shown)
		off_max += hexv->bytes_skipped[ny+1];
	if (off_max >= hexv->byte_buffer_size)
		off_max = hexv->byte_buffer_size-1; // TODO: Investigate what to do in these cases
	//Out of scope
	if (off_max > xchange_get_size(hexv->xf) - hexv->fileoffset)
		return -1;

	gint x_anterior, off;
	GdkPoint p, pi;
	gboolean achou = FALSE;

	p = offset_para_pontoxy_texto(hexv, hexv->fileoffset + off_min);
	pi = p;
	x_anterior = p.x;
	for (off = off_min; off < off_max; off++)
	{
		p = offset_para_pontoxy_texto(hexv, hexv->fileoffset + off+1);
		if ((x >= x_anterior && x < p.x) || p.y > pi.y)
		{
			achou = TRUE;
			break;
		}
	}

	return achou ? off : -1;
}

static gboolean xchange_hex_view_button_press(GtkWidget *widget,
		GdkEventButton *event)
{
	// Posicionar cursor e também permitir seleção
	XChangeHexView *hexv = XCHANGE_HEX_VIEW(widget);
	gtk_widget_grab_focus(widget);

	off_t cursor_inicial = hexv->cursor_position;

	int offset = pontoxy_para_offset(widget, event->x, event->y);
	if (offset < 0)
	{
		offset = pontoxy_texto_para_offset(widget, event->x, event->y);
		if (offset < 0)
			return FALSE;
	}
	if (xchange_get_size(hexv->xf)!=-1)
	{
		if ((event->state & GDK_SHIFT_MASK) && event->button == 1)
		{
			hexv->cursor_position = hexv->fileoffset + offset;
			hexv->selection_end = -1;//hexv->cursor_position;
		}
		else if (event->button == 1)
		{
			hexv->cursor_position = hexv->fileoffset + offset;
			if (hexv->selection_start != hexv->cursor_position)
			{
				hexv->selection_start = hexv->cursor_position;
				hexv->selection_end = -1;//hexv->cursor_position;
				g_signal_emit_by_name(hexv, "selection-changed");
			}
		}
	}
	if (cursor_inicial != hexv->cursor_position)
		g_signal_emit_by_name(hexv, "cursor-moved");
	gtk_widget_queue_draw(widget);
	return FALSE;
}

static void copiar_bytes(XChangeHexView *hexv)
{
	GtkClipboard * clipboard_selecao = gtk_clipboard_get(GDK_SELECTION_PRIMARY);

	size_t tamanho;
	guchar *bytes = xchange_hex_view_get_selected_bytes(XCHANGE_HEX_VIEW(hexv),
			&tamanho);
	if (bytes == NULL || tamanho == 0)
	{
		g_free(bytes);
		return;
	}
	size_t n;
	gchar *texto = g_malloc(tamanho * 3 + 1);
	tamanho--;
	for (n = 0; n < tamanho; n++)
	{
		sprintf(&texto[n * 3], "%02hhX ", bytes[n]);
	}
	sprintf(&texto[tamanho * 3], "%02hhX", bytes[tamanho]);
	g_free(bytes);
	gtk_clipboard_set_text(clipboard_selecao, texto, -1);
	g_free(texto);
}

/*
 * Tells if (x,y) is far enough from the start of selection in order to consider it selected
 */
static gboolean is_not_so_far_from_start(XChangeHexView *hexv, gint x, gint y)
{
	GdkPoint p_start;
	int offset = pontoxy_para_offset(GTK_WIDGET(hexv), x, y);
	if (offset < 0) // Está no painel de texto?
		p_start = offset_para_pontoxy_texto(hexv, hexv->selection_start);
	else // Está no painel de byte
		p_start = offset_para_pontoxy(hexv, hexv->selection_start);

	gint xdiff = x - p_start.x;
	if (xdiff < 0)
		return FALSE; // Previous byte
	return xdiff < (2*hexv->character_width)/3;
}

static gboolean xchange_hex_view_button_release(GtkWidget *widget,
		GdkEventButton *event)
{
	XChangeHexView *hexv = XCHANGE_HEX_VIEW(widget);

	int offset = pontoxy_para_offset(widget, event->x, event->y);
	if (offset < 0)
	{
		offset = pontoxy_texto_para_offset(widget, event->x, event->y);
		if (offset < 0)
			return FALSE;
	}

	if (event->button == 1 && hexv->selection_start != -1)
	{
		if (is_not_so_far_from_start(hexv, event->x, event->y))
		{
			hexv->selection_start = (off_t) -1;
			hexv->selection_end = (off_t) -1;
			g_signal_emit_by_name(hexv, "selection-changed");
		}
		else
		{
			hexv->cursor_position = hexv->fileoffset + offset;
			if (hexv->selection_end != hexv->cursor_position)
			{
				hexv->selection_end = hexv->cursor_position;
				g_signal_emit_by_name(hexv, "selection-changed");
			}
			copiar_bytes(hexv);
		}
		gtk_widget_queue_draw(widget);
	}
	else
	{
		if (hexv->selection_start != -1)
		{
			hexv->selection_start = -1;
			g_signal_emit_by_name(hexv, "selection-changed");
		}
		gtk_widget_queue_draw(widget);
	}
	return FALSE;

}

static gboolean xchange_hex_view_motion(GtkWidget *widget,
		GdkEventMotion *event)
{
	XChangeHexView *hexv = XCHANGE_HEX_VIEW(widget);

	int offset = pontoxy_para_offset(widget, event->x, event->y);
	if (offset < 0)
	{
		offset = pontoxy_texto_para_offset(widget, event->x, event->y);
		if (offset < 0)
			return FALSE;
	}
	//if (event->button == 1)
	if (xchange_get_size(hexv->xf)!=-1)
	{
		off_t novo_cursor = hexv->fileoffset + offset;

		if (novo_cursor == hexv->selection_start && is_not_so_far_from_start(hexv, event->x, event->y))
		{
			if (hexv->cursor_position < hexv->selection_start)
				avanca_cursor(hexv, hexv->selection_start - hexv->cursor_position, TRUE);
			if (hexv->selection_end != (off_t) -1)
			{
				hexv->selection_end = (off_t) -1;
				g_signal_emit_by_name(hexv, "selection-changed");
			}
		}
		else
		{
			if (novo_cursor > hexv->cursor_position)
				avanca_cursor(hexv, novo_cursor - hexv->cursor_position, TRUE);
			else
				retrocede_cursor(hexv, hexv->cursor_position - novo_cursor, TRUE);
			if (hexv->selection_end != novo_cursor)
			{
				hexv->selection_end = novo_cursor;
				g_signal_emit_by_name(hexv, "selection-changed");
			}
		}

		gtk_widget_queue_draw(widget);
	}
	return FALSE;
}

static void xchange_hex_view_typing(XChangeHexView *hexv, uint8_t value)
{
	if (!hexv->editable)
		return;
	uint8_t byte;
	// TODO: Se seleção existe, excluí-la
	if (!hexv->cursor_position_half)
	{
		if (hexv->overwrite && hexv->cursor_position < xchange_get_size(
				hexv->xf))
		{
			xchange_get_bytes(hexv->xf, hexv->cursor_position, &byte, 1);
			byte = ((value & 0xf) << 4) | (byte & 0x0f);
			xchange_replace_bytes(hexv->xf, hexv->cursor_position, &byte, 1);
		}
		else
		{
			byte = (value & 0xf) << 4;
			xchange_insert_bytes(hexv->xf, hexv->cursor_position, &byte, 1);
		}
		hexv->cursor_position_half = TRUE;
	}
	else
	{
		xchange_get_bytes(hexv->xf, hexv->cursor_position, &byte, 1);
		byte = (value & 0xf) | (byte & 0xf0);
		xchange_replace_bytes(hexv->xf, hexv->cursor_position, &byte, 1);

		avanca_cursor(hexv, 1, FALSE);
	}
	if (hexv->selection_start != -1)
	{
		hexv->selection_start = -1;
		g_signal_emit_by_name(hexv, "selection-changed");
	}
	xchange_get_bytes(hexv->xf, hexv->fileoffset, hexv->bytes, hexv->byte_buffer_size);
	update_scroll_bounds(hexv);
	g_signal_emit_by_name(GTK_WIDGET(hexv), "changed");
}

struct MiniEditionWindowData
{
	XChangeHexView *hexv;
	GtkLabel *label_bytes;
	GtkLabel *label_bytes_length;
	GtkTextView *view;
	GtkWindow *window;
};

static gboolean miniedition_key_press (GtkWidget *widget, GdkEventKey *event, gpointer data)
{
	struct MiniEditionWindowData *mew = data;

	if (event->keyval == GDK_KEY_Escape)
	{
		gtk_widget_destroy(GTK_WIDGET(mew->window));
		return TRUE;
	}

	if (event->keyval == GDK_KEY_Return && (event->state & ~GDK_LOCK_MASK) == 0)
	{
		// Insere o texto
		// Converte para bytes
		GtkTextIter start, end;
		GtkTextBuffer *text_buffer = GTK_TEXT_BUFFER(gtk_text_view_get_buffer(mew->view));
		gtk_text_buffer_get_bounds(text_buffer, &start, &end);
		gchar *texto_original = gtk_text_buffer_get_text(text_buffer, &start, &end, FALSE);
		if (texto_original == NULL)
			return TRUE;
		gint tamanho_texto = strlen(texto_original);
		if (tamanho_texto == 0)
		{
			g_free(texto_original);
			return TRUE;
		}
		gint chave_mais_comprida = xchange_table_get_largest_entry_length(mew->hexv->xt,TRUE);
		if (chave_mais_comprida < 0)
			chave_mais_comprida = 4;
		gint tamanho_bytes = tamanho_texto* chave_mais_comprida;
		guchar * bytes = g_malloc(tamanho_bytes);
		if (bytes == NULL)
		{
			g_warning("Couldn't allocate data for bytes");
			g_free(texto_original);
			return TRUE;
		}
		int resp = xchange_table_scan_stringUTF8(mew->hexv->xt, texto_original, tamanho_texto, bytes);
		g_free(texto_original);

		// Confere tamanho
		if (resp >= 0)
		{
			if (mew->hexv->editable)
			{
				off_t posicao_cursor = mew->hexv->cursor_position;
				if (mew->hexv->selection_start != -1)
				{
					// Verifica se é de tamanho compatível com a seleção...
					size_t comprimento_selecao;
					if (mew->hexv->selection_end > mew->hexv->selection_start )
					{
						comprimento_selecao = mew->hexv->selection_end - mew->hexv->selection_start;
						posicao_cursor = mew->hexv->selection_start;
					}
					else
					{
						comprimento_selecao = mew->hexv->selection_start - mew->hexv->selection_end;
						posicao_cursor = mew->hexv->selection_end;
					}
					comprimento_selecao++;
					if (comprimento_selecao > resp)
					{
						; // TODO:
					}
					else if (comprimento_selecao < resp)
					{
						if (mew->hexv->overwrite)
							resp = comprimento_selecao; // TODO:
					}
					else
						; // Nada para fazer, tudo ok.
				}
				// Hora de efetivar a inserção/substituição dos bytes
				if (mew->hexv->overwrite)
					xchange_hex_view_replace_bytes(mew->hexv, bytes, posicao_cursor, resp);
				else
				{
					xchange_hex_view_delete_selection(mew->hexv);
					xchange_hex_view_insert_bytes(mew->hexv, bytes, posicao_cursor, resp);
				}
				g_free(bytes);
				gtk_widget_destroy(GTK_WIDGET(mew->window));
				return TRUE;
			}
		}
		g_free(bytes);
	}

	return FALSE;
}

static void miniedition_focus_out(GtkWidget *widget, GdkEvent *event, gpointer data)
{
	gtk_widget_destroy(widget);
}

static void miniedition_destroy(GtkWidget *widget, gpointer data)
{
	struct MiniEditionWindowData *mew = data;
	free(mew);
}

static void miniedition_convert_text_bytes_(GtkTextBuffer *text_buffer, gpointer data)
{
	struct MiniEditionWindowData *mew = data;
	GtkLabel *label_bytes = GTK_LABEL(mew->label_bytes);
	if (!GTK_TEXT_BUFFER(text_buffer))
		return;
	GtkTextIter start, end;
	gtk_text_buffer_get_bounds(text_buffer, &start, &end);
	gchar *texto_original = gtk_text_buffer_get_text(text_buffer, &start, &end, FALSE);
	int qtd_bytes_texto = strlen(texto_original);
	guchar * bytes = NULL;
	int tamanho_bytes = xchange_table_scan_stringUTF8(mew->hexv->xt, texto_original, qtd_bytes_texto, NULL);
	if (tamanho_bytes > 0)
	{
		bytes = g_malloc(tamanho_bytes);
		if (bytes == NULL)
			return;
		int tamanho_bytes2 = xchange_table_scan_stringUTF8(mew->hexv->xt, texto_original, qtd_bytes_texto, bytes);
		g_assert(tamanho_bytes == tamanho_bytes2);
	}
	g_free(texto_original);
	if (tamanho_bytes > 0)
	{
		gchar *texto_bytes = g_malloc0(tamanho_bytes*3+1);
		int c;
		for (c = 0; c < tamanho_bytes; c++)
		{
			if ((c+1) % 16 != 0)
				sprintf(texto_bytes+c*3, "%02hhX ", bytes[c]);
			else
				sprintf(texto_bytes+c*3, "%02hhX\n", bytes[c]);
		}

		//gtk_label_set_text(label, texto_bytes);
		char *saida = g_markup_printf_escaped("<small>%s</small>", texto_bytes);
		g_free(texto_bytes);
		gtk_label_set_markup (label_bytes, saida);
		g_free(saida);
	}
	else if (tamanho_bytes == 0)
	{
		gtk_label_set_text(label_bytes, "");
	}
	else
		gtk_label_set_markup(label_bytes, "<b>Erro na conversão</b>: caracteres desconhecidos?\nConserte ou pressione ESC para cancelar");

	// Escreve quantidade de bytes...
	// Calcula comprimento da seleção, se houver
	size_t comprimento_selecao = -1;
	if (mew->hexv->selection_start != -1)
	{
		// Verifica se é de tamanho compatível com a seleção...

		if (mew->hexv->selection_end > mew->hexv->selection_start )
			comprimento_selecao = mew->hexv->selection_end - mew->hexv->selection_start;
		else
			comprimento_selecao = mew->hexv->selection_start - mew->hexv->selection_end;
		comprimento_selecao++;
	}
	char *formatacao_extra_qtd_abre = "";
	char *formatacao_extra_qtd_fecha = "";
	if (tamanho_bytes > 0)
	{
		if (tamanho_bytes > comprimento_selecao)
		{
			formatacao_extra_qtd_abre = "<b>";
			formatacao_extra_qtd_fecha = "</b>";
		}
		else if (tamanho_bytes < comprimento_selecao)
		{
			formatacao_extra_qtd_abre = "<i>";
			formatacao_extra_qtd_fecha = "</i>";
		}
	}

	gchar *formatacao_qtd = g_strdup_printf("<small><small>%s%d bytes%s</small></small>", formatacao_extra_qtd_abre, tamanho_bytes >=0? tamanho_bytes : 0, formatacao_extra_qtd_fecha);

	GtkLabel *label_bytes_length = mew->label_bytes_length;
	gtk_label_set_markup (label_bytes_length, formatacao_qtd);
	g_free(formatacao_qtd);

	g_free(bytes);
}

static gboolean miniedition_create(XChangeHexView* hexv)
{
	GtkWidget *widget;
	GtkWidget *miniwindow;
	GdkPoint p;
	gint hexv_x = 0, hexv_y = 0, par_x = 0, par_y = 0;

	widget = GTK_WIDGET(hexv);

	// TODO: Obter cursor de texto, mas funcionando... para fazer da mesma largura da seleção?
	off_t start_cursor_position;
	if (!xchange_hex_view_get_selection_bounds(hexv, &start_cursor_position, NULL))
		start_cursor_position = hexv->cursor_position;
	p = offset_para_pontoxy_texto(hexv, start_cursor_position);
	if (p.y > hexv->virtual_h)
		p.y = hexv->virtual_h;
	if (p.y < 0)
		p.y = 0;
	if (p.x < 0)
		p.x = 0;
	GtkAllocation allocation;
	gtk_widget_get_allocation(widget, &allocation);
	if (p.x > allocation.width)
		p.x = allocation.width;
	gdk_window_get_position(gtk_widget_get_window(widget), &hexv_x, &hexv_y);
	gdk_window_get_position(gtk_widget_get_parent_window(widget), &par_x, &par_y);

	miniwindow = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	if (miniwindow == NULL)
		return FALSE;
	gtk_window_set_decorated(GTK_WINDOW(miniwindow), FALSE);
	gtk_window_set_title(GTK_WINDOW(miniwindow), "heXchange typer");
	gtk_window_set_skip_taskbar_hint(GTK_WINDOW(miniwindow), TRUE);
	gtk_window_set_skip_pager_hint(GTK_WINDOW(miniwindow), TRUE);
	gtk_window_move(GTK_WINDOW(miniwindow), par_x+hexv_x + p.x, par_y+hexv_y + p.y + hexv->font_size + 1);

	GtkWidget *frame_externo = gtk_frame_new(NULL);
	GtkWidget *vbox = gtk_vbox_new(FALSE, 1);
	gtk_container_set_border_width (GTK_CONTAINER(vbox), 3);

	GtkWidget * widget_text = gtk_text_view_new();
	GtkTextBuffer *text_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(widget_text));
	GtkWidget *frame_text = gtk_frame_new(NULL);

	size_t selected_size;
	gchar *selected_text = xchange_hex_view_get_selected_text(hexv,&selected_size);
	if (selected_text)
	{
		const gchar *end = NULL;
		gint valid_size = selected_size;
		if (!g_utf8_validate(selected_text, selected_size, &end))
		{
			if (end != NULL)
				valid_size = end - selected_text;
			selected_text[valid_size] = 0;
		}
		if (valid_size)
			gtk_text_buffer_set_text(text_buffer, selected_text, valid_size);
		g_free(selected_text);
	}

	GtkWidget * widget_label = gtk_label_new("");
	gtk_misc_set_alignment(GTK_MISC(widget_label), 0,0.5);

	GtkWidget * widget_label_length = gtk_label_new("");
	gtk_misc_set_alignment(GTK_MISC(widget_label_length), 0.5,0.5);

	struct MiniEditionWindowData *mew = malloc(sizeof(struct MiniEditionWindowData));
	mew->hexv = hexv;
	mew->label_bytes = GTK_LABEL(widget_label);
	mew->label_bytes_length = GTK_LABEL(widget_label_length);
	mew->view = GTK_TEXT_VIEW(widget_text);
	mew->window = GTK_WINDOW(miniwindow);

	gtk_widget_set_events(miniwindow, gtk_widget_get_events(miniwindow) | GDK_KEY_PRESS_MASK);
	g_signal_connect (G_OBJECT (miniwindow), "key-press-event", G_CALLBACK (miniedition_key_press), mew);
	g_signal_connect (G_OBJECT (miniwindow), "destroy", G_CALLBACK (miniedition_destroy), mew);
	g_signal_connect (G_OBJECT (miniwindow), "focus-out-event", G_CALLBACK (miniedition_focus_out), mew);
	g_signal_connect (G_OBJECT (text_buffer), "changed", G_CALLBACK (miniedition_convert_text_bytes_), mew);

	gtk_container_add (GTK_CONTAINER(frame_text), widget_text);
	gtk_container_add (GTK_CONTAINER(vbox), frame_text);
	gtk_container_add (GTK_CONTAINER(vbox), widget_label);
	gtk_container_add (GTK_CONTAINER(vbox), widget_label_length);
	gtk_container_add (GTK_CONTAINER(frame_externo), vbox);
	gtk_container_add (GTK_CONTAINER(miniwindow), frame_externo);
	gtk_widget_show_all(miniwindow);
	gtk_window_set_transient_for(GTK_WINDOW(miniwindow), NULL);

	g_signal_emit_by_name(text_buffer, "changed", text_buffer, mew, G_TYPE_NONE);

	return TRUE;
}

static void change_start_selection(XChangeHexView *hexv, off_t start)
{
	if (hexv->selection_start != start)
	{
		hexv->selection_start = start;
		g_signal_emit_by_name(hexv, "selection-changed");
	}
}

static void change_end_selection(XChangeHexView *hexv, off_t end)
{
	if (hexv->selection_end != end)
	{
		hexv->selection_end = end;
		g_signal_emit_by_name(hexv, "selection-changed");
	}
}

static void change_selection(XChangeHexView *hexv, off_t start, off_t end)
{
	if (hexv->selection_start != start || hexv->selection_end != end)
	{
		hexv->selection_start = start;
		hexv->selection_end = end;
		g_signal_emit_by_name(hexv, "selection-changed");
	}
}

static void reset_selection(XChangeHexView *hexv)
{
	if (hexv->selection_start != -1)
	{
		hexv->selection_start = -1;
		g_signal_emit_by_name(hexv, "selection-changed");
	}
}

static gboolean xchange_hex_view_key_press(GtkWidget *widget,
		GdkEventKey *event)
{
	XChangeHexView *hexv = XCHANGE_HEX_VIEW(widget);
	gboolean block_propagation = FALSE;

	if (event->keyval == GDK_KEY_Left)
	{
		if (hexv->cursor_visible)
		{
			if (event->state & GDK_SHIFT_MASK)
				if (hexv->selection_start == -1 && hexv->cursor_position > 0)
					change_start_selection(hexv, hexv->cursor_position);

			retrocede_cursor(hexv, 1, event->state & (GDK_SHIFT_MASK|GDK_CONTROL_MASK) ? TRUE
					: FALSE);

			if (event->state & GDK_SHIFT_MASK)
				change_end_selection(hexv, hexv->cursor_position);
			else
				reset_selection(hexv);

			block_propagation = TRUE;
		}
	}
	else if (event->keyval == GDK_KEY_Right)
	{
		if (hexv->cursor_visible)
		{
			if (event->state & GDK_SHIFT_MASK)
				if (hexv->selection_start == -1 && xchange_get_size(hexv->xf))
					change_start_selection(hexv, hexv->cursor_position);

			avanca_cursor(hexv, 1, event->state & (GDK_SHIFT_MASK|GDK_CONTROL_MASK) ? TRUE : FALSE);

			if (event->state & GDK_SHIFT_MASK)
				change_end_selection(hexv, hexv->cursor_position);
			else
				reset_selection(hexv);

			block_propagation = TRUE;
		}
	}
	else if (event->keyval == GDK_KEY_Down)
	{
		if (hexv->cursor_visible)
		{
			//TODO: Melhorar!
			if (event->state & GDK_SHIFT_MASK)
				if (hexv->selection_start == -1 && xchange_get_size(hexv->xf))
					change_start_selection(hexv, hexv->cursor_position);
			if (hexv->cursor_position + hexv->byte_row_length < xchange_get_size(hexv->xf))
			{
				avanca_cursor(hexv, hexv->byte_row_length, TRUE);

				if (event->state & GDK_SHIFT_MASK)
					change_end_selection(hexv, hexv->cursor_position);
				else
					reset_selection(hexv);
			}
			block_propagation = TRUE;
		}
	}
	else if (event->keyval == GDK_KEY_Up)
	{
		if (hexv->cursor_visible)
		{
			//TODO: Melhorar!
			if (event->state & GDK_SHIFT_MASK)
				if (hexv->selection_start == -1 && hexv->cursor_position > 0)
					change_start_selection(hexv, hexv->cursor_position);
			if (hexv->cursor_position - hexv->byte_row_length >= 0)
			{
				retrocede_cursor(hexv, hexv->byte_row_length, TRUE);
				if (event->state & GDK_SHIFT_MASK)
					change_end_selection(hexv, hexv->cursor_position);
				else
					reset_selection(hexv);
			}
			block_propagation = TRUE;
		}
	}
	else if (event->keyval == GDK_KEY_Home)
	{
		if (hexv->cursor_visible)
		{
			//TODO: Melhorar!
			if (event->state & GDK_SHIFT_MASK)
				if (hexv->selection_start == -1 && hexv->cursor_position > 0)
					change_start_selection(hexv, hexv->cursor_position);

			if (event->state & GDK_CONTROL_MASK)
				retrocede_cursor(hexv, hexv->cursor_position, TRUE);//hexv->cursor_position = 0;
			else
				retrocede_cursor(hexv, hexv->cursor_position
						% hexv->byte_row_length, TRUE);//hexv->cursor_position = (hexv->cursor_position/hexv->byte_row_length)*hexv->byte_row_length;

			hexv->cursor_position_half = FALSE;
			if (event->state & GDK_SHIFT_MASK)
				change_end_selection(hexv, hexv->cursor_position);
			else
				reset_selection(hexv);
			block_propagation = TRUE;
		}
	}
	else if (event->keyval == GDK_KEY_End)
	{
		if (hexv->cursor_visible)
		{
			//TODO: Melhorar!
			if (event->state & GDK_SHIFT_MASK)
				if (hexv->selection_start == -1 && xchange_get_size(hexv->xf))
					change_start_selection(hexv, hexv->cursor_position);

			if (event->state & GDK_CONTROL_MASK)
				avanca_cursor(hexv, xchange_get_size(hexv->xf), TRUE);
			else
			{
				avanca_cursor(hexv, hexv->byte_row_length - 1
						- (hexv->cursor_position % hexv->byte_row_length), TRUE);
			}

			hexv->cursor_position_half = FALSE;
			if (event->state & GDK_SHIFT_MASK)
				change_end_selection(hexv, hexv->cursor_position);
			else
				reset_selection(hexv);

			block_propagation = TRUE;
		}
	}
	else if (event->keyval == GDK_KEY_Page_Up)
	{
		if (hexv->cursor_visible)
		{
			if (event->state & GDK_SHIFT_MASK)
				if (hexv->selection_start == -1 && hexv->cursor_position > 0)
					change_start_selection(hexv, hexv->cursor_position);

			gint lines_to_back = hexv->lines_shown; // FIXME: lines_shown não representa a realidade...
			gint lines_can_back = hexv->cursor_position / hexv->byte_row_length;
			if ( lines_can_back < lines_to_back)
				lines_to_back = lines_can_back;

			retrocede_cursor(hexv, lines_to_back * hexv->byte_row_length, TRUE);

			hexv->cursor_position_half = FALSE;
			if (event->state & GDK_SHIFT_MASK)
				change_end_selection(hexv, hexv->cursor_position);
			else
				reset_selection(hexv);

			block_propagation = TRUE;
		}
	}
	else if (event->keyval == GDK_KEY_Page_Down)
	{
		if (hexv->cursor_visible)
		{
			if (event->state & GDK_SHIFT_MASK)
				if (hexv->selection_start == -1 && xchange_get_size(hexv->xf))
					change_start_selection(hexv, hexv->cursor_position);

			gint lines_to_go = hexv->lines_shown; // FIXME: lines_shown não representa a realidade...
			gint lines_can_go = (xchange_get_size(hexv->xf) - hexv->cursor_position) / hexv->byte_row_length;
			if ( lines_to_go > lines_can_go)
				lines_to_go = lines_can_go;

			avanca_cursor(hexv, lines_to_go * hexv->byte_row_length, TRUE);

			hexv->cursor_position_half = FALSE;
			if (event->state & GDK_SHIFT_MASK)
				change_end_selection(hexv, hexv->cursor_position);
			else
				reset_selection(hexv);

			block_propagation = TRUE;
		}
	}
	else if (event->keyval == GDK_KEY_Insert && event->state == 0)
	{
		if (hexv->editable)
		{
			xchange_hex_view_set_edition_mode(hexv, hexv->overwrite);
		}
	}
	else if (event->keyval == GDK_KEY_Delete)
	{
		if (hexv->editable && !hexv->overwrite && !hexv->cursor_position_half)
		{
			if (xchange_hex_view_get_has_selection(hexv))
				xchange_hex_view_delete_selection(hexv);
			else
				xchange_hex_view_delete(hexv, hexv->cursor_position, 1);
		}
	}
	else if (event->keyval == GDK_KEY_BackSpace)
	{
		if (hexv->editable && !hexv->overwrite && !hexv->cursor_position_half)
		{
			if (xchange_hex_view_get_has_selection(hexv))
				xchange_hex_view_delete_selection(hexv);
			else
			{
				if (hexv->cursor_position > 0)
				{
					xchange_hex_view_delete(hexv, hexv->cursor_position-1, 1);
					retrocede_cursor(hexv, 1, TRUE);
				}
			}
		}
	}
	else if (event->keyval == GDK_KEY_z && event->state == GDK_CONTROL_MASK)
	{
		if (hexv->editable)
			xchange_hex_view_undo(hexv);
	}
	else if (event->keyval == GDK_KEY_y && event->state == GDK_CONTROL_MASK)
	{
		if (hexv->editable)
			xchange_hex_view_redo(hexv);
	}
	else if (event->keyval == GDK_KEY_Return)
	{
		if (hexv->editable)
		{
			// TODO: Verificar se há fuga de memória
			miniedition_create(hexv);
			block_propagation = TRUE;
		}
	}
	else
	{
		switch (event->keyval)
		{
		case GDK_KEY_A:
		case GDK_KEY_a:
		{
			xchange_hex_view_typing(hexv, 0xa);
			break;
		}
		case GDK_KEY_B:
		case GDK_KEY_b:
		{
			xchange_hex_view_typing(hexv, 0xb);
			break;
		}
		case GDK_KEY_C:
		case GDK_KEY_c:
		{
			xchange_hex_view_typing(hexv, 0xc);
			break;
		}
		case GDK_KEY_D:
		case GDK_KEY_d:
		{
			xchange_hex_view_typing(hexv, 0xd);
			break;
		}
		case GDK_KEY_E:
		case GDK_KEY_e:
		{
			xchange_hex_view_typing(hexv, 0xe);
			break;
		}
		case GDK_KEY_F:
		case GDK_KEY_f:
		{
			xchange_hex_view_typing(hexv, 0xf);
			break;
		}
		case GDK_KEY_0:
		{
			xchange_hex_view_typing(hexv, 0);
			break;
		}
		case GDK_KEY_1:
		{
			xchange_hex_view_typing(hexv, 1);
			break;
		}
		case GDK_KEY_2:
		{
			xchange_hex_view_typing(hexv, 2);
			break;
		}
		case GDK_KEY_3:
		{
			xchange_hex_view_typing(hexv, 3);
			break;
		}
		case GDK_KEY_4:
		{
			xchange_hex_view_typing(hexv, 4);
			break;
		}
		case GDK_KEY_5:
		{
			xchange_hex_view_typing(hexv, 5);
			break;
		}
		case GDK_KEY_6:
		{
			xchange_hex_view_typing(hexv, 6);
			break;
		}
		case GDK_KEY_7:
		{
			xchange_hex_view_typing(hexv, 7);
			break;
		}
		case GDK_KEY_8:
		{
			xchange_hex_view_typing(hexv, 8);
			break;
		}
		case GDK_KEY_9:
		{
			xchange_hex_view_typing(hexv, 9);
			break;
		}
		}
	}
	gtk_widget_queue_draw(widget); // FIXME: _area
	return block_propagation;
}

void xchange_hex_view_set_editable(XChangeHexView *xchange_hex_view,
		gboolean editable)
{
	if (!XCHANGE_IS_HEX_VIEW(xchange_hex_view))
		return;
	xchange_hex_view->editable = editable;
	gtk_widget_queue_draw(GTK_WIDGET(xchange_hex_view)); // FIXME _area
}

gboolean xchange_hex_view_is_editable(const XChangeHexView *xchange_hex_view)
{
	if (!XCHANGE_IS_HEX_VIEW(xchange_hex_view))
		return FALSE;
	return xchange_hex_view->editable;
}

void xchange_hex_view_set_edition_mode(XChangeHexView *xchange_hex_view, gboolean insertion)
{
	if (!XCHANGE_IS_HEX_VIEW(xchange_hex_view))
		return;
	gboolean new_overwrite = !insertion;

	if (new_overwrite != xchange_hex_view->overwrite)
	{
		if (insertion)
		{
			gint blink_time;
			gboolean blink;

			GtkSettings *s = gtk_settings_get_default();
			g_object_get(s, "gtk-cursor-blink", &blink, "gtk-cursor-blink-time", &blink_time, NULL);
			xchange_hex_view->cursor_in_blink = FALSE;
			if (blink)
				g_timeout_add(blink_time, pisca_cursor, xchange_hex_view);
			if (xchange_hex_view->cursor_position_half)
				avanca_cursor(xchange_hex_view, 1, TRUE);
		}
		xchange_hex_view->overwrite = new_overwrite;
		g_signal_emit_by_name(xchange_hex_view, "edition-mode-changed");
		gtk_widget_queue_draw(GTK_WIDGET(xchange_hex_view)); // FIXME _area
	}
}

gboolean xchange_hex_view_is_insertion_mode(const XChangeHexView *xchange_hex_view)
{
	if (!XCHANGE_IS_HEX_VIEW(xchange_hex_view))
		return FALSE;
	return !xchange_hex_view->overwrite;
}

/*
void xchange_hex_view_set_overwrite(XChangeHexView *xchange_hex_view,
		gboolean overwrite)
{
	if (!XCHANGE_IS_HEX_VIEW(xchange_hex_view))
		return;
	xchange_hex_view->overwrite = overwrite;
	gtk_widget_queue_draw(GTK_WIDGET(xchange_hex_view)); // FIXME _area
}
gboolean xchange_hex_view_is_overwrite(XChangeHexView *xchange_hex_view)
{
	if (!XCHANGE_IS_HEX_VIEW(xchange_hex_view))
		return FALSE;
	return xchange_hex_view->overwrite;
}
*/
void xchange_hex_view_show_offset_panel(XChangeHexView *xchange_hex_view)
{
	if (!XCHANGE_IS_HEX_VIEW(xchange_hex_view))
		return;
	if (xchange_hex_view->show_offset_panel == FALSE)
	{
		xchange_hex_view->show_offset_panel = TRUE;
		gtk_widget_queue_draw(GTK_WIDGET(xchange_hex_view)); // FIXME _area
		update_scroll_bounds(xchange_hex_view);
	}
}
void xchange_hex_view_hide_offset_panel(XChangeHexView *xchange_hex_view)
{
	if (!XCHANGE_IS_HEX_VIEW(xchange_hex_view))
		return;
	if (xchange_hex_view->show_offset_panel == TRUE)
	{
		xchange_hex_view->show_offset_panel = FALSE;
		gtk_widget_queue_draw(GTK_WIDGET(xchange_hex_view)); // FIXME _area
		update_scroll_bounds(xchange_hex_view);
	}
}
void xchange_hex_view_set_offset_panel_visibility(
		XChangeHexView *xchange_hex_view, gboolean visible)
{
	if (!XCHANGE_IS_HEX_VIEW(xchange_hex_view))
		return;
	if (xchange_hex_view->show_offset_panel != visible)
	{
		xchange_hex_view->show_offset_panel = visible;
		gtk_widget_queue_draw(GTK_WIDGET(xchange_hex_view)); // FIXME _area
		update_scroll_bounds(xchange_hex_view);
	}
}
gboolean xchange_hex_view_is_offset_panel_visible(
		const XChangeHexView *xchange_hex_view)
{
	if (!XCHANGE_IS_HEX_VIEW(xchange_hex_view))
		return FALSE;
	return xchange_hex_view->show_offset_panel;
}

void xchange_hex_view_show_text_panel(XChangeHexView *xchange_hex_view)
{
	if (!XCHANGE_IS_HEX_VIEW(xchange_hex_view))
		return;
	if (xchange_hex_view->show_text_panel == FALSE)
	{
		xchange_hex_view->show_text_panel = TRUE;
		gtk_widget_queue_draw(GTK_WIDGET(xchange_hex_view)); // FIXME _area
		update_scroll_bounds(xchange_hex_view);
	}
}
void xchange_hex_view_hide_text_panel(XChangeHexView *xchange_hex_view)
{
	if (!XCHANGE_IS_HEX_VIEW(xchange_hex_view))
		return;
	if (xchange_hex_view->show_text_panel == TRUE)
	{
		xchange_hex_view->show_text_panel = FALSE;
		gtk_widget_queue_draw(GTK_WIDGET(xchange_hex_view)); // FIXME _area
		update_scroll_bounds(xchange_hex_view);
	}
}
void xchange_hex_view_set_text_panel_visibility(
		XChangeHexView *xchange_hex_view, gboolean visible)
{
	if (!XCHANGE_IS_HEX_VIEW(xchange_hex_view))
		return;
	if (xchange_hex_view->show_text_panel != visible)
	{
		xchange_hex_view->show_text_panel = visible;
		gtk_widget_queue_draw(GTK_WIDGET(xchange_hex_view)); // FIXME _area
		update_scroll_bounds(xchange_hex_view);
	}
}
gboolean xchange_hex_view_is_text_panel_visible(
		const XChangeHexView *xchange_hex_view)
{
	if (!XCHANGE_IS_HEX_VIEW(xchange_hex_view))
		return FALSE;
	return xchange_hex_view->show_text_panel;
}

void xchange_hex_view_set_byte_row_length(XChangeHexView *xchange_hex_view,
		gushort byte_row_length)
{
	if (!XCHANGE_IS_HEX_VIEW(xchange_hex_view))
		return;
	if (byte_row_length < 0)
		return;
	if (byte_row_length == 0)
		byte_row_length = PADRAO_BYTES_POR_LINHA;
	if (xchange_hex_view->byte_row_length != byte_row_length)
	{
		xchange_hex_view->byte_row_length = byte_row_length;
		// Computes byte panel width
		xchange_hex_view->byte_panel_width = (xchange_hex_view->byte_row_length) * (xchange_hex_view->character_width * 3);

		// Computes number of lines shown
		compute_lines_shown(xchange_hex_view);
		calcula_tamanho_buffer(xchange_hex_view);

		gtk_widget_queue_draw(GTK_WIDGET(xchange_hex_view)); // FIXME _area
		update_scroll_bounds(xchange_hex_view);
	}
}

const XChangeFile *xchange_hex_view_get_file(XChangeHexView *xchange_hex_view)
{
	if (!XCHANGE_IS_HEX_VIEW(xchange_hex_view))
		return NULL;
	return xchange_hex_view->xf;
}

const XChangeTable *xchange_hex_view_get_table(XChangeHexView *xchange_hex_view)
{
	if (!XCHANGE_IS_HEX_VIEW(xchange_hex_view))
		return NULL;
	return xchange_hex_view->xt;
}

void xchange_hex_view_goto(XChangeHexView *xchange_hex_view, off_t offset)
{
	if (!XCHANGE_IS_HEX_VIEW(xchange_hex_view))
		return;

	off_t cursor_inicial = xchange_hex_view->cursor_position;
	size_t filesize = xchange_get_size(xchange_hex_view->xf);
	if (filesize == (size_t) -1)
		return;

	if (offset > filesize)
		offset = filesize;

	off_t destination = offset;
	// TODO: compute destination to point to first line to appear

	 // Tentar manter posição relativa do cursor na tela: base-base, topo-topo, 2linha-2a.linha
	/*
	int tmp = ((offset-xchange_hex_view->fileoffset)/xchange_hex_view->byte_row_length);
	g_print("tmp: %i (%i)\n", tmp, xchange_hex_view->lines_shown);
	if (tmp < 0)
	{
		xchange_hex_view->view_offset = 0;
		destination = xchange_hex_view->fileoffset + (tmp)*xchange_hex_view->byte_row_length;
	}
	else if (tmp >= xchange_hex_view->lines_shown-1)
		destination = xchange_hex_view->fileoffset + (tmp - xchange_hex_view->lines_shown + 1)*xchange_hex_view->byte_row_length;
*/
	if (xchange_hex_view->vadjustment != NULL)
		gtk_adjustment_set_value(xchange_hex_view->vadjustment,
				(destination / xchange_hex_view->byte_row_length) * (LINE_SPACEMENT
						+ xchange_hex_view->font_size) + xchange_hex_view->view_offset);
	else
	{
		xchange_hex_view->fileoffset = (destination
				/ xchange_hex_view->byte_row_length)
				* xchange_hex_view->byte_row_length;
		xchange_get_bytes(xchange_hex_view->xf, xchange_hex_view->fileoffset,
				xchange_hex_view->bytes, xchange_hex_view->byte_buffer_size);
		gtk_widget_queue_draw(GTK_WIDGET(xchange_hex_view)); // FIXME _area
	}
	//gtk_adjustment_set_value(xchange_hex_view->hadjustment, (xchange_hex_view->cursor_position % xchange_hex_view->byte_row_length)*(xchange_hex_view->character_width * 3)+(xchange_hex_view->show_offset_panel? xchange_hex_view->offset_panel_width : 0));

	xchange_hex_view->cursor_position = offset;

	// Rolagem horizontal
	if (xchange_hex_view->hadjustment != NULL)
	{
		int new_xscroll = ((xchange_hex_view->cursor_position - xchange_hex_view->fileoffset) % xchange_hex_view->byte_row_length) * xchange_hex_view->character_width * 3;

		if (xchange_hex_view->show_offset_panel)
			new_xscroll += xchange_hex_view->offset_panel_width + OFFSET_PANEL_SPACEMENT;
		gdouble current_xscroll = gtk_adjustment_get_value(xchange_hex_view->hadjustment);

		GtkWidget *widget = GTK_WIDGET(xchange_hex_view);
		GtkAllocation allocation;
		gtk_widget_get_allocation(widget, &allocation);
		if (new_xscroll < current_xscroll || new_xscroll - current_xscroll > allocation.width)
			gtk_adjustment_set_value(xchange_hex_view->hadjustment, new_xscroll);
	}


	if (cursor_inicial != xchange_hex_view->cursor_position)
		g_signal_emit_by_name(xchange_hex_view, "cursor-moved");

}

gboolean xchange_hex_view_load_table_file(XChangeHexView *xchange_hex_view,
		gchar *filename, gchar *encoding)
{
	if (!XCHANGE_IS_HEX_VIEW(xchange_hex_view))
		return FALSE;

	if (filename == NULL)
		return FALSE;

	XChangeTable * xt = xchange_table_open(filename, THINGY_TABLE, encoding);
	if (xt == NULL)
		return FALSE;

	xchange_hex_view_set_table(xchange_hex_view, xt);
	xchange_hex_view->table_loaded = TRUE;
	return TRUE;
}

void xchange_hex_view_set_table(XChangeHexView *xchange_hex_view, const XChangeTable *xt)
{
	if (!XCHANGE_IS_HEX_VIEW(xchange_hex_view) || xt == NULL)
		return;
	if (xchange_hex_view->table_loaded)
		xchange_table_close(xchange_hex_view->xt);
	xchange_hex_view->xt = xt;
	xchange_hex_view->table_loaded = FALSE;

	//update_scroll_bounds(xchange_hex_view);
	gtk_widget_queue_draw(GTK_WIDGET(xchange_hex_view));
}

/*
 * Change the look how a NULL character appears at screen. It can be replaced by a string.
 * @character: the string with new representation. Set NULL for default (_ )
 */
void xchange_hex_view_set_null_character_replacement(XChangeHexView *xchange_hex_view, const gchar *character)
{
	if (!XCHANGE_IS_HEX_VIEW(xchange_hex_view))
		return;
	if (g_strcmp0(xchange_hex_view->null_char_replacer, character) == 0)
		return;

	g_free(xchange_hex_view->null_char_replacer);

	if (character == NULL)
	{
		xchange_hex_view->null_char_replacer = g_strdup(VISUAL_CARACTERE_NULO);
	}
	else
		xchange_hex_view->null_char_replacer = g_strdup(character);

	gtk_widget_queue_draw(GTK_WIDGET(xchange_hex_view));
}


gchar *xchange_hex_view_get_selected_text(XChangeHexView *xchange_hex_view, size_t *size)
{
	// TODO: Refazer...
	size_t bytes_size;
	guchar *bytes = xchange_hex_view_get_selected_bytes(xchange_hex_view, &bytes_size);
	if (bytes == NULL)
	{
		if (size != NULL)
			*size = 0;
		return NULL;
	}
	// TODO: Garantir que converterá para o mesmo texto escrito na tela...
	XChangeTable *xt = xchange_hex_view->xt;
	int tamanho_texto = xchange_table_print_stringUTF8(xt, bytes, bytes_size, NULL);
	if (tamanho_texto <= 0)
	{
		g_free(bytes);
		if (size != NULL)
			*size = 0;
		return NULL;
	}
	gchar *texto = g_malloc0(tamanho_texto+1);
	if (texto == NULL)
	{
		g_free(bytes);
		if (size != NULL)
			*size = 0;
		return NULL;
	}
	xchange_table_print_stringUTF8(xt, bytes, bytes_size, texto);
	texto[tamanho_texto] = 0;
	if (size != NULL)
		*size = tamanho_texto+1;
	g_free(bytes);
	return texto;
}

guchar *xchange_hex_view_get_selected_bytes(XChangeHexView *xchange_hex_view,
		size_t *size)
{
	if (!XCHANGE_IS_HEX_VIEW(xchange_hex_view))
	{
		if (size != NULL)
			*size = 0;
		return NULL;
	}
	off_t start, end;
	if(!xchange_hex_view_get_selection_bounds(xchange_hex_view, &start, &end))
	{
		if (size != NULL)
			*size = 0;
		return NULL;
	}
	size_t _size = end - start +1;
	guchar *bytes = xchange_hex_view_get_bytes(xchange_hex_view, start, _size);

	if (size != NULL)
		*size = _size;
	return bytes;
}



void xchange_hex_view_select(XChangeHexView *xchange_hex_view, off_t from, size_t size)
{
	if (!XCHANGE_IS_HEX_VIEW(xchange_hex_view))
			return;
	if (from == (off_t) -1 || size == 0 || size == (size_t)-1)
	{
		change_start_selection(xchange_hex_view, -1);
		return;
	}

	change_selection(xchange_hex_view, from, from + size -1);
}

off_t xchange_hex_view_get_cursor(const XChangeHexView *xchange_hex_view)
{
	if (!XCHANGE_IS_HEX_VIEW(xchange_hex_view))
		return (off_t) -1;
	return xchange_hex_view->cursor_position;
}

gboolean xchange_hex_view_get_has_selection(const XChangeHexView *xchange_hex_view)
{
	if (!XCHANGE_IS_HEX_VIEW(xchange_hex_view))
		return FALSE;
	return (xchange_hex_view->selection_start != -1 && xchange_hex_view->selection_end != -1);
}

gboolean xchange_hex_view_get_selection_bounds(XChangeHexView *xchange_hex_view, off_t *start, off_t *end)
{
	if (!XCHANGE_IS_HEX_VIEW(xchange_hex_view))
		return FALSE;

	if (xchange_hex_view->selection_start == -1 || xchange_hex_view->selection_end == -1)
		return FALSE;

	off_t inicio, fim;
	if (xchange_hex_view->selection_start > xchange_hex_view->selection_end)
	{
		inicio = xchange_hex_view->selection_end;
		fim = xchange_hex_view->selection_start;
	}
	else
	{
		inicio = xchange_hex_view->selection_start;
		fim = xchange_hex_view->selection_end;
	}

	if (start != NULL)
		*start = inicio;
	if (end != NULL)
		*end = fim;
	return TRUE;
}

static void xchange_hex_view_delete(XChangeHexView *xchange_hex_view, off_t origem, size_t qtd)
{
	if (xchange_hex_view->editable)
	{
		int excluiu = 0;

		excluiu = xchange_delete_bytes(xchange_hex_view->xf, origem, qtd);

		if (excluiu)
		{
			xchange_get_bytes(xchange_hex_view->xf, xchange_hex_view->fileoffset, xchange_hex_view->bytes,
					xchange_hex_view->byte_buffer_size);

			size_t new_filesize = xchange_get_size(xchange_hex_view->xf);
			if (new_filesize != -1 && xchange_hex_view->cursor_position >= new_filesize)
			{
				xchange_hex_view->cursor_position_half = FALSE;
				retrocede_cursor(xchange_hex_view, xchange_hex_view->cursor_position - new_filesize, TRUE);
			}

			if (xchange_hex_view->selection_start != -1 && xchange_hex_view->selection_start >= new_filesize)
			{
				reset_selection(xchange_hex_view);
			}
			if (xchange_hex_view->selection_end != -1 && xchange_hex_view->selection_end >= new_filesize)
			{
				change_end_selection(xchange_hex_view, new_filesize-1);
			}

			g_signal_emit_by_name(GTK_WIDGET(xchange_hex_view), "changed");
		}
	}
}

void xchange_hex_view_delete_selection(XChangeHexView *xchange_hex_view)
{
	off_t inicio, fim;
	if (!xchange_hex_view_get_selection_bounds(xchange_hex_view, &inicio, &fim))
		return;
	if (fim == -1)
		return;
	xchange_hex_view_delete(xchange_hex_view, inicio, fim-inicio +1);
	reset_selection(xchange_hex_view);
	update_scroll_bounds(xchange_hex_view);
	if (xchange_hex_view->cursor_position > inicio)
		retrocede_cursor(xchange_hex_view, xchange_hex_view->cursor_position - inicio, TRUE);
	else if (xchange_hex_view->cursor_position < inicio)
		avanca_cursor(xchange_hex_view, inicio - xchange_hex_view->cursor_position, TRUE);
	gtk_widget_queue_draw(GTK_WIDGET(xchange_hex_view)); // FIXME _area
}


guchar *xchange_hex_view_get_bytes(XChangeHexView *xchange_hex_view, off_t offset, size_t size)
{
	if (!XCHANGE_IS_HEX_VIEW(xchange_hex_view))
	{
		return NULL;
	}

	guchar *bytes = g_malloc(size);
	if (bytes == NULL)
	{
		return NULL;
	}
	size_t bytes_read = xchange_get_bytes(xchange_hex_view->xf, offset, bytes, size);
	if (bytes_read == 0)
	{
		g_free(bytes);
		return NULL;
	}
	// TODO: Redimensionar (reduzir o tamanho) do vetor bytes caso bytes_read != size?
	return bytes;
}

gboolean xchange_hex_view_insert_bytes(XChangeHexView *xchange_hex_view, const uint8_t *bytes, off_t offset, size_t size)
{
	if (!XCHANGE_IS_HEX_VIEW(xchange_hex_view))
		return FALSE;
	if (bytes == NULL)
		return FALSE;
	if (xchange_hex_view->xf == NULL)
		return FALSE;
	if (offset > xchange_get_size(xchange_hex_view->xf))
		return FALSE;

	if (!xchange_insert_bytes(xchange_hex_view->xf, offset, bytes, size))
	{
		return FALSE;
	}

	xchange_get_bytes(xchange_hex_view->xf, xchange_hex_view->fileoffset, xchange_hex_view->bytes,
					xchange_hex_view->byte_buffer_size);

	gtk_widget_queue_draw(GTK_WIDGET(xchange_hex_view));
	g_signal_emit_by_name(GTK_WIDGET(xchange_hex_view), "changed");
	return TRUE;
}
gboolean xchange_hex_view_replace_bytes(XChangeHexView *xchange_hex_view, const uint8_t *bytes, off_t offset, size_t size)
{
	if (!XCHANGE_IS_HEX_VIEW(xchange_hex_view))
		return FALSE;
	if (bytes == NULL)
		return FALSE;
	if (xchange_hex_view->xf == NULL)
		return FALSE;
	if (offset > xchange_get_size(xchange_hex_view->xf))
		return FALSE;

	if (!xchange_replace_bytes(xchange_hex_view->xf, offset, bytes, size))
	{
		return FALSE;
	}
	xchange_get_bytes(xchange_hex_view->xf, xchange_hex_view->fileoffset, xchange_hex_view->bytes,
					xchange_hex_view->byte_buffer_size);

	gtk_widget_queue_draw(GTK_WIDGET(xchange_hex_view));
	g_signal_emit_by_name(GTK_WIDGET(xchange_hex_view), "changed");
	return TRUE;
}

gboolean xchange_hex_view_delete_bytes(XChangeHexView *xchange_hex_view, off_t offset, size_t size)
{
	if (!XCHANGE_IS_HEX_VIEW(xchange_hex_view))
		return FALSE;
	if (xchange_hex_view->xf == NULL)
		return FALSE;
	if (offset > xchange_get_size(xchange_hex_view->xf))
		return FALSE;

	if (!xchange_delete_bytes(xchange_hex_view->xf, offset, size))
	{
		return FALSE;
	}

	xchange_get_bytes(xchange_hex_view->xf, xchange_hex_view->fileoffset, xchange_hex_view->bytes,
					xchange_hex_view->byte_buffer_size);

	size_t filesize = xchange_get_size(xchange_hex_view->xf);
	if (xchange_hex_view->cursor_position >= filesize)
	{
		retrocede_cursor(xchange_hex_view, xchange_hex_view->cursor_position - filesize +1, TRUE);
	}
	if (xchange_hex_view->selection_start != -1 && xchange_hex_view->selection_start >= filesize)
	{
		reset_selection(xchange_hex_view);
	}
	if (xchange_hex_view->selection_end != -1 && xchange_hex_view->selection_end >= filesize)
	{
		change_end_selection(xchange_hex_view, filesize-1);
	}

	gtk_widget_queue_draw(GTK_WIDGET(xchange_hex_view));
	g_signal_emit_by_name(GTK_WIDGET(xchange_hex_view), "changed");
	return TRUE;
}



gboolean xchange_hex_view_undo(XChangeHexView *xchange_hex_view)
{
	if (!XCHANGE_IS_HEX_VIEW(xchange_hex_view))
		return FALSE;
	if (! xchange_hex_view->editable)
		return FALSE;

	if (!xchange_undo(xchange_hex_view->xf))
		return FALSE;

	xchange_get_bytes(xchange_hex_view->xf, xchange_hex_view->fileoffset, xchange_hex_view->bytes,
			xchange_hex_view->byte_buffer_size);

	size_t filesize = xchange_get_size(xchange_hex_view->xf);
	if (xchange_hex_view->cursor_position >= filesize)
	{
		retrocede_cursor(xchange_hex_view, xchange_hex_view->cursor_position - filesize +1, TRUE);
	}
	if (xchange_hex_view->selection_start != -1 && xchange_hex_view->selection_start >= filesize)
	{
		reset_selection(xchange_hex_view);
	}
	if (xchange_hex_view->selection_end != -1 && xchange_hex_view->selection_end >= filesize)
	{
		change_end_selection(xchange_hex_view, filesize-1);
	}

	g_signal_emit_by_name(GTK_WIDGET(xchange_hex_view), "changed");
	return TRUE;
}
gboolean xchange_hex_view_redo(XChangeHexView *xchange_hex_view)
{
	if (!XCHANGE_IS_HEX_VIEW(xchange_hex_view))
		return FALSE;
	if (! xchange_hex_view->editable)
		return FALSE;

	if (!xchange_redo(xchange_hex_view->xf))
		return FALSE;

	xchange_get_bytes(xchange_hex_view->xf, xchange_hex_view->fileoffset, xchange_hex_view->bytes,
			xchange_hex_view->byte_buffer_size);

	size_t filesize = xchange_get_size(xchange_hex_view->xf);
	if (xchange_hex_view->cursor_position >= filesize)
	{
		retrocede_cursor(xchange_hex_view, xchange_hex_view->cursor_position - filesize + 1, TRUE);
	}
	if (xchange_hex_view->selection_start != -1 && xchange_hex_view->selection_start >= filesize)
	{
		reset_selection(xchange_hex_view);
	}
	if (xchange_hex_view->selection_end != -1 && xchange_hex_view->selection_end >= filesize)
	{
		change_end_selection(xchange_hex_view, filesize-1);
	}

	g_signal_emit_by_name(GTK_WIDGET(xchange_hex_view), "changed");
	return TRUE;
}

/*****************************************************************
 *                            Rolagem
 *****************************************************************/

/*
 * Função de retorno ao rolar o widget
 */
static void xchange_hex_view_scroll_value_changed(GtkAdjustment *adjustment,
		gpointer data)
{
	XChangeHexView *hexv = XCHANGE_HEX_VIEW(data);
	if (adjustment == hexv->vadjustment)
	{
		gint pos = gtk_adjustment_get_value(adjustment);
		gint line_height = hexv->font_size + LINE_SPACEMENT;
		hexv->view_offset = -(pos % line_height);
		gint file_offset = (pos / line_height) * hexv->byte_row_length;
		if (file_offset > xchange_get_size(hexv->xf))
			file_offset = (xchange_get_size(hexv->xf) / hexv->byte_row_length)
					* hexv->byte_row_length;

		hexv->fileoffset = file_offset;
		xchange_get_bytes(hexv->xf, hexv->fileoffset, hexv->bytes,
				hexv->byte_buffer_size);

		compute_lines_shown(hexv);
	}
	gtk_widget_queue_draw(data);
}

/*
 * Função de retorno ao definir as barras de rolagem
 */
static gboolean xchange_hex_view_set_scroll_adjustments(GtkWidget *widget,
		GtkAdjustment *hadjustment, GtkAdjustment *vadjustment)
{
	if (!XCHANGE_IS_HEX_VIEW(widget))
		return FALSE;
	XChangeHexView * xchange_hex_view = XCHANGE_HEX_VIEW(widget);
	xchange_hex_view->hadjustment = hadjustment;
	xchange_hex_view->vadjustment = vadjustment;

	GtkAllocation allocation;
	gtk_widget_get_allocation(widget, &allocation);
	gint lines = (allocation.height - xchange_hex_view->view_offset)
			/ (xchange_hex_view->font_size + LINE_SPACEMENT);

	if (lines * xchange_hex_view->byte_row_length > xchange_get_size(
			xchange_hex_view->xf))
	{
		g_print("Maior\n");
	}

	if (hadjustment != NULL)
	{
		gtk_adjustment_set_value(xchange_hex_view->hadjustment, 0);
		g_signal_connect(G_OBJECT(xchange_hex_view->hadjustment),
				"value-changed", G_CALLBACK(
						xchange_hex_view_scroll_value_changed), widget);
	}
	if (vadjustment != NULL)
	{
		gtk_adjustment_set_value(xchange_hex_view->vadjustment, 0);
		g_signal_connect(G_OBJECT(xchange_hex_view->vadjustment),
				"value-changed", G_CALLBACK(
						xchange_hex_view_scroll_value_changed), widget);
	}
	update_scroll_bounds(xchange_hex_view);
	return TRUE;
}

/*
 * Atualiza os limites de rolagem
 */
static void update_scroll_bounds(XChangeHexView *xchange_hex_view)
{
	if (!XCHANGE_IS_HEX_VIEW(xchange_hex_view))
		return;
	GtkAllocation allocation;
	gtk_widget_get_allocation(GTK_WIDGET(xchange_hex_view), &allocation);
	if (xchange_hex_view->hadjustment != NULL)
	{
		gdouble valor = gtk_adjustment_get_value(xchange_hex_view->hadjustment);
		gint w_visivel = allocation.width;
		gtk_adjustment_configure(xchange_hex_view->hadjustment, valor, 0, xchange_hex_view->virtual_w,
				xchange_hex_view->font_size + LINE_SPACEMENT, w_visivel, w_visivel);
	}
	if (xchange_hex_view->vadjustment != NULL)
	{
		gdouble valor = gtk_adjustment_get_value(xchange_hex_view->vadjustment);
		gint h_visivel = allocation.height;
		gtk_adjustment_configure(xchange_hex_view->vadjustment, valor, 0, xchange_hex_view->virtual_h,
				xchange_hex_view->font_size + LINE_SPACEMENT, h_visivel, h_visivel);
	}
}
