/*
 * preferencias.h
 *
 *  Created on: 02/06/2011
 *      Author: rodolfo
 */

#ifndef PREFERENCIAS_H_
#define PREFERENCIAS_H_

#include <gtk/gtk.h>
#include "hexview.h"

// Ao adicionar um campo, iniciá-lo em inicia_preferencias_padrao.
// Acrescentar a leitura em interpreta_preferencias
// Configurar o diálogo de preferência em configura_dialogo
// Verificar se precisa destruir em destroi_preferencias
// Se for um estado a decorar, inserir código em salvar_preferencias
typedef struct Preferencias
{
	// Arquivos
	gboolean lembrar_diretorio;
		gchar *ultimo_diretorio;
	gboolean lembrar_arquivos_recentes;
	gint qtd_arquivos_recentes;
		GList* arquivos_recentes;
	gint tamanho_buffer_arquivo;
	// Aparência
		// Fonte
	gboolean fonte_padrao;
	gchar *familia_fonte;
	gchar *familia_fonte_texto;
	gint tamanho_fonte;
	GdkColor cor_fonte;
	guint16 alfa_fonte;
		// Seleção
	gboolean selecao_padrao;
	GdkColor cor_selecao;
	guint16 alfa_selecao;
		// Cursor
	gboolean cursor_padrao;
	GdkColor cor_contorno_cursor;
	guint16 alfa_contorno_cursor;
	GdkColor cor_fundo_cursor;
	guint16 alfa_fundo_cursor;

	gboolean salvar_posicao_tamanho_janela;
		gint janela_x, janela_y;
		gint janela_w, janela_h;
	// Painel Bytes
	gint qtd_bytes_por_linha;
	// Painel Texto
	gchar * codificacao_texto;
	gchar * caractere_desconhecido; // FIXME: Texto?!
	gchar * caractere_nulo; // FIXME: Texto?!
	gchar * caractere_novalinha; // FIXME: Texto?!
	gchar * caractere_fimmensagem; // FIXME: Texto?!
} Preferencias;

void inicia_preferencias_padrao(Preferencias *preferencias);
gboolean carrega_arquivo_preferencias(XChangeHexView *hexv, Preferencias *preferencias);
void salva_preferencias_se_necessario();
void destroi_preferencias(Preferencias *preferencias);
gboolean abrir_dialogo_preferencias(const Preferencias *preferencias);

#endif /* PREFERENCIAS_H_ */
