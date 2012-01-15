/*
 * gtk-dialogos-mensagens.h
 *
 *  Created on: 16/07/2011
 *      Author: rodolfo
 */

#ifndef R_GTKMESSAGEDIALOGS_H_
#define R_GTKMESSAGEDIALOGS_H_

#include <gtk/gtk.h>

void showErrorMessage (GtkWindow *parent, const gchar *msg);
void showWarningMessage (GtkWindow *parent, const gchar *msg);
gint showYesNoDialog (GtkWindow *parent, const gchar *msg);
gint showYesNoCancelDialog (GtkWindow *parent, const gchar *msg);

#endif // R_GTKMESSAGEDIALOGS

