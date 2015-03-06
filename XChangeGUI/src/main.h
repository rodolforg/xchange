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

#ifndef MAIN_H_
#define MAIN_H_

#include <gtk/gtk.h>
#include "chartable.h"

void limpa_barra_de_estado(const char* contexto);
void pipoca_na_barra_de_estado(const char* contexto, const gchar* msg);

void elabora_menu_recentes();
extern GtkWidget *menu_item_abrir_recente;

extern gint qtd_tabelas;
extern XChangeTable **xt_tabela;

gboolean carrega_arquivo_interface(GtkBuilder *builder, const gchar *arquivo);

#endif /* MAIN_H_ */
