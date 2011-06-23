/*
 * preferencias.c
 *
 *  Created on: 02/06/2011
 *      Author: rodolfo
 */

#include "preferencias.h"
#include "hexview.h"
#include "main.h"

struct Janelas
{
	GtkWidget *hexv;

	GtkWidget *dialogo_preferencias;
	GtkWidget *table_fonte_personalizada;
	GtkWidget *fontbutton_painel_bytes;
	GtkWidget *fontbutton_painel_texto;
	GtkWidget *colorbutton_fonte;
	GtkWidget *table_cursor_personalizado;
	GtkWidget *colorbutton_contorno_cursor;
	GtkWidget *colorbutton_fundo_cursor;
	GtkWidget *colorbutton_realce_selecao;
	GtkWidget *table_selecao_personalizada;

	GtkAction *toggleaction_lembrar_ultimo_diretorio;
	GtkAction *toggleaction_lembrar_ultimos_arquivos;
	GtkWidget *hbox_arquivos_recentes;
	GtkAdjustment *adjustment_quantidade_arquivos_recentes;
	GtkAdjustment *adjustment_qtd_memoria_por_arquivo;

	GtkAction *toggleaction_fonte_padrao;
	GtkAction *toggleaction_selecao_padrao;
	GtkAction *toggleaction_cursor_padrao;
	GtkAction *toggleaction_salva_pos_dim_janela;

	GtkAdjustment *adjustment_bytes_por_linha;

	GtkComboBox *comboboxentry_seletor_codificacao_caracteres_preferencial;
	GtkEntry *entry_caractere_para_desconhecido;
	GtkEntry *entry_caractere_para_nulo;
	GtkEntry *entry_caractere_para_novalinha;
	GtkEntry *entry_caractere_para_paragrafo;
};
typedef struct Janelas Janelas;

struct ManipuladorPreferencias
{
	Preferencias *preferencias;
	GKeyFile * keyfile;
	Janelas janelas;
//	Aplicativo aplicativo;
} dadosInternos;

// Preferências

static void interpreta_preferencias(GKeyFile * keyfile, Preferencias * preferencias);

void inicia_preferencias_padrao(Preferencias *preferencias)
{
	if (preferencias == NULL)
		return;

	// Arquivos
	preferencias->lembrar_diretorio = FALSE;
		preferencias->ultimo_diretorio = NULL;
	preferencias->lembrar_arquivos_recentes = TRUE;
	preferencias->qtd_arquivos_recentes = 5;
		preferencias->arquivos_recentes = NULL;
	preferencias->tamanho_buffer_arquivo = 16;
	// Aparência
	preferencias->fonte_padrao = TRUE;
		preferencias->familia_fonte = g_strdup("Mono");
		preferencias->familia_fonte_texto = g_strdup("Mono");
		gdk_color_parse("#000000", &preferencias->cor_fonte);
		preferencias->alfa_fonte = 65535;
	preferencias->selecao_padrao = TRUE;
		gdk_color_parse("#8080f3", &preferencias->cor_selecao);
		preferencias->alfa_selecao = 32768;
	preferencias->cursor_padrao = TRUE;
		gdk_color_parse("#f3f3f3", &preferencias->cor_contorno_cursor);
		preferencias->alfa_contorno_cursor = 45875;
		gdk_color_parse("#cdcdcd", &preferencias->cor_fundo_cursor);
		preferencias->alfa_fundo_cursor = 65535;

	preferencias->salvar_posicao_tamanho_janela = FALSE;
		preferencias->janela_x = 0;
		preferencias->janela_y = 0;
		preferencias->janela_w = 0;
		preferencias->janela_h = 0;
	// Painel Bytes
	preferencias->qtd_bytes_por_linha = 16;
	// Painel Texto
	const char *system_encoding;
	g_get_charset(&system_encoding);
	preferencias->codificacao_texto = g_strdup(system_encoding);
	preferencias->caractere_desconhecido = g_strdup("�");
	preferencias->caractere_nulo = g_strdup("_");
	preferencias->caractere_novalinha = g_strdup("⏎");
	preferencias->caractere_fimmensagem = g_strdup("¶");

	dadosInternos.preferencias = preferencias;
}

gboolean carrega_arquivo_preferencias(XChangeHexView *hexv, Preferencias *preferencias)
{
	GKeyFile * keyfile = g_key_file_new();
	if (keyfile == NULL)
	{
		pipoca_na_barra_de_estado("Preferências", "Não foi possível carregar preferências");
		return FALSE;
	}
	GError *erro = NULL;
	gchar * nomearq_pref = g_build_filename(g_get_user_config_dir(), g_get_prgname (), "prefs.key", NULL);

	if (!g_key_file_load_from_file(keyfile, nomearq_pref, G_KEY_FILE_KEEP_COMMENTS|G_KEY_FILE_KEEP_TRANSLATIONS, &erro))
	{
	}
	g_free(nomearq_pref);

	dadosInternos.janelas.hexv = GTK_WIDGET(hexv);
	dadosInternos.preferencias = preferencias;
	dadosInternos.keyfile = keyfile;
	interpreta_preferencias(keyfile, preferencias);

	limpa_barra_de_estado("Preferências");
	return TRUE;
}

static void ler_preferencia_booleano(GKeyFile *keyfile, const gchar *grupo, const char *chave, gboolean *valor)
{
	GError *erro = NULL;
	if (g_key_file_has_key (keyfile, grupo, chave, &erro) )
		*valor = g_key_file_get_boolean(keyfile, grupo, chave, &erro);
	if (erro != NULL)
	{
		if (erro->code != G_KEY_FILE_ERROR_GROUP_NOT_FOUND)
		{
			g_warning("Erro ao ler as preferências: %s\n", erro->message);
		}
		g_error_free(erro);
		erro = NULL;
	}
}

static void ler_preferencia_inteiro(GKeyFile *keyfile, const gchar *grupo, const char *chave, gboolean *valor)
{
	GError *erro = NULL;
	if (g_key_file_has_key (keyfile, grupo, chave, &erro) )
		*valor = g_key_file_get_integer(keyfile, grupo, chave, &erro);
	if (erro != NULL)
	{
		if (erro->code != G_KEY_FILE_ERROR_GROUP_NOT_FOUND)
		{
			g_warning("Erro ao ler as preferências: %s\n", erro->message);
		}
		g_error_free(erro);
		erro = NULL;
	}
}

static void ler_preferencia_texto(GKeyFile *keyfile, const gchar *grupo, const char *chave, gchar **valor)
{
	GError *erro = NULL;
	gchar *texto = NULL;
	if (g_key_file_has_key (keyfile, grupo, chave, &erro) )
		texto = g_key_file_get_string(keyfile, grupo, chave, &erro);
	if (erro != NULL)
	{
		if (erro->code != G_KEY_FILE_ERROR_GROUP_NOT_FOUND)
		{
			g_warning("Erro ao ler as preferências: %s\n", erro->message);
		}
		g_error_free(erro);
		erro = NULL;
	}
	if (texto != NULL)
	{
		g_free(*valor);
		*valor = texto;
	}
}

static void ler_preferencia_cor(GKeyFile *keyfile, const gchar *grupo, const char *chave, GdkColor *cor, guint16 *alfa)
{
	GError *erro = NULL;
	gint *lista;
	gsize tamanho;
	if (g_key_file_has_key (keyfile, grupo, chave, &erro) )
	{
		lista = g_key_file_get_integer_list(keyfile, grupo, chave, &tamanho, &erro);
		if (lista != NULL)
		{
			if (tamanho >= 3)
			{
				cor->red = lista[0];
				cor->green = lista[1];
				cor->blue = lista[2];
			}
			if (tamanho >= 4 && alfa != NULL)
				*alfa = lista[3];
		}
	}
	if (erro != NULL)
	{
		if (erro->code != G_KEY_FILE_ERROR_GROUP_NOT_FOUND)
		{
			g_warning("Erro ao ler as preferências: %s\n", erro->message);
		}
		g_error_free(erro);
		erro = NULL;
	}
}

static void ler_preferencia_lista_inteiros(GKeyFile *keyfile, const gchar *grupo, const char *chave, gint **lista, gsize *comprimento)
{
	GError *erro = NULL;
	if (g_key_file_has_key (keyfile, grupo, chave, &erro) )
	{
		*lista = g_key_file_get_integer_list(keyfile, grupo, chave, comprimento, &erro);

	}
	if (erro != NULL)
	{
		if (erro->code != G_KEY_FILE_ERROR_GROUP_NOT_FOUND)
		{
			g_warning("Erro ao ler as preferências: %s\n", erro->message);
		}
		g_error_free(erro);
		erro = NULL;
	}
}

static void interpreta_preferencias(GKeyFile *keyfile, struct Preferencias * preferencias)
{
	if (keyfile == NULL || preferencias == NULL)
		return;

	GtkWidget *hexv = dadosInternos.janelas.hexv;

	// Arquivos
	ler_preferencia_booleano(keyfile, "Arquivos", "Lembrar diretório", &preferencias->lembrar_diretorio);
	if (preferencias->lembrar_diretorio)
	{
		//FIXME: último diretório
		ler_preferencia_texto(keyfile, "Arquivos", "Último diretório", &preferencias->ultimo_diretorio);
	}

	ler_preferencia_booleano(keyfile, "Arquivos", "Lembrar arquivos recentes", &preferencias->lembrar_arquivos_recentes);
	if (preferencias->lembrar_arquivos_recentes)
	{
		ler_preferencia_inteiro(keyfile, "Arquivos", "Quantidade arquivos recentes", &preferencias->qtd_arquivos_recentes);
		// Ler arquivos recentes...
		int n;
		for (n = 0; n < preferencias->qtd_arquivos_recentes; n++)
		{
			gchar *arquivo = NULL;
			gchar *nome_chave = g_strdup_printf("%i", n);
			ler_preferencia_texto(keyfile, "Recentes", nome_chave, &arquivo);
			g_free(nome_chave);
			if (arquivo != NULL)
			{
				preferencias->arquivos_recentes = g_list_append(preferencias->arquivos_recentes, arquivo);
			}
		}
	}
	ler_preferencia_inteiro(keyfile, "Arquivos", "Limite de memória reservada", &preferencias->tamanho_buffer_arquivo);

	// Aparência
	ler_preferencia_booleano(keyfile, "Aparência", "Fonte padrão", &preferencias->fonte_padrao);

	ler_preferencia_texto(keyfile, "Aparência", "Nome da fonte", &preferencias->familia_fonte);
	if (!preferencias->fonte_padrao)
		xchange_hex_view_set_byte_font_name(XCHANGE_HEX_VIEW(hexv), preferencias->familia_fonte);
	ler_preferencia_texto(keyfile, "Aparência", "Nome da fonte (texto)", &preferencias->familia_fonte_texto);
	if (!preferencias->fonte_padrao)
		xchange_hex_view_set_text_font_name(XCHANGE_HEX_VIEW(hexv), preferencias->familia_fonte_texto);
	ler_preferencia_inteiro(keyfile, "Aparência", "Tamanho da fonte", &preferencias->tamanho_fonte);
	if (!preferencias->fonte_padrao)
		xchange_hex_view_set_font_size(XCHANGE_HEX_VIEW(hexv), preferencias->tamanho_fonte);
	ler_preferencia_cor(keyfile, "Aparência", "Cor da fonte", &preferencias->cor_fonte, &preferencias->alfa_fonte);
	if (!preferencias->fonte_padrao)
		xchange_hex_view_set_font_color(XCHANGE_HEX_VIEW(hexv), preferencias->cor_fonte, preferencias->alfa_fonte);
	ler_preferencia_booleano(keyfile, "Aparência", "Seleção padrão", &preferencias->selecao_padrao);
	ler_preferencia_cor(keyfile, "Aparência", "Cor da seleção", &preferencias->cor_selecao, &preferencias->alfa_selecao);
	if (!preferencias->selecao_padrao)
		xchange_hex_view_set_selection_color(XCHANGE_HEX_VIEW(hexv), preferencias->cor_selecao, preferencias->alfa_selecao);
	ler_preferencia_booleano(keyfile, "Aparência", "Cursor padrão", &preferencias->cursor_padrao);
	ler_preferencia_cor(keyfile, "Aparência", "Cor de contorno do cursor", &preferencias->cor_contorno_cursor, &preferencias->alfa_contorno_cursor);
	ler_preferencia_cor(keyfile, "Aparência", "Cor de fundo do cursor", &preferencias->cor_fundo_cursor, &preferencias->alfa_fundo_cursor);
	if (!preferencias->cursor_padrao)
	{
		xchange_hex_view_set_cursor_background_color(XCHANGE_HEX_VIEW(hexv), preferencias->cor_fundo_cursor, preferencias->alfa_fundo_cursor);
		xchange_hex_view_set_cursor_foreground_color(XCHANGE_HEX_VIEW(hexv), preferencias->cor_contorno_cursor, preferencias->alfa_contorno_cursor);
	}
	ler_preferencia_booleano(keyfile, "Aparência", "Salva posição da janela", &preferencias->salvar_posicao_tamanho_janela);
	// TODO: Carregar dimensões e posição da janela
	{
		gint *pos = NULL, *tam = NULL;
		gsize tamanho_pos, tamanho_tam;
		GtkWindow * window = GTK_WINDOW(gtk_widget_get_toplevel(dadosInternos.janelas.hexv));
		ler_preferencia_lista_inteiros(keyfile, "Aparência", "Posição da janela", &pos, &tamanho_pos);
		ler_preferencia_lista_inteiros(keyfile, "Aparência", "Tamanho da janela", &tam, &tamanho_tam);
		if (pos != NULL && tamanho_pos > 0)
		{
			preferencias->janela_x = pos[0];
			preferencias->janela_y = pos[1];
			gtk_window_move(window, pos[0], pos[1]);
			g_free(pos);
		}
		if (tam != NULL && tamanho_tam > 0)
		{
			preferencias->janela_w = tam[0];
			preferencias->janela_h = tam[1];
			gtk_window_resize(window, tam[0], tam[1]);
			g_free(tam);
		}
	}

	// Painel de bytes
	ler_preferencia_inteiro(keyfile, "Painel Bytes", "Bytes por linha", &preferencias->qtd_bytes_por_linha);
	xchange_hex_view_set_byte_row_length(XCHANGE_HEX_VIEW(hexv), preferencias->qtd_bytes_por_linha);

	// Painel de texto
	ler_preferencia_texto(keyfile, "Painel Texto", "Codificação padrão", &preferencias->codificacao_texto);
	// TODO: ativar codificação?
	ler_preferencia_texto(keyfile, "Painel Texto", "Caractere desconhecido", &preferencias->caractere_desconhecido);
	// TODO: ativar caractere desconhecido?
	ler_preferencia_texto(keyfile, "Painel Texto", "Caractere nulo", &preferencias->caractere_nulo);
	// TODO: ativar caractere nulo?
	ler_preferencia_texto(keyfile, "Painel Texto", "Caractere quebra de linha", &preferencias->caractere_novalinha);
	// TODO: ativar caractere novalinha?
	ler_preferencia_texto(keyfile, "Painel Texto", "Caractere fim de mensagem", &preferencias->caractere_fimmensagem);
	// TODO: ativar caractere fim mensagem?
}

void salva_preferencias_se_necessario()
{
	GKeyFile * keyfile = dadosInternos.keyfile;
	if (keyfile == NULL)
		return;
	gsize length;
	GError *erro = NULL;

	// Último diretorio
	if (dadosInternos.preferencias->lembrar_diretorio && dadosInternos.preferencias->ultimo_diretorio != NULL)
	{
		g_key_file_set_string(keyfile, "Arquivos", "Último diretório", dadosInternos.preferencias->ultimo_diretorio);
	}
	// Arquivos recentes
	g_key_file_remove_group(keyfile, "Recentes", NULL);
	if (dadosInternos.preferencias->lembrar_arquivos_recentes)
	{
		GList *item = g_list_first(dadosInternos.preferencias->arquivos_recentes);
		gint n = 0;
		while (item != NULL)
		{
			gchar *chave = g_strdup_printf("%i", n);
			if (n < dadosInternos.preferencias->qtd_arquivos_recentes)
			{
				g_key_file_set_string(keyfile, "Recentes", chave, item->data);
			}
			g_free(chave);
			n ++;
			item = g_list_next(item);
		}
	}
	// Salva posição da janela
	if (dadosInternos.preferencias->salvar_posicao_tamanho_janela)
	{
		gint pos[2], tam[2];
		pos[0] = dadosInternos.preferencias->janela_x;
		pos[1] = dadosInternos.preferencias->janela_y;
		tam[0] = dadosInternos.preferencias->janela_w;
		tam[1] = dadosInternos.preferencias->janela_h;
		g_key_file_set_integer_list(keyfile, "Aparência", "Posição da janela", pos, 2);
		g_key_file_set_integer_list(keyfile, "Aparência", "Tamanho da janela", tam, 2);
	}

	gchar ** grupos = g_key_file_get_groups(keyfile, &length);
	if (length == 0)
	{
		return;
	}
	g_strfreev(grupos);

	gchar *file_content = g_key_file_to_data(keyfile, &length, &erro);
	if (file_content == NULL)
	{
	// FIXME: Conferir erro
		pipoca_na_barra_de_estado("Preferências", erro->message);
		g_error_free(erro);
		return;
	}

	gchar * nomearq_pref = g_build_filename(g_get_user_config_dir(), g_get_prgname (), "prefs.key", NULL);
	gchar * pastaArq_pref = g_build_filename(g_get_user_config_dir(), g_get_prgname (), NULL);

	// Diretório de configurações existe?
	if (!g_file_test (pastaArq_pref, G_FILE_TEST_EXISTS|G_FILE_TEST_IS_DIR))
	{
		g_mkdir_with_parents(pastaArq_pref, 0744);
	}
	g_free(pastaArq_pref);

	if (!g_file_set_contents(nomearq_pref, file_content, length, &erro))
	{
		// FIXME: Conferir erro
		pipoca_na_barra_de_estado("Preferências", erro->message);
		g_warning("%s\n", erro->message);
		g_error_free(erro);
		erro = NULL;
		g_free(nomearq_pref);
		return;
	}
	g_free(nomearq_pref);
}

void destroi_preferencias(Preferencias *preferencias)
{
	if (preferencias == NULL)
		return;

#if GLIB_CHECK_VERSION(2,28,0)
	g_list_free_full(preferencias->arquivos_recentes, g_free);
#else
	GList *arq_rec = g_list_first(preferencias->arquivos_recentes);
	while (arq_rec)
	{
		g_free(arq_rec->data);
		arq_rec = g_list_next(arq_rec);
	}
	g_list_free(arq_rec);
#endif
	g_free(preferencias->familia_fonte);
	g_free(preferencias->familia_fonte_texto);
	g_free(preferencias->codificacao_texto);
	g_free(preferencias->caractere_desconhecido);
	g_free(preferencias->caractere_nulo);
	g_free(preferencias->caractere_novalinha);
	g_free(preferencias->caractere_fimmensagem);

	g_key_file_free(dadosInternos.keyfile); // FIXME: Global? (dadosInternos)
}

// Diálogo de preferências

static gboolean carrega_dialogo_do_builder(struct ManipuladorPreferencias *dados)
{
	GtkBuilder * builder = gtk_builder_new();

	GError *err = NULL;
	gtk_builder_add_from_file(builder, "prefs.gui", &err);
	if (err != NULL)
	{
		g_warning("Erro: %s\n", err->message);
		g_error_free(err);
		return FALSE;
	}

	gtk_builder_connect_signals(builder, dados); // FIXME: Alocar dinamicamente?

	dados->janelas.dialogo_preferencias = GTK_WIDGET(gtk_builder_get_object(builder, "dialog_preferencias"));
	dados->janelas.toggleaction_lembrar_ultimo_diretorio = GTK_ACTION(gtk_builder_get_object(builder, "toggleaction_lembrar_ultimo_diretorio"));
	dados->janelas.toggleaction_lembrar_ultimos_arquivos = GTK_ACTION(gtk_builder_get_object(builder, "toggleaction_lembrar_ultimos_arquivos"));
	dados->janelas.hbox_arquivos_recentes = GTK_WIDGET(gtk_builder_get_object(builder, "hbox_arquivos_recentes"));
	dados->janelas.adjustment_quantidade_arquivos_recentes = GTK_ADJUSTMENT(gtk_builder_get_object(builder, "adjustment_quantidade_arquivos_recentes"));
	dados->janelas.adjustment_qtd_memoria_por_arquivo = GTK_ADJUSTMENT(gtk_builder_get_object(builder, "adjustment_qtd_memoria_por_arquivo"));

	dados->janelas.toggleaction_fonte_padrao = GTK_ACTION(gtk_builder_get_object(builder, "toggleaction_fonte_padrao"));
	dados->janelas.toggleaction_selecao_padrao = GTK_ACTION(gtk_builder_get_object(builder, "toggleaction_selecao_padrao"));
	dados->janelas.toggleaction_cursor_padrao = GTK_ACTION(gtk_builder_get_object(builder, "toggleaction_cursor_padrao"));
	dados->janelas.toggleaction_salva_pos_dim_janela = GTK_ACTION(gtk_builder_get_object(builder, "toggleaction_salva_pos_dim_janela"));
	dados->janelas.table_fonte_personalizada = GTK_WIDGET(gtk_builder_get_object(builder, "table_fonte_personalizada"));
	dados->janelas.fontbutton_painel_bytes = GTK_WIDGET(gtk_builder_get_object(builder, "fontbutton_painel_bytes"));
	dados->janelas.fontbutton_painel_texto = GTK_WIDGET(gtk_builder_get_object(builder, "fontbutton_painel_texto"));
	dados->janelas.colorbutton_fonte = GTK_WIDGET(gtk_builder_get_object(builder, "colorbutton_fonte"));
	dados->janelas.table_cursor_personalizado = GTK_WIDGET(gtk_builder_get_object(builder, "table_cursor_personalizado"));
	dados->janelas.colorbutton_contorno_cursor = GTK_WIDGET(gtk_builder_get_object(builder, "colorbutton_contorno_cursor"));
	dados->janelas.colorbutton_fundo_cursor = GTK_WIDGET(gtk_builder_get_object(builder, "colorbutton_fundo_cursor"));
	dados->janelas.colorbutton_fonte = GTK_WIDGET(gtk_builder_get_object(builder, "colorbutton_fonte"));
	dados->janelas.colorbutton_realce_selecao = GTK_WIDGET(gtk_builder_get_object(builder, "colorbutton_realce_selecao"));
	dados->janelas.table_selecao_personalizada = GTK_WIDGET(gtk_builder_get_object(builder, "table_selecao_personalizada"));

	dados->janelas.adjustment_bytes_por_linha = GTK_ADJUSTMENT(gtk_builder_get_object(builder, "adjustment_bytes_por_linha"));

	dados->janelas.comboboxentry_seletor_codificacao_caracteres_preferencial = GTK_COMBO_BOX(gtk_builder_get_object(builder, "comboboxentry_seletor_codificacao_caracteres_preferencial"));
	dados->janelas.entry_caractere_para_desconhecido = GTK_ENTRY(gtk_builder_get_object(builder, "entry_caractere_para_desconhecido"));
	dados->janelas.entry_caractere_para_nulo = GTK_ENTRY(gtk_builder_get_object(builder, "entry_caractere_para_nulo"));
	dados->janelas.entry_caractere_para_novalinha = GTK_ENTRY(gtk_builder_get_object(builder, "entry_caractere_para_novalinha"));
	dados->janelas.entry_caractere_para_paragrafo = GTK_ENTRY(gtk_builder_get_object(builder, "entry_caractere_para_paragrafo"));

	g_object_unref(builder);

	return TRUE;
}

static void configura_dialogo(const struct ManipuladorPreferencias *dados)
{
	// Arquivos
	gtk_toggle_action_set_active(GTK_TOGGLE_ACTION(dados->janelas.toggleaction_lembrar_ultimo_diretorio), dados->preferencias->lembrar_diretorio);
	gtk_toggle_action_set_active(GTK_TOGGLE_ACTION(dados->janelas.toggleaction_lembrar_ultimos_arquivos), dados->preferencias->lembrar_arquivos_recentes);
	gtk_adjustment_set_value(GTK_ADJUSTMENT(dados->janelas.adjustment_quantidade_arquivos_recentes), dados->preferencias->qtd_arquivos_recentes);
	gtk_adjustment_set_value(GTK_ADJUSTMENT(dados->janelas.adjustment_qtd_memoria_por_arquivo), dados->preferencias->tamanho_buffer_arquivo);
	// Aparência
		// Fonte
	gtk_toggle_action_set_active(GTK_TOGGLE_ACTION(dados->janelas.toggleaction_fonte_padrao), dados->preferencias->fonte_padrao);
	gchar *nome_fonte;
	nome_fonte = g_strdup_printf("%s %i", dados->preferencias->familia_fonte, dados->preferencias->tamanho_fonte);
	gtk_font_button_set_font_name(GTK_FONT_BUTTON(dados->janelas.fontbutton_painel_bytes), nome_fonte);
	g_free(nome_fonte);
	nome_fonte = g_strdup_printf("%s %i", dados->preferencias->familia_fonte_texto, dados->preferencias->tamanho_fonte);
	gtk_font_button_set_font_name(GTK_FONT_BUTTON(dados->janelas.fontbutton_painel_texto), nome_fonte);
	g_free(nome_fonte);

	gtk_color_button_set_color(GTK_COLOR_BUTTON(dados->janelas.colorbutton_fonte), &dados->preferencias->cor_fonte);
	gtk_color_button_set_alpha(GTK_COLOR_BUTTON(dados->janelas.colorbutton_fonte), dados->preferencias->alfa_fonte);
		// Seleção
	gtk_toggle_action_set_active(GTK_TOGGLE_ACTION(dados->janelas.toggleaction_selecao_padrao), dados->preferencias->selecao_padrao);
	gtk_color_button_set_color(GTK_COLOR_BUTTON(dados->janelas.colorbutton_realce_selecao), &dados->preferencias->cor_selecao);
	gtk_color_button_set_alpha(GTK_COLOR_BUTTON(dados->janelas.colorbutton_realce_selecao), dados->preferencias->alfa_selecao);
		// Cursor
	gtk_toggle_action_set_active(GTK_TOGGLE_ACTION(dados->janelas.toggleaction_cursor_padrao), dados->preferencias->cursor_padrao);
	gtk_color_button_set_color(GTK_COLOR_BUTTON(dados->janelas.colorbutton_contorno_cursor), &dados->preferencias->cor_contorno_cursor);
	gtk_color_button_set_alpha(GTK_COLOR_BUTTON(dados->janelas.colorbutton_contorno_cursor), dados->preferencias->alfa_contorno_cursor);
	gtk_color_button_set_color(GTK_COLOR_BUTTON(dados->janelas.colorbutton_fundo_cursor), &dados->preferencias->cor_fundo_cursor);
	gtk_color_button_set_alpha(GTK_COLOR_BUTTON(dados->janelas.colorbutton_fundo_cursor), dados->preferencias->alfa_fundo_cursor);
		// Salvar pos/tam janela
	gtk_toggle_action_set_active(GTK_TOGGLE_ACTION(dados->janelas.toggleaction_salva_pos_dim_janela), dados->preferencias->salvar_posicao_tamanho_janela);

	// Painel Bytes
	gtk_adjustment_set_value(GTK_ADJUSTMENT(dados->janelas.adjustment_bytes_por_linha), dados->preferencias->qtd_bytes_por_linha);
	// Painel Texto
	// TODO: Codificação preferencial por iter? E padrão do sistema?
	gtk_entry_set_text (GTK_ENTRY (gtk_bin_get_child (GTK_BIN (dados->janelas.comboboxentry_seletor_codificacao_caracteres_preferencial))), dados->preferencias->codificacao_texto);
	gtk_entry_set_text(GTK_ENTRY(dados->janelas.entry_caractere_para_desconhecido), dados->preferencias->caractere_desconhecido);
	gtk_entry_set_text(GTK_ENTRY(dados->janelas.entry_caractere_para_nulo), dados->preferencias->caractere_nulo);
	gtk_entry_set_text(GTK_ENTRY(dados->janelas.entry_caractere_para_novalinha), dados->preferencias->caractere_novalinha);
	gtk_entry_set_text(GTK_ENTRY(dados->janelas.entry_caractere_para_paragrafo), dados->preferencias->caractere_fimmensagem);

}

gboolean abrir_dialogo_preferencias(const Preferencias *preferencias)
{
	if (! carrega_dialogo_do_builder(&dadosInternos))
		return FALSE;

	configura_dialogo(&dadosInternos);
	gtk_dialog_run(GTK_DIALOG(dadosInternos.janelas.dialogo_preferencias));
	return TRUE;
}

G_MODULE_EXPORT
void on_action_fechar_preferencias_activate(GtkAction *action, gpointer data)
{
	struct ManipuladorPreferencias *dados = data;
	gtk_widget_destroy(dados->janelas.dialogo_preferencias);
}

G_MODULE_EXPORT
void on_toggleaction_lembrar_ultimo_diretorio_toggled(GtkToggleAction *toggleaction, gpointer data)
{
	struct ManipuladorPreferencias *dados = data;
	gboolean value = gtk_toggle_action_get_active(toggleaction);
	g_key_file_set_boolean(dados->keyfile, "Arquivos", "Lembrar diretório", value);
	dados->preferencias->lembrar_diretorio = gtk_toggle_action_get_active(toggleaction);
}

G_MODULE_EXPORT
void on_toggleaction_lembrar_ultimos_arquivos_toggled(GtkToggleAction *toggleaction, gpointer data)
{
	struct ManipuladorPreferencias *dados = data;
	gboolean value = gtk_toggle_action_get_active(toggleaction);
	gtk_widget_set_sensitive(dados->janelas.hbox_arquivos_recentes, value);
	g_key_file_set_boolean(dados->keyfile, "Arquivos", "Lembrar arquivos recentes", value);
	dados->preferencias->lembrar_arquivos_recentes = gtk_toggle_action_get_active(toggleaction);
	if (value)
		gtk_widget_show(menu_item_abrir_recente);
	else
		gtk_widget_hide(menu_item_abrir_recente);
}

G_MODULE_EXPORT
void on_adjustment_quantidade_arquivos_recentes_value_changed(GtkAdjustment *adjustment, gpointer data)
{
	struct ManipuladorPreferencias *dados = data;
	gdouble value = gtk_adjustment_get_value(adjustment);
	g_key_file_set_integer(dados->keyfile, "Arquivos", "Quantidade arquivos recentes", value);
	dados->preferencias->qtd_arquivos_recentes = value;
	elabora_menu_recentes();
}

G_MODULE_EXPORT
void on_adjustment_qtd_memoria_por_arquivo_value_changed(GtkAdjustment *adjustment, gpointer data)
{
	struct ManipuladorPreferencias *dados = data;
	gdouble value = gtk_adjustment_get_value(adjustment);
	g_key_file_set_integer(dados->keyfile, "Arquivos", "Limite de memória reservada", value);
	dados->preferencias->tamanho_buffer_arquivo = value;
}

G_MODULE_EXPORT
void on_toggleaction_salva_pos_dim_janela_toggled(GtkToggleAction *toggleaction, gpointer data)
{
	struct ManipuladorPreferencias *dados = data;
	gboolean value = gtk_toggle_action_get_active(toggleaction);
	g_key_file_set_boolean(dados->keyfile, "Aparência", "Salva posição da janela", value);
	dados->preferencias->salvar_posicao_tamanho_janela = value;
}

G_MODULE_EXPORT
void on_adjustment_bytes_por_linha_value_changed(GtkAdjustment *adjustment, gpointer data)
{
	struct ManipuladorPreferencias *dados = data;
	gdouble row_length = gtk_adjustment_get_value(adjustment);
	xchange_hex_view_set_byte_row_length(XCHANGE_HEX_VIEW(dados->janelas.hexv), row_length);
	g_key_file_set_integer(dados->keyfile, "Painel Bytes", "BytesPorLinha", row_length);
	dados->preferencias->qtd_bytes_por_linha = row_length;
}

G_MODULE_EXPORT
void on_fontbutton_painel_bytes_font_set(GtkFontButton *fontbutton, gpointer data);

G_MODULE_EXPORT
void on_toggleaction_fonte_padrao_toggled(GtkToggleAction *toggleaction,
		gpointer data)
{
	struct ManipuladorPreferencias *dados = data;
	GtkWidget *hexv = dados->janelas.hexv;
	gboolean padrao = gtk_toggle_action_get_active(toggleaction);
	if (!padrao)
	{
		gtk_widget_set_sensitive(dados->janelas.table_fonte_personalizada, TRUE);
		//gtk_color_button_get_color(GTK_COLOR_BUTTON(dados->janelas.colorbutton_fonte), &cor);
		//alfa = gtk_color_button_get_alpha(GTK_COLOR_BUTTON(dados->janelas.colorbutton_fonte));
		xchange_hex_view_set_font_color(XCHANGE_HEX_VIEW(hexv), dados->preferencias->cor_fonte, dados->preferencias->alfa_fonte);
		// Nomes e tamanho....
		xchange_hex_view_set_font_size(XCHANGE_HEX_VIEW(hexv), dados->preferencias->tamanho_fonte);
		gchar *nome_fonte;
		nome_fonte = g_strdup_printf("%s %i", dados->preferencias->familia_fonte, dados->preferencias->tamanho_fonte);
		gtk_font_button_set_font_name(GTK_FONT_BUTTON(dados->janelas.fontbutton_painel_bytes), nome_fonte);
		g_free(nome_fonte);
		xchange_hex_view_set_byte_font_name(XCHANGE_HEX_VIEW(hexv), dados->preferencias->familia_fonte);

		nome_fonte = g_strdup_printf("%s %i", dados->preferencias->familia_fonte_texto, dados->preferencias->tamanho_fonte);
		gtk_font_button_set_font_name(GTK_FONT_BUTTON(dados->janelas.fontbutton_painel_texto), nome_fonte);
		g_free(nome_fonte);
		xchange_hex_view_set_text_font_name(XCHANGE_HEX_VIEW(hexv), dados->preferencias->familia_fonte_texto);
	}
	else
	{
		gtk_widget_set_sensitive(dados->janelas.table_fonte_personalizada, FALSE);
		xchange_hex_view_set_font_size(XCHANGE_HEX_VIEW(hexv), -1);
		xchange_hex_view_set_font_color_default(XCHANGE_HEX_VIEW(hexv));
		xchange_hex_view_set_byte_font_name(XCHANGE_HEX_VIEW(hexv), NULL);
		xchange_hex_view_set_text_font_name(XCHANGE_HEX_VIEW(hexv), NULL);
	}

	g_key_file_set_boolean(dados->keyfile, "Aparência", "Fonte padrão", padrao);
	dados->preferencias->fonte_padrao = padrao;
}

G_MODULE_EXPORT
void on_fontbutton_painel_bytes_font_set(GtkFontButton *fontbutton, gpointer data)
{
	struct ManipuladorPreferencias *dados = data;
	GtkWidget *hexv = dados->janelas.hexv;
	const gchar *fontname = gtk_font_button_get_font_name(fontbutton);
	PangoFontDescription * pg_desc = pango_font_description_from_string(fontname);

	// Font face
	if (pango_font_description_get_set_fields(pg_desc) & PANGO_FONT_MASK_FAMILY)
	{
		const char *family = pango_font_description_get_family(pg_desc);
		xchange_hex_view_set_byte_font_name(XCHANGE_HEX_VIEW(hexv), family);
		g_key_file_set_string(dados->keyfile, "Aparência", "Nome da fonte", family);
		g_free(dados->preferencias->familia_fonte);
		dados->preferencias->familia_fonte = g_strdup(family);
	}
	// Font size
	if (pango_font_description_get_set_fields(pg_desc) & PANGO_FONT_MASK_SIZE)
	{
		gint fontsize = pango_font_description_get_size(pg_desc);
		if (pango_font_description_get_size_is_absolute(pg_desc))
		{
			//g_print("Absolute\n");
			////fontsize /= PANGO_SCALE;
		}
		else
		{
			fontsize /= PANGO_SCALE;
		}
		xchange_hex_view_set_font_size(XCHANGE_HEX_VIEW(hexv), fontsize);
		g_key_file_set_integer(dados->keyfile, "Aparência", "Tamanho da fonte", fontsize);
		dados->preferencias->tamanho_fonte = fontsize;
	}
	pango_font_description_free(pg_desc);
}

G_MODULE_EXPORT
void on_fontbutton_painel_texto_font_set(GtkFontButton *fontbutton, gpointer data)
{
	struct ManipuladorPreferencias *dados = data;
	GtkWidget *hexv = dados->janelas.hexv;
	const gchar *fontname = gtk_font_button_get_font_name(fontbutton);
	PangoFontDescription * pg_desc = pango_font_description_from_string(fontname);

	// Font face
	if (pango_font_description_get_set_fields(pg_desc) & PANGO_FONT_MASK_FAMILY)
	{
		const char *family = pango_font_description_get_family(pg_desc);
		xchange_hex_view_set_text_font_name(XCHANGE_HEX_VIEW(hexv), family);
		g_key_file_set_string(dados->keyfile, "Aparência", "Nome da fonte (texto)", family);
		g_free(dados->preferencias->familia_fonte_texto);
		dados->preferencias->familia_fonte_texto = g_strdup(family);
	}
	/*
	// Font size
	if (pango_font_description_get_set_fields(pg_desc) & PANGO_FONT_MASK_SIZE)
	{
		gint fontsize = pango_font_description_get_size(pg_desc);
		if (pango_font_description_get_size_is_absolute(pg_desc))
		{
			//g_print("Absolute\n");
			////fontsize /= PANGO_SCALE;
		}
		else
		{
			fontsize /= PANGO_SCALE;
		}
		xchange_hex_view_set_font_size(XCHANGE_HEX_VIEW(hexv), fontsize);
		g_key_file_set_integer(dados->keyfile, "Aparência", "Tamanho da fonte", fontsize);
		dados->preferencias->tamanho_fonte = fontsize;
	}
	*/
	pango_font_description_free(pg_desc);
}

G_MODULE_EXPORT
void on_colorbutton_fonte_color_set(GtkColorButton *colorbutton,
		gpointer data)
{
	struct ManipuladorPreferencias *dados = data;
	GdkColor cor;
	guint16 alfa;
	gtk_color_button_get_color(GTK_COLOR_BUTTON(colorbutton), &cor);
	alfa = gtk_color_button_get_alpha(GTK_COLOR_BUTTON(colorbutton));
	xchange_hex_view_set_font_color(XCHANGE_HEX_VIEW(dados->janelas.hexv), cor, alfa);

	gint listaRGBA[4] = {cor.red, cor.green, cor.blue, alfa};
	g_key_file_set_integer_list(dados->keyfile, "Aparência", "Cor da fonte", listaRGBA, 4);
	dados->preferencias->cor_fonte = cor;
	dados->preferencias->alfa_fonte = alfa;
}


G_MODULE_EXPORT
void on_toggleaction_selecao_padrao_toggled(GtkToggleAction *toggleaction,
		gpointer data)
{
	struct ManipuladorPreferencias *dados = data;
	GtkWidget *hexv = dados->janelas.hexv;
	gboolean padrao = gtk_toggle_action_get_active(toggleaction);
	if (!padrao)
	{
		GdkColor cor;
		guint16 alfa;
		gtk_widget_set_sensitive(dados->janelas.table_selecao_personalizada, TRUE);
		//gtk_color_button_get_color(GTK_COLOR_BUTTON(dados->janelas.colorbutton_realce_selecao), &cor);
		//alfa = gtk_color_button_get_alpha(GTK_COLOR_BUTTON(dados->janelas.colorbutton_realce_selecao));
		cor = dados->preferencias->cor_selecao;
		alfa = dados->preferencias->alfa_selecao;
		xchange_hex_view_set_selection_color(XCHANGE_HEX_VIEW(hexv), cor, alfa);
	}
	else
	{
		gtk_widget_set_sensitive(dados->janelas.table_selecao_personalizada, FALSE);
		xchange_hex_view_set_selection_color_default(XCHANGE_HEX_VIEW(hexv));
	}
	g_key_file_set_boolean(dados->keyfile, "Aparência", "Seleção padrão", padrao);
	dados->preferencias->selecao_padrao = padrao;
}

G_MODULE_EXPORT
void on_colorbutton_realce_selecao_color_set(GtkColorButton *colorbutton,
		gpointer data)
{
	struct ManipuladorPreferencias *dados = data;
	GdkColor cor;
	guint16 alfa;
	gtk_color_button_get_color(GTK_COLOR_BUTTON(colorbutton), &cor);
	alfa = gtk_color_button_get_alpha(GTK_COLOR_BUTTON(colorbutton));
	xchange_hex_view_set_selection_color(XCHANGE_HEX_VIEW(dados->janelas.hexv), cor, alfa);

	gint listaRGBA[4] = {cor.red, cor.green, cor.blue, alfa};
	g_key_file_set_integer_list(dados->keyfile, "Aparência", "Cor da seleção", listaRGBA, 4);
	dados->preferencias->cor_selecao = cor;
	dados->preferencias->alfa_selecao = alfa;
}

G_MODULE_EXPORT
void on_toggleaction_cursor_padrao_toggled(GtkToggleAction *toggleaction,
		gpointer data)
{
	struct ManipuladorPreferencias *dados = data;
	GtkWidget *hexv = dados->janelas.hexv;
	gboolean padrao = gtk_toggle_action_get_active(toggleaction);
	if (!padrao)
	{
		GdkColor cor;
		guint16 alfa;
		gtk_widget_set_sensitive(dados->janelas.table_cursor_personalizado, TRUE);
		// Contorno
		//gtk_color_button_get_color(GTK_COLOR_BUTTON(dados->janelas.colorbutton_contorno_cursor), &cor);
		//alfa = gtk_color_button_get_alpha(GTK_COLOR_BUTTON(dados->janelas.colorbutton_contorno_cursor));
		cor = dados->preferencias->cor_contorno_cursor;
		alfa = dados->preferencias->alfa_contorno_cursor;
		xchange_hex_view_set_cursor_foreground_color(XCHANGE_HEX_VIEW(hexv), cor, alfa);
		// Fundo
		//gtk_color_button_get_color(GTK_COLOR_BUTTON(dados->janelas.colorbutton_fundo_cursor), &cor);
		//alfa = gtk_color_button_get_alpha(GTK_COLOR_BUTTON(dados->janelas.colorbutton_fundo_cursor));
		cor = dados->preferencias->cor_fundo_cursor;
		alfa = dados->preferencias->alfa_fundo_cursor;
		xchange_hex_view_set_cursor_background_color(XCHANGE_HEX_VIEW(hexv), cor, alfa);
	}
	else
	{
		gtk_widget_set_sensitive(dados->janelas.table_cursor_personalizado, FALSE);
		xchange_hex_view_set_cursor_foreground_color_default(XCHANGE_HEX_VIEW(hexv));
		xchange_hex_view_set_cursor_background_color_default(XCHANGE_HEX_VIEW(hexv));
	}

	g_key_file_set_boolean(dados->keyfile, "Aparência", "Cursor padrão", padrao);
	dados->preferencias->cursor_padrao = padrao;
}

G_MODULE_EXPORT
void on_colorbutton_contorno_cursor_color_set(GtkColorButton *colorbutton,
		gpointer data)
{
	struct ManipuladorPreferencias *dados = data;
	GdkColor cor;
	guint16 alfa;
	gtk_color_button_get_color(GTK_COLOR_BUTTON(colorbutton), &cor);
	alfa = gtk_color_button_get_alpha(GTK_COLOR_BUTTON(colorbutton));
	xchange_hex_view_set_cursor_foreground_color(XCHANGE_HEX_VIEW(dados->janelas.hexv), cor, alfa);

	gint listaRGBA[4] = {cor.red, cor.green, cor.blue, alfa};
	g_key_file_set_integer_list(dados->keyfile, "Aparência", "Cor de contorno do cursor", listaRGBA, 4);
	dados->preferencias->cor_contorno_cursor = cor;
	dados->preferencias->alfa_contorno_cursor = alfa;

}

G_MODULE_EXPORT
void on_colorbutton_fundo_cursor_color_set(GtkColorButton *colorbutton,
		gpointer data)
{
	struct ManipuladorPreferencias *dados = data;
	GdkColor cor;
	guint16 alfa;
	gtk_color_button_get_color(GTK_COLOR_BUTTON(colorbutton), &cor);
	alfa = gtk_color_button_get_alpha(GTK_COLOR_BUTTON(colorbutton));
	xchange_hex_view_set_cursor_background_color(XCHANGE_HEX_VIEW(dados->janelas.hexv), cor, alfa);

	gint listaRGBA[4] = {cor.red, cor.green, cor.blue, alfa};
	g_key_file_set_integer_list(dados->keyfile, "Aparência", "Cor de fundo do cursor", listaRGBA, 4);
	dados->preferencias->cor_fundo_cursor= cor;
	dados->preferencias->alfa_fundo_cursor = alfa;
}

G_MODULE_EXPORT
void on_comboboxentry_seletor_codificacao_caracteres_preferencial_changed(GtkComboBox *combobox, gpointer data)
{
	struct ManipuladorPreferencias *dados = data;
	char * codificacao = NULL;
	gint item = gtk_combo_box_get_active(combobox);
	gboolean padrao_esta_selecionado = FALSE;

	// Conferindo se é o primeiro item: Padrão do sistema
	GtkTreeIter iter_ativo;
	if (gtk_combo_box_get_active_iter(combobox, &iter_ativo))
	{
		GtkTreeModel *modelo = gtk_combo_box_get_model(combobox);
		GtkTreePath * path_ativo = gtk_tree_model_get_path(modelo, &iter_ativo);
		GtkTreePath * path_primeiro = gtk_tree_path_new_first();

		padrao_esta_selecionado = gtk_tree_path_compare(path_ativo, path_primeiro) == 0;

		gtk_tree_path_free(path_ativo);
		gtk_tree_path_free(path_primeiro);
	}

	if (item!=0 && !padrao_esta_selecionado)
	{
#if GTK_CHECK_VERSION(2,24,0)
		if (gtk_combo_box_get_has_entry(combobox))
		{
			const char *tmp_encoding2 = gtk_entry_get_text (GTK_ENTRY (gtk_bin_get_child (GTK_BIN (combobox))));
			if (tmp_encoding2 != NULL)
			{
				codificacao = g_strdup(tmp_encoding2);
			}
		}
		else if (GTK_IS_COMBO_BOX_TEXT(combobox))
		{
			codificacao = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(combobox));
		}
#else
		codificacao = gtk_combo_box_get_active_text(combobox);
#endif
	}
	if (codificacao!= NULL && (g_utf8_strlen(codificacao, -1) == 0 || g_strcmp0(codificacao, "Padrão do sistema") == 0))
	{
		g_free(codificacao);
		codificacao = NULL;
	}

	if (codificacao != NULL)
	{
		g_key_file_set_string(dados->keyfile, "Painel Texto", "Codificação padrão", codificacao);
		g_free(codificacao);
	}
	else
		g_key_file_remove_key(dados->keyfile, "Painel Texto", "Codificação padrão", NULL);
}

G_MODULE_EXPORT
void on_entry_caractere_para_desconhecido_changed(GtkEntry *entry, gpointer data)
{
	struct ManipuladorPreferencias *dados = data;
	const gchar *valor;

	if (xt_tabela == NULL)
		return;

	valor = gtk_entry_get_text(entry);
	if (g_utf8_strlen(valor, -1)!=0)
		g_key_file_set_string(dados->keyfile, "Painel Texto", "Caractere desconhecido", valor);
	else
	{
		g_key_file_remove_key(dados->keyfile, "Painel Texto", "Caractere desconhecido", NULL);
		valor = "�";
	}

	g_free(dados->preferencias->caractere_desconhecido);
	dados->preferencias->caractere_desconhecido = g_strdup(valor);
	int n;
	for (n = 0; n < qtd_tabelas; n++)
		if (xt_tabela[n] != NULL)
			xchange_table_set_unknown_char(xt_tabela[n], valor, -1);
	gtk_widget_queue_draw(dados->janelas.hexv);
}
