/*
 * main.h
 *
 *  Created on: 04/06/2011
 *      Author: rodolfo
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

#endif /* MAIN_H_ */
