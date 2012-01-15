/*
 * testes.c
 *
 *  Created on: 23/01/2011
 *      Author: rodolfo
 */

#include "main.h"

#include "file.h"
#include "chartable.h"

#include "hexview.h"
#include "preferencias.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <locale.h>
#ifndef __WIN32__
#include <langinfo.h>
#endif

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "message_dialogs.h"

#include "filehandler.h"


#define MAX_TABELAS 3

GtkWidget *hexv;
GtkWidget *main_window;

GtkWidget *menu_item_abrir_recente;

GtkWidget *spinbutton_cursor;

GtkWidget *dialog_ir_para;
GtkWidget *entry_valor_ir_para;

GtkWidget *filechooserdialog_carregar_texto;
GtkWidget *comboboxentry_codificacao_caracteres_arquivo;

GtkWidget *dialog_sobre;

GtkWidget *dialog_localizar;
GtkWidget *entry_valor_a_localizar;
GtkWidget *radiobutton_localizar_do_inicio;
GtkWidget *radiobutton_localizar_do_cursor;
GtkWidget *radiobutton_localizar_no_intervalo;
GtkWidget *table_localizar_no_intervalo;
GtkAdjustment *adjustment_inicio_localizar;
GtkAdjustment *adjustment_fim_localizar;
GtkWidget *spinbutton_inicio_localizar;
GtkWidget *spinbutton_fim_localizar;
GtkWidget *radiobutton_localizar_texto;
GtkWidget *radiobutton_localizar_bytes;
GtkWidget *checkbutton_localizar_para_tras;
GtkAction *action_localizar;

GtkWidget *vbox_substituir;
GtkWidget *entry_valor_para_substituir;
GtkWidget *button_localizar;
GtkWidget *button_substituir;
GtkWidget *button_substituir_todos;
GtkWidget *checkbutton_aceitar_alterar_tamanho_ao_substituir;
GtkWidget *vbox_substituir_por_tamanho_diferente;
GtkAction *action_substituir;
GtkAction *action_substituir_todos;

GtkAction *toggleaction_visao_tabela;
GtkAction *radioaction_visao_codificao_padronizada;
GtkAction *radioaction_visao_tabela1;
GtkAction *radioaction_visao_tabela2;
GtkAction *toggleaction_modo_insercao;
GtkAction *action_undo;
GtkAction *action_redo;

GtkWidget *statusbar;
GtkWidget *label_arquivo_modificado;

GtkWidget *dialog_codificacoes;
GtkWidget *comboboxentry_codificacao_caracteres;


XChangeTable **xt_tabela;
gint qtd_tabelas;
gint id_tabela_atual;

gchar *ultimo_diretorio;
char *nomeArquivoAtual;

struct DadosLocalizar
{
	gchar *texto_a_localizar;
	gboolean localizando_proximo;
	gboolean localizando_bytes;
	guint64 inicio;
	guint64 fim;
} dados_localizar;

GKeyFile* preferenciasKeyFile;
Preferencias preferencias;

gboolean file_changes_saved;
gint changes_until_last_save;

static void mudou_posicao_cursor(XChangeHexView *hexv, gpointer data);
static void mudou_modo_edicao(XChangeHexView *hexv, gpointer data);
static void mudou_selecao(XChangeHexView *hexv, gpointer data);
static void mudou_conteudo(XChangeHexView *hexv, gpointer data);


static void inicia_dados_localizar(struct DadosLocalizar *);
static void destroi_dados_localizar(struct DadosLocalizar *);
static uint8_t *recupera_bytes_de_texto_hexa(const gchar *texto, int *tamanho_bytes, const gchar *contexto);
static uint8_t *converte_pela_codificacao(const char *texto, int *tamanho_bytes, const char *contexto);

Filehandler *fh;

static void novo_arquivo(gpointer data);
static gboolean abre_arquivo(const char *nome_arquivo, gpointer data);
static gboolean salvar_arquivo(gpointer data);
static gboolean salvar_arquivo_como(const gchar *nome_arquivo, gpointer data);
static void fechar_arquivo();
static void acrescenta_aos_recentes(const gchar*nome_arquivo, gpointer data);

G_MODULE_EXPORT
void on_abrir_arquivo_definido_activate(GtkWidget *widget, gpointer data);

static void altera_padrao_codificacao_caracteres(char * encoding, gpointer data);

G_MODULE_EXPORT
void on_radioaction_visao_codificao_padronizada_changed(GtkRadioAction *action, GtkRadioAction *current, gpointer data);

G_MODULE_EXPORT
void on_fontbutton_painel_bytes_font_set(GtkFontButton *fontbutton, gpointer data);

/* liberar com g_strfreev*/
gchar **obtem_lista_data_dir()
{
	const gchar *nome_programa = g_get_prgname();
	const gchar * const * system_data_dirs = g_get_system_data_dirs ();
	gint m, max;
	for (m = 0; system_data_dirs[m]!=NULL; ++m)
		;
	max = m;

	const gchar * user_data_dir = g_get_user_data_dir ();

	gchar **diretorios = g_try_malloc(sizeof(gchar*)*(max+3));
	if (diretorios == NULL)
		return NULL;

	diretorios[0] = g_strdup("./");
	diretorios[1] = g_strdup_printf("%s/%s/", user_data_dir, nome_programa);
	for (m = 0; m < max; ++m) {
		diretorios[m+2] = g_strdup_printf("%s/%s/", system_data_dirs[m], nome_programa);
	}
	diretorios[max+2] = NULL;

	return diretorios;
}

gboolean carrega_arquivo_interface(GtkBuilder *builder, const gchar *arquivo)
{
	GError *err = NULL;
	gchar *caminho;
	gchar **pastas;

	pastas = obtem_lista_data_dir();
	if (pastas == NULL)
		return FALSE;

	int m;
	for (m = 0; pastas[m] != NULL; m++)
	{
		caminho = g_strdup_printf("%s%s", pastas[m], arquivo);
		gtk_builder_add_from_file(builder, caminho, &err);
		g_free(caminho);
		if (err != NULL)
		{
			g_error_free(err);
			err = NULL;
		}
		else
		{
			m = -1;
			break;
		}
	}
	g_strfreev(pastas);

	if (m >= 0)
		return FALSE;

	return TRUE;
}

static gboolean mostra_janela(Filehandler *fh)
{
	GtkBuilder * builder;

	builder = gtk_builder_new();
	if (builder == NULL)
		return FALSE;

	if (!carrega_arquivo_interface(builder, "gui.xml"))
	{
		g_object_unref(builder);
		return FALSE;
	}

	gtk_builder_connect_signals(builder, fh);

	main_window = GTK_WIDGET(gtk_builder_get_object(builder, "main_window"));
	menu_item_abrir_recente = GTK_WIDGET(gtk_builder_get_object(builder, "menuitem_abrir_recente"));

	GtkWidget * scrolled_window = GTK_WIDGET(gtk_builder_get_object(builder,
			"scrolledwindow"));

	spinbutton_cursor = GTK_WIDGET(gtk_builder_get_object(builder,
			"spinbutton_cursor"));

	dialog_ir_para = GTK_WIDGET(gtk_builder_get_object(builder,
			"dialog_ir_para"));
	entry_valor_ir_para = GTK_WIDGET(gtk_builder_get_object(builder,
			"entry_valor_ir_para"));

	filechooserdialog_carregar_texto = GTK_WIDGET(gtk_builder_get_object(builder,
			"filechooserdialog_carregar_texto"));
	comboboxentry_codificacao_caracteres_arquivo = GTK_WIDGET(gtk_builder_get_object(builder,
			"comboboxentry_codificacao_caracteres_arquivo"));

	dialog_localizar = GTK_WIDGET(gtk_builder_get_object(builder,
			"dialog_localizar"));
	entry_valor_a_localizar = GTK_WIDGET(gtk_builder_get_object(builder,
			"entry_valor_a_localizar"));
	radiobutton_localizar_do_inicio = GTK_WIDGET(gtk_builder_get_object(
			builder, "radiobutton_localizar_do_inicio"));
	radiobutton_localizar_do_cursor = GTK_WIDGET(gtk_builder_get_object(
			builder, "radiobutton_localizar_do_cursor"));
	radiobutton_localizar_no_intervalo = GTK_WIDGET(gtk_builder_get_object(
			builder, "radiobutton_localizar_no_intervalo"));
	table_localizar_no_intervalo = GTK_WIDGET(gtk_builder_get_object(
			builder, "table_localizar_no_intervalo"));
	adjustment_inicio_localizar = GTK_ADJUSTMENT(gtk_builder_get_object(
			builder, "adjustment_inicio_localizar"));
	adjustment_fim_localizar = GTK_ADJUSTMENT(gtk_builder_get_object(
			builder, "adjustment_fim_localizar"));
	spinbutton_inicio_localizar = GTK_WIDGET(gtk_builder_get_object(
			builder, "spinbutton_inicio_localizar"));
	spinbutton_fim_localizar = GTK_WIDGET(gtk_builder_get_object(
			builder, "spinbutton_fim_localizar"));
	radiobutton_localizar_texto = GTK_WIDGET(gtk_builder_get_object(
			builder, "radiobutton_localizar_texto"));
	radiobutton_localizar_bytes = GTK_WIDGET(gtk_builder_get_object(
			builder, "radiobutton_localizar_bytes"));
	checkbutton_localizar_para_tras = GTK_WIDGET(gtk_builder_get_object(
			builder, "checkbutton_localizar_para_tras"));
	action_localizar = GTK_ACTION(gtk_builder_get_object(
			builder, "action_localizar"));

	vbox_substituir = GTK_WIDGET(gtk_builder_get_object(
			builder, "vbox_substituir"));
	entry_valor_para_substituir = GTK_WIDGET(gtk_builder_get_object(
			builder, "entry_valor_para_substituir"));
	button_localizar = GTK_WIDGET(gtk_builder_get_object(
			builder, "button_localizar"));
	button_substituir = GTK_WIDGET(gtk_builder_get_object(
			builder, "button_substituir"));
	button_substituir_todos = GTK_WIDGET(gtk_builder_get_object(
			builder, "button_substituir_todos"));
	checkbutton_aceitar_alterar_tamanho_ao_substituir = GTK_WIDGET(gtk_builder_get_object(
			builder, "checkbutton_aceitar_alterar_tamanho_ao_substituir"));
	vbox_substituir_por_tamanho_diferente = GTK_WIDGET(gtk_builder_get_object(
			builder, "vbox_substituir_por_tamanho_diferente"));
	action_substituir = GTK_ACTION(gtk_builder_get_object(
			builder, "action_substituir"));
	action_substituir_todos = GTK_ACTION(gtk_builder_get_object(
			builder, "action_substituir_todos"));

	toggleaction_visao_tabela = GTK_ACTION(gtk_builder_get_object(
			builder, "toggleaction_visao_tabela"));
	radioaction_visao_codificao_padronizada = GTK_ACTION(gtk_builder_get_object(
			builder, "radioaction_visao_codificao_padronizada"));
	radioaction_visao_tabela1 = GTK_ACTION(gtk_builder_get_object(builder,
			"radioaction_visao_tabela1"));
	radioaction_visao_tabela2 = GTK_ACTION(gtk_builder_get_object(builder,
			"radioaction_visao_tabela2"));

	toggleaction_modo_insercao = GTK_ACTION(gtk_builder_get_object(
			builder, "toggleaction_modo_edicao"));

	action_undo = GTK_ACTION(gtk_builder_get_object(builder, "action_undo"));
	action_redo = GTK_ACTION(gtk_builder_get_object(builder, "action_redo"));

	dialog_sobre = GTK_WIDGET(gtk_builder_get_object(builder, "dialog_sobre"));

	statusbar = GTK_WIDGET(gtk_builder_get_object(builder, "statusbar"));
	label_arquivo_modificado = GTK_WIDGET(gtk_builder_get_object(builder, "label_arquivo_modificado"));

	dialog_codificacoes = GTK_WIDGET(gtk_builder_get_object(builder, "dialog_codificacoes"));
	comboboxentry_codificacao_caracteres = GTK_WIDGET(gtk_builder_get_object(builder,
			"comboboxentry_codificacao_caracteres"));

	// Configura as ações para o Filehandler
	fh->actions.close = GTK_ACTION(gtk_builder_get_object(builder, "action_fechar"));
	fh->actions.save = GTK_ACTION(gtk_builder_get_object(builder, "action_salvar"));
	fh->actions.save_as = GTK_ACTION(gtk_builder_get_object(builder, "action_salvar_como"));

	// Configura a janela principal para o Filehandler
	fh->main_window = main_window;

	g_object_unref(builder);

	//XChangeFile * xf = xchange_open(NULL, NULL);
	hexv = xchange_hex_view_new(NULL);
	gtk_container_add(GTK_CONTAINER(scrolled_window), hexv);


	g_signal_connect(hexv, "cursor-moved", G_CALLBACK(mudou_posicao_cursor),
			main_window);
	g_signal_connect(hexv, "edition-mode-changed", G_CALLBACK(mudou_modo_edicao),
			main_window);
	g_signal_connect(hexv, "selection-changed", G_CALLBACK(mudou_selecao),
				main_window);
	g_signal_connect(hexv, "changed", G_CALLBACK(mudou_conteudo),
				fh); // dados é Filehandler! Para lá poder avisar que algo mudou //TODO: Mudar para dados do aplicativo?

	gtk_window_set_icon_from_file(GTK_WINDOW(main_window), "arte/icone.png", NULL);

	gtk_widget_show_all(main_window);

	gtk_widget_grab_focus(hexv);

	return TRUE;
}

void limpa_barra_de_estado(const char* contexto)
{
	guint _contexto = gtk_statusbar_get_context_id(GTK_STATUSBAR(statusbar),
				contexto);
	gtk_statusbar_pop(GTK_STATUSBAR(statusbar), _contexto);
}

void pipoca_na_barra_de_estado(const char* contexto, const gchar* msg)
{
	guint _contexto = gtk_statusbar_get_context_id(GTK_STATUSBAR(statusbar),
				contexto);
	gtk_statusbar_pop(GTK_STATUSBAR(statusbar), _contexto);
	gtk_statusbar_push(GTK_STATUSBAR(statusbar), _contexto, msg);
}

void elabora_menu_recentes()
{
	if (preferencias.qtd_arquivos_recentes)
	{
		int qtd = preferencias.qtd_arquivos_recentes;
		if (qtd > 0 && preferencias.arquivos_recentes != NULL)
		{
			int n;

			GList *item_lista = g_list_first(preferencias.arquivos_recentes);
			GtkWidget *menu_recentes = gtk_menu_new();

			for (n = 0; n < qtd && item_lista != NULL; n++, item_lista = g_list_next(item_lista))
			{
				gchar *rotulo;
				if (n > 10)
					rotulo = g_strdup_printf(" %s", (gchar*)item_lista->data);
				else
					rotulo = g_strdup_printf("_%i: %s", n, (gchar*)item_lista->data);
				GtkWidget *item_menu = gtk_menu_item_new_with_mnemonic(rotulo);
				g_free(rotulo);
				// FIXME: arriscado : item_lista->data em vez de cópia? Mas como descartar depois?
				g_signal_connect(item_menu, "activate", G_CALLBACK(on_abrir_arquivo_definido_activate), item_lista->data);
				gtk_menu_shell_append(GTK_MENU_SHELL(menu_recentes), item_menu);
			}

			GtkWidget* menu_anterior = gtk_menu_item_get_submenu(GTK_MENU_ITEM(menu_item_abrir_recente));
			if (menu_anterior != NULL)
				gtk_widget_destroy(menu_anterior);
			gtk_menu_item_set_submenu(GTK_MENU_ITEM(menu_item_abrir_recente), menu_recentes);
			gtk_widget_set_sensitive(menu_item_abrir_recente, TRUE);
			gtk_widget_show_all (menu_item_abrir_recente);
		}
		else
		{
			gtk_widget_set_sensitive(menu_item_abrir_recente, FALSE);
			gtk_widget_hide (menu_item_abrir_recente);
		}
	}
	else
	{
		gtk_widget_set_sensitive(menu_item_abrir_recente, FALSE);
		gtk_widget_hide (menu_item_abrir_recente);
	}
}

static gboolean drag_drop_handler(GtkWidget      *widget,
        GdkDragContext *drag_context,
        gint            x,
        gint            y,
        guint           time,
        gpointer        user_data)
{
	GdkAtom         target_type;

	target_type = gtk_drag_dest_find_target(widget, drag_context, gtk_drag_dest_get_target_list(widget));
	if (target_type != GDK_NONE)
	{
		gtk_drag_get_data
						(
								widget,         /* will receive 'drag-data-received' signal */
								drag_context,        /* represents the current state of the DnD */
								target_type,    /* the target type we want */
								time            /* time stamp */
						);

		return TRUE;
	}
	return FALSE;
}

static void drag_data_received_handler
(GtkWidget *widget, GdkDragContext *context, gint x, gint y,
        GtkSelectionData *selection_data, guint target_type, guint time,
        gpointer data)
{
	Filehandler *fh = data; // TODO: Mudar para pegar dos dados do aplicativo

	gchar *nome = gdk_atom_name(gtk_selection_data_get_target(selection_data));

	if (g_strcmp0(nome, "text/uri-list") == 0)
	{
		gchar **uris = gtk_selection_data_get_uris (selection_data);
		if (uris)
		{
			if (uris[0] != NULL)
			{
				gchar *arquivo = g_filename_from_uri(uris[0], NULL, NULL);
				if (arquivo != NULL)
				{
					filehandler_open_file(fh, arquivo);
					g_free(arquivo);
				}
			}
			g_strfreev(uris);
		}
	}

	// TODO: Aceitar texto para ser inserido!

	g_free(nome);

	gtk_drag_finish(context, TRUE, FALSE, time);
}

// TODO: Substituir aqui (*e nos callbacks*) fh pelos dados do aplicativo
void configura_arrastar_e_soltar(Filehandler *fh)
{
	enum {
		TARGET_STRING,
	};

	/* datatype (string), restrictions on DnD (GtkTargetFlags), datatype (int) */
	static GtkTargetEntry target_list[] = {
			{ "text/uri-list", 0, TARGET_STRING },
			{ "STRING",     0, TARGET_STRING },
			{ "text/plain", 0, TARGET_STRING },
	};

	static guint n_targets = G_N_ELEMENTS (target_list);
	gtk_drag_dest_set(hexv,GTK_DEST_DEFAULT_ALL,NULL,0,GDK_ACTION_COPY);
	gtk_drag_dest_set
		(
			hexv,              /* widget that will accept a drop */
			GTK_DEST_DEFAULT_MOTION /* default actions for dest on DnD */
			| GTK_DEST_DEFAULT_HIGHLIGHT,
			target_list,            /* lists of target to support */
			n_targets,              /* size of list */
			GDK_ACTION_COPY         /* what to do with data after dropped */
		);

	g_signal_connect(hexv, "drag-data-received", G_CALLBACK(drag_data_received_handler),
					fh);
	g_signal_connect(hexv, "drag-drop", G_CALLBACK(drag_drop_handler),
					fh);
}

int main(int argc, char *argv[])
{
	int qtdErros = 0;
	ultimo_diretorio = NULL;
	nomeArquivoAtual = NULL;

	gtk_init(&argc, &argv);

	g_set_prgname("hexchange");

	g_set_application_name ("heXchange");

	{
		FilehandlerCallbacks cb;

		cb.new = novo_arquivo;
		cb.open = abre_arquivo;
		cb.save = salvar_arquivo;
		cb.save_as = salvar_arquivo_como;
		cb.close = fechar_arquivo;
		cb.include_in_recents = acrescenta_aos_recentes;

		fh = filehandler_new(&cb, NULL, NULL); // TODO: user_data deveria estar aqui com os dados do aplicativo
		if (fh == NULL)
		{
			showErrorMessage(NULL, "Não foi possível configurar para manipular os arquivos.");
			exit(EXIT_FAILURE);
		}
	}

	if (!mostra_janela(fh))
	{
		showErrorMessage(NULL, "Não foi possível carregar a interface.");
		exit(EXIT_FAILURE);
	}

	inicia_preferencias_padrao(&preferencias);
	if ( ! carrega_arquivo_preferencias(XCHANGE_HEX_VIEW(hexv), &preferencias) )
		fprintf(stderr, "Problema nas preferências\n");


	qtd_tabelas = MAX_TABELAS;
	xt_tabela = calloc(qtd_tabelas, sizeof(XChangeTable*));
	if (xt_tabela == NULL)
	{
		perror("Alocando espaço para tabelas");
		destroi_preferencias(&preferencias);
		exit(EXIT_FAILURE);
	}
	int n;
	for (n = 0; n < qtd_tabelas; n++)
		xt_tabela[n] = NULL;
	altera_padrao_codificacao_caracteres(preferencias.codificacao_texto, NULL);
	id_tabela_atual = 0;


	file_changes_saved = TRUE;
	changes_until_last_save = 0;

	gchar *autores[2] = {NULL, NULL};
	const char * autor = ">moc.liamg@groflodor< GRoflodoR";
	int t;
	t = strlen(autor);
	char *verdadeiro = malloc(t+1);
	for (n = 0; n < t; n++)
		verdadeiro[n] = autor[t-n-1];
	verdadeiro[t] = 0;
	autores[0] = verdadeiro;
	gtk_about_dialog_set_authors (GTK_ABOUT_DIALOG(dialog_sobre), (const gchar**)autores);
	free(verdadeiro);

	elabora_menu_recentes();

	// Preparando dados de localização
	inicia_dados_localizar(&dados_localizar);

	if (argc > 1)
	{
		// Abre arquivo listado na linha de comando
		filehandler_open_file(fh, argv[1]);
	}
	else
		filehandler_on_action_new_activate(NULL, fh);

	// Arrastar e soltar
	configura_arrastar_e_soltar(fh); // TODO: trocar para passar os dados do aplicativo

	gtk_main();

	salva_preferencias_se_necessario(); // FIXME argumentos

	// Liberando preferências
	destroi_preferencias(&preferencias);

	g_free(ultimo_diretorio);

	// Destrói dados de localização
	destroi_dados_localizar(&dados_localizar);

	for (n = 0; n < qtd_tabelas; n++)
		xchange_table_close(xt_tabela[n]);

	filehandler_destroy(fh);

	exit(qtdErros);
}

G_MODULE_EXPORT
void on_action_undo_activate(GtkAction *action, gpointer user_data)
{
	xchange_hex_view_undo(XCHANGE_HEX_VIEW(hexv));
	gtk_widget_queue_draw(hexv);
}

G_MODULE_EXPORT
void on_action_redo_activate(GtkAction *action, gpointer user_data)
{
	xchange_hex_view_redo(XCHANGE_HEX_VIEW(hexv));
	gtk_widget_queue_draw(hexv);
}

static void fechar_arquivo()
{
	limpa_barra_de_estado("Arquivo");
	xchange_hex_view_close_file(XCHANGE_HEX_VIEW(hexv));

	gtk_window_set_title(GTK_WINDOW(main_window), g_get_application_name());
	file_changes_saved = TRUE;
}

G_MODULE_EXPORT
void on_toggleaction_show_offset_panel_toggled(GtkToggleAction *toggleaction,
		gpointer data)
{
	xchange_hex_view_set_offset_panel_visibility(XCHANGE_HEX_VIEW(hexv),
			gtk_toggle_action_get_active(toggleaction));
}

G_MODULE_EXPORT
void on_toggleaction_show_text_toggled(GtkToggleAction *toggleaction,
		gpointer data)
{
	xchange_hex_view_set_text_panel_visibility(XCHANGE_HEX_VIEW(hexv),
			gtk_toggle_action_get_active(toggleaction));
}

G_MODULE_EXPORT
void on_action_novo_arquivo_activate(GtkAction *action, gpointer data)
{
	filehandler_on_action_new_activate(action, data);
}

static void novo_arquivo(gpointer data)
{
	limpa_barra_de_estado("Arquivo");
	XChangeFile * xf = xchange_open(NULL, NULL);
	xchange_hex_view_load_file(XCHANGE_HEX_VIEW(hexv), xf);
	file_changes_saved = TRUE;
	changes_until_last_save = 0;
}

G_MODULE_EXPORT
void on_action_abrir_activate(GtkAction *action, gpointer data)
{
	filehandler_on_action_open_activate(action, data);
}

G_MODULE_EXPORT
void on_abrir_arquivo_definido_activate(GtkWidget *widget, gpointer data)
{
	// FIXME: dar um jeito sem usar variável global!
	filehandler_open_file(fh, data);
}

static void acrescenta_aos_recentes(const gchar*nome_arquivo, gpointer data)
{
	GList * item_recente = g_list_find_custom(preferencias.arquivos_recentes, nome_arquivo, g_strcmp0);
	if (item_recente != NULL)
	{
		g_free(item_recente->data);
		preferencias.arquivos_recentes = g_list_delete_link(preferencias.arquivos_recentes, item_recente);
	}
	preferencias.arquivos_recentes = g_list_prepend(preferencias.arquivos_recentes, g_strdup(nome_arquivo));
	elabora_menu_recentes();
}

static void atualiza_titulo_janela(const gchar *nome_arquivo, const gchar *diretorio)
{
	const gchar *nome_app = g_get_application_name();
	gchar *nome = g_path_get_basename(nome_arquivo);
	gchar *new_title = g_strdup_printf("%s - %s (%s)", nome_app, nome, diretorio);
	g_free(nome);
	gtk_window_set_title(GTK_WINDOW(main_window), new_title);
	g_free(new_title);
}

static gboolean abre_arquivo(const char *nome_arquivo, gpointer data)
{
	limpa_barra_de_estado("Arquivo");
	gboolean editavel = TRUE;
	char *filename_decoded = g_locale_from_utf8(nome_arquivo, -1, NULL, NULL, NULL);
	XChangeFile *xf = xchange_open(filename_decoded, "rb+");

	if (xf == NULL)
	{
		xf = xchange_open(filename_decoded, "rb");
		if (xf == NULL)
		{
			gchar *msg = g_strdup_printf("Não conseguiu carregar o arquivo %s .", filename_decoded);
			showErrorMessage(GTK_WINDOW(main_window), msg);
			g_free(msg);
			g_free(filename_decoded);
			return FALSE;
		}
		editavel = FALSE;
	}
	g_free(filename_decoded);

	if (!xchange_hex_view_load_file(XCHANGE_HEX_VIEW(hexv), xf))
	{
		xchange_close(xf);
		showErrorMessage(GTK_WINDOW(main_window), "Não conseguiu exibir o arquivo!");
		return FALSE;
	}

	xchange_hex_view_set_editable(XCHANGE_HEX_VIEW(hexv), editavel);
	if (!editavel)
		showWarningMessage(GTK_WINDOW(main_window), "O arquivo só pôde ser aberto para leitura.");

	file_changes_saved = TRUE;
	changes_until_last_save = 0;
	gtk_label_set_text(GTK_LABEL(label_arquivo_modificado), " ");

	atualiza_titulo_janela(nome_arquivo, ultimo_diretorio);

	if (editavel)
		pipoca_na_barra_de_estado("Arquivo", "Arquivo aberto.");
	else
		pipoca_na_barra_de_estado("Arquivo", "Arquivo aberto somente para leitura.");

	return TRUE;
}

//#ifndef __WIN32__
G_MODULE_EXPORT void on_action_salvar_como_activate(GtkAction *action,
		gpointer data)
{
	filehandler_on_action_save_as_activate(action, data);
}

static gboolean salvar_arquivo_como(const gchar *nome_arquivo, gpointer data)
{
	limpa_barra_de_estado("Arquivo");

	char *filename_decoded = g_locale_from_utf8(nome_arquivo, -1, NULL, NULL, NULL);
	gboolean salvou = xchange_hex_view_save_file(XCHANGE_HEX_VIEW(hexv), filename_decoded);
	g_free(filename_decoded);

	if (!salvou)
	{
		showErrorMessage(GTK_WINDOW(main_window), "Não conseguiu salvar o arquivo!");
		return FALSE;
	}

	file_changes_saved = TRUE;
	changes_until_last_save = xchange_get_undo_list_size(xchange_hex_view_get_file(XCHANGE_HEX_VIEW(hexv)));
	gtk_label_set_text(GTK_LABEL(label_arquivo_modificado), " ");

	atualiza_titulo_janela(nome_arquivo, ultimo_diretorio);

	pipoca_na_barra_de_estado("Arquivo", "Arquivo salvo.");

	return TRUE;
}


G_MODULE_EXPORT void on_action_salvar_activate(GtkAction *action, gpointer data)
{
	filehandler_on_action_save_activate(action, data);
}

static gboolean salvar_arquivo(gpointer data)
{
	limpa_barra_de_estado("Arquivo");

	if (!xchange_save(xchange_hex_view_get_file(XCHANGE_HEX_VIEW(hexv))))
	{
		perror("Salvando");
		showErrorMessage(GTK_WINDOW(main_window), "Erro ao salvar.\nExperimente \"salvar como\" ou \"como cópia\"\n");
		return FALSE;
	}

	pipoca_na_barra_de_estado("Arquivo", "Arquivo salvo.");
	file_changes_saved = TRUE;
	changes_until_last_save = xchange_get_undo_list_size(xchange_hex_view_get_file(XCHANGE_HEX_VIEW(hexv)));
	gtk_label_set_text(GTK_LABEL(label_arquivo_modificado), " ");

	return TRUE;
}

G_MODULE_EXPORT void on_action_salvar_copia_activate(GtkAction *action,
		gpointer data)
{
	limpa_barra_de_estado("Arquivo");
	GtkWidget * dialog = gtk_file_chooser_dialog_new("Salvar cópia...",
			GTK_WINDOW(main_window), GTK_FILE_CHOOSER_ACTION_SAVE,
			GTK_STOCK_SAVE, GTK_RESPONSE_OK, GTK_STOCK_CANCEL,
			GTK_RESPONSE_CANCEL, NULL);

	if (ultimo_diretorio != NULL)
		gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog),
				ultimo_diretorio);

	gint result = gtk_dialog_run(GTK_DIALOG(dialog));

	if (result == GTK_RESPONSE_OK)
	{
		char *filename;
		filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));

		char *filename_decoded = g_locale_from_utf8(filename, -1, NULL, NULL, NULL);
		xchange_save_copy_as(xchange_hex_view_get_file(XCHANGE_HEX_VIEW(hexv)),
				filename_decoded);
		g_free(filename_decoded);

		g_free(filename);
		pipoca_na_barra_de_estado("Arquivo", "Cópia salva.");
	}
	gtk_widget_destroy(dialog);
}
//#endif

G_MODULE_EXPORT gboolean on_main_window_delete_event(GtkWidget *widget,
		GdkEvent *event, gpointer data)
{
	return filehandler_on_main_window_delete_event(widget, event, data);
}

G_MODULE_EXPORT void on_action_sair_activate(GtkAction *action, gpointer data)
{
	filehandler_on_action_quit_activate(action, data);
}

G_MODULE_EXPORT
void on_main_window_destroy(GtkObject *object, gpointer data)
{
	on_action_sair_activate(NULL, data);
}

G_MODULE_EXPORT
gboolean on_main_window_configure_event(GtkWidget *widget, GdkEventConfigure *event, gpointer data)
{
	gtk_window_get_position(GTK_WINDOW(widget), &preferencias.janela_x, &preferencias.janela_y);
	gtk_window_get_size(GTK_WINDOW(widget), &preferencias.janela_w, &preferencias.janela_h);
	return FALSE;
}

/* Precisa destruir a string retornada */
static gchar *obtem_escolha_codificacao(GtkComboBox *combobox)
{
	gint encoding_id;
	gchar *encoding = NULL;
	encoding_id = gtk_combo_box_get_active(combobox);
	if (encoding_id == 0)
		encoding = NULL;
	else if (encoding_id == 1)
	{
		const char *tmp_encoding;
		g_get_charset(&tmp_encoding);
		if (tmp_encoding != NULL)
			encoding = g_strdup(tmp_encoding);
	}
	else
	{
		gchar *tmp_encoding = NULL;

#if GTK_CHECK_VERSION(2,24,0)
		if (gtk_combo_box_get_has_entry(combobox))
		{
			const gchar *tmp_encoding2 = gtk_entry_get_text (GTK_ENTRY (gtk_bin_get_child (GTK_BIN (combobox))));
			if (tmp_encoding2 != NULL)
			{
				tmp_encoding = g_strdup(tmp_encoding2);
			}
		}
		else if (GTK_IS_COMBO_BOX_TEXT(combobox))
			tmp_encoding = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(combobox));
#if !GTK_CHECK_VERSION(2,25,0)
		else
			tmp_encoding = gtk_combo_box_get_active_text(combobox);
#endif
#else
		tmp_encoding = gtk_combo_box_get_active_text(combobox);
#endif
		encoding = tmp_encoding;
	}
	return encoding;
}

static gchar *usuario_escolhe_arquivo_tabela(gchar **encoding)
{
	gchar *filename = NULL;
	GtkWidget * dialog = filechooserdialog_carregar_texto;

	if (ultimo_diretorio != NULL)
		gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog),
				ultimo_diretorio);

	GtkFileFilter *filter[3];
	filter[0] = gtk_file_filter_new ();
	gtk_file_filter_add_pattern (filter[0], "*.tbl");
	gtk_file_filter_set_name(filter[0], "Tabelas (*.tbl)");
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter[0]);

	filter[1] = gtk_file_filter_new ();
	gtk_file_filter_add_mime_type(filter[1], "text/plain");
	gtk_file_filter_set_name(filter[1], "Arquivos-texto");
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter[1]);

	filter[2] = gtk_file_filter_new ();
	gtk_file_filter_add_pattern (filter[2], "*");
	gtk_file_filter_set_name(filter[2], "Todos os arquivos");
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter[2]);


	gtk_window_set_title(GTK_WINDOW(dialog), "Carregar arquivo de tabela de caracteres");


	gint result = gtk_dialog_run(GTK_DIALOG(dialog));

	if (result == GTK_RESPONSE_OK)
	{
		gchar *dirname;

		filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));

		if (encoding != NULL)
			*encoding = obtem_escolha_codificacao(GTK_COMBO_BOX(comboboxentry_codificacao_caracteres_arquivo));

		dirname = g_path_get_dirname(filename);
		if (dirname != NULL)
		{
			g_free(ultimo_diretorio);
			ultimo_diretorio = dirname;
		}
		else
			pipoca_na_barra_de_estado("Arquivo", "Problema na identificação do diretório.");

	}
	gtk_widget_hide(dialog);

	gtk_file_chooser_remove_filter (GTK_FILE_CHOOSER(dialog), filter[0]);
	gtk_file_chooser_remove_filter (GTK_FILE_CHOOSER(dialog), filter[1]);
	gtk_file_chooser_remove_filter (GTK_FILE_CHOOSER(dialog), filter[2]);

	return filename;
}

static XChangeTable *carregar_tabela(const char *filename, const char *encoding)
{
	XChangeTable * xt;
	char *filename_decoded = g_locale_from_utf8(filename, -1, NULL, NULL, NULL);
	xt = xchange_table_open(filename_decoded, THINGY_TABLE, encoding);
	g_free(filename_decoded);

	if (xt == NULL)
	{
		gchar * msg = g_strdup_printf("Não conseguiu abrir o arquivo de codificação de caracteres!\n%s",filename);
		showErrorMessage(GTK_WINDOW(main_window), msg);
		g_free(msg);
		return NULL;
	}

	xchange_table_set_unknown_charUTF8(xt, preferencias.caractere_desconhecido, -1);
	xchange_table_set_lineend_markUTF8(xt, preferencias.caractere_novalinha, -1);
	xchange_table_set_paragraph_markUTF8(xt, preferencias.caractere_fimmensagem, -1);

	return xt;
}

static void set_current_table(int id)
{
	id_tabela_atual = id;
	xchange_hex_view_set_table(XCHANGE_HEX_VIEW(hexv), xt_tabela[id_tabela_atual]);
	gtk_radio_action_set_current_value(GTK_RADIO_ACTION(radioaction_visao_codificao_padronizada), id);
}

static void altera_padrao_codificacao_caracteres(char * encoding, gpointer data)
{
	XChangeTable * xt_new = xchange_table_create_from_encoding(encoding);
	if (xt_new != NULL)
	{
		pipoca_na_barra_de_estado("Codificação de caracteres", encoding);
		xchange_table_close(xt_tabela[0]);
		xt_tabela[0] = xt_new;
		on_radioaction_visao_codificao_padronizada_changed(NULL, GTK_RADIO_ACTION(radioaction_visao_codificao_padronizada), data);
		gchar *rotulo = g_strdup_printf("Visão %s", encoding);
		gtk_action_set_label(radioaction_visao_codificao_padronizada, rotulo);
		g_free(rotulo);
		gtk_action_set_short_label(radioaction_visao_codificao_padronizada, encoding);

		xchange_table_set_unknown_charUTF8(xt_new, preferencias.caractere_desconhecido, -1);
		xchange_table_set_lineend_markUTF8(xt_new, preferencias.caractere_novalinha, -1);
		xchange_table_set_paragraph_markUTF8(xt_new, preferencias.caractere_fimmensagem, -1);

		set_current_table(0);
	}
	else
	{
		if (encoding != NULL)
		{
			gchar *tmp = g_strdup_printf("Não foi possível usar esta codificação: %s", encoding);
			pipoca_na_barra_de_estado("Codificação de caracteres", tmp);
			g_free(tmp);
		}
		else
			pipoca_na_barra_de_estado("Codificação de caracteres", "Não é possível detectar automaticamente uma codificação para esse fim.");
	}
}

G_MODULE_EXPORT
void on_menuitem_codificacao_padrao_activate(GtkMenuItem *menuitem, gpointer data)
{
	GValue valor = {0};
	gchar *encoding;

	encoding = g_strdup(gtk_menu_item_get_label(menuitem));

	g_value_init(&valor, G_TYPE_BOOLEAN);
	g_object_get_property(G_OBJECT(menuitem), "use-underline", &valor);
	if (g_value_get_boolean(&valor))
	{
		// Remover sublinha
		gchar **itens = g_strsplit (encoding, "_", 20);
		if (itens != NULL)
		{
			gchar *new_encoding = g_strjoinv (NULL, itens);
			g_strfreev(itens);
			if (new_encoding != NULL)
			{
				g_free(encoding);
				encoding = new_encoding;
			}
		}
	}
	g_value_unset(&valor);
	altera_padrao_codificacao_caracteres(encoding, data);
	g_free(encoding);
}

G_MODULE_EXPORT
void on_comboboxentry_codificacao_caracteres_changed(GtkComboBox *widget, gpointer data)
{
	char *encoding = obtem_escolha_codificacao(widget);
	g_print("%s\n", encoding);
	altera_padrao_codificacao_caracteres(encoding, data);
	g_free(encoding);
}

G_MODULE_EXPORT
void on_action_escolher_codificacao_activate(GtkAction *action,
		gpointer data)
{
	limpa_barra_de_estado("Codificação de caracteres");
	gint resposta = gtk_dialog_run(GTK_DIALOG(dialog_codificacoes));
	if (resposta == GTK_RESPONSE_OK)
	{
		char *encoding = obtem_escolha_codificacao(GTK_COMBO_BOX(comboboxentry_codificacao_caracteres));
		altera_padrao_codificacao_caracteres(encoding, data);
		g_free(encoding);
	}
	gtk_widget_hide(dialog_codificacoes);
}

static void alterna_tabela_atual()
{
	int id_nova_tabela = id_tabela_atual;
	do
	{
		id_nova_tabela = (id_nova_tabela+1)%qtd_tabelas;
	} while (xt_tabela[id_nova_tabela] == NULL && id_nova_tabela != 0);

	if (id_tabela_atual != id_nova_tabela)
		set_current_table(id_nova_tabela);
}

G_MODULE_EXPORT
void on_action_alternar_codificacao_activate(GtkAction *action, gpointer data)
{
	alterna_tabela_atual();
}

G_MODULE_EXPORT
gboolean on_main_window_key_press_event(GtkWidget *widget, GdkEventKey *event, gpointer user)
{
	if (event->keyval == GDK_KEY_Tab && event->state == 0)
	{
		alterna_tabela_atual();
		return TRUE;
	}
	return FALSE;
}

G_MODULE_EXPORT
void on_action_carregar_tabela_activate(GtkAction *action, gpointer data)
{
	gchar *filename, *encoding;
	filename = usuario_escolhe_arquivo_tabela(&encoding);
	if (filename == NULL)
		return;

	XChangeTable *xt = carregar_tabela(filename, encoding);
	if (xt != NULL)
	{
		XChangeTable *xt_anterior = xt_tabela[1];
		xt_tabela[1] = xt;
		set_current_table(1);
		if (xt_anterior != NULL)
			xchange_table_close(xt_anterior);
		gchar *nome_base_arquivo = g_path_get_basename(filename);
		gchar *rotulo = NULL;
		if (nome_base_arquivo == NULL)
			rotulo = g_strdup("Visão tabela 1");
		else
			rotulo = g_strdup_printf("Visão tabela 1 (%s)", nome_base_arquivo);
		gtk_action_set_label(radioaction_visao_tabela1, rotulo);
		g_free(nome_base_arquivo);
		g_free(rotulo);
	}
	g_free(filename);
	g_free(encoding);
}

G_MODULE_EXPORT
void on_action_carregar_tabela2_activate(GtkAction *action, gpointer data)
{
	gchar *filename, *encoding;
	filename = usuario_escolhe_arquivo_tabela(&encoding);
	if (filename == NULL)
		return;

	XChangeTable *xt = carregar_tabela(filename, encoding);
	if (xt != NULL)
	{
		XChangeTable *xt_anterior = xt_tabela[2];
		xt_tabela[2] = xt;
		set_current_table(2);
		if (xt_anterior != NULL)
			xchange_table_close(xt_anterior);
		gchar *nome_base_arquivo = g_path_get_basename(filename);
		gchar *rotulo = NULL;
		if (nome_base_arquivo == NULL)
			rotulo = g_strdup("Visão tabela 2");
		else
			rotulo = g_strdup_printf("Visão tabela 2 (%s)", nome_base_arquivo);
		gtk_action_set_label(radioaction_visao_tabela2, rotulo);
		g_free(nome_base_arquivo);
		g_free(rotulo);
	}
	g_free(filename);
	g_free(encoding);
}

G_MODULE_EXPORT
void on_radioaction_visao_codificao_padronizada_changed(GtkRadioAction *action, GtkRadioAction *current, gpointer data)
{
	gint id_tabela = gtk_radio_action_get_current_value(current);

	if (id_tabela != id_tabela_atual)
	{
		xchange_hex_view_set_table(XCHANGE_HEX_VIEW(hexv), xt_tabela[id_tabela]);
		id_tabela_atual = id_tabela;
	}
}

static off_t converte_texto_em_posicao(const char *texto)
{
	if (texto == NULL)
		return (off_t)-1;
	gint comprimento = strlen(texto);
	if (comprimento <= 0)
		return (off_t)-1;

	off_t valor;
	int conversoes;
	if (texto[0] == 'x' || texto[0] == 'X' || texto[0] == '$')
		conversoes = sscanf(texto + 1, "%lx", &valor);
	else if (texto[0] == '0' && comprimento > 1 && (texto[1] == 'x' || texto[1] == 'X'))
		conversoes = sscanf(texto + 2, "%lx", &valor);
	else
		conversoes = sscanf(texto, "%lu", &valor);
	if (conversoes == 1)
		return valor;
	else
		return (off_t)-2;

}

static void tenta_ir_para(const char *texto)
{
	off_t valor;
	valor =	converte_texto_em_posicao(texto);

	if (valor == (off_t) -1)
		pipoca_na_barra_de_estado("Localizar", "Não há endereço para onde ir.");
	else if (valor == (off_t) -2)
		pipoca_na_barra_de_estado("Localizar", "Não conseguiu entender o endereço.");
	else
	{
		size_t filesize = xchange_hex_view_get_file_size(XCHANGE_HEX_VIEW(hexv));
		if (filesize == -1)
			return;
		if (filesize == 0 && valor == 0)
			return;
		if (valor >= filesize)
		{
			off_t cursor = xchange_hex_view_get_cursor(XCHANGE_HEX_VIEW(hexv));
			if (filesize && cursor < filesize -1)
			{
				if (showYesNoDialog(GTK_WINDOW(main_window), "O destino está além do fim do arquivo.\nDeseja ir ao fim do arquivo?") != GTK_RESPONSE_YES)
					return;
			}
			else
			{
				showErrorMessage(GTK_WINDOW(main_window), "O destino está além do fim do arquivo.");
				return;
			}
		}
		xchange_hex_view_goto(XCHANGE_HEX_VIEW(hexv), valor);
	}
}

G_MODULE_EXPORT
void on_action_ir_para_activate(GtkAction *action, gpointer data)
{
	gint result = gtk_dialog_run(GTK_DIALOG(dialog_ir_para));

	if (result == GTK_RESPONSE_OK)
	{
		const gchar * texto =
				gtk_entry_get_text(GTK_ENTRY(entry_valor_ir_para));
		tenta_ir_para(texto);
	}
	gtk_widget_hide(dialog_ir_para);
}

G_MODULE_EXPORT
void on_action_copiar_activate(GtkAction *action, gpointer data)
{
	GtkClipboard * clipboard_normal =
			gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
	//	GtkClipboard * clipboard_selecao = gtk_clipboard_get(GDK_SELECTION_PRIMARY);

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
	for (n = 0; n < tamanho; n++)
	{
		sprintf(&texto[n * 3], "%02hhX ", bytes[n]);
	}
	g_free(bytes);
	gtk_clipboard_set_text(clipboard_normal, texto, -1);
	//	gtk_clipboard_set_text(clipboard_selecao, texto, -1);
	g_free(texto);
}

G_MODULE_EXPORT
void on_action_copiar_texto_activate(GtkAction *action, gpointer data)
{
	GtkClipboard * clipboard_normal =
			gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
	//	GtkClipboard * clipboard_selecao = gtk_clipboard_get(GDK_SELECTION_PRIMARY);

	size_t tamanho;
	gchar *texto = xchange_hex_view_get_selected_text(XCHANGE_HEX_VIEW(hexv),
			&tamanho);
	if (texto == NULL || tamanho == 0)
	{
		g_free(texto);
		return;
	}

	gtk_clipboard_set_text(clipboard_normal, texto, -1);
	//	gtk_clipboard_set_text(clipboard_selecao, texto, -1);
	g_free(texto);
}

G_MODULE_EXPORT
void on_action_colar_activate(GtkAction *action, gpointer data)
{
	if (!xchange_hex_view_is_editable(XCHANGE_HEX_VIEW(hexv)))
		return;

	GtkClipboard * clipboard_normal =
			gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);

	gchar *texto = gtk_clipboard_wait_for_text(clipboard_normal);
	if (texto == NULL)
		return;

	// Verifica se é um texto UTF-8, sendo byte ou não...
	if (g_utf8_strlen(texto, -1) == 0)
	{
		g_free(texto);
		return;
	}

	int tamanho_bytes;
	uint8_t * bytes = recupera_bytes_de_texto_hexa(texto, &tamanho_bytes, NULL);
	if (bytes == NULL)
	{
		// Não é uma sequência de bytes. Tenta converter pela codificação.
		bytes = converte_pela_codificacao(texto, &tamanho_bytes, NULL);
	}

	if (bytes != NULL)
	{
		off_t offset_inicio;
		off_t offset_cursor = xchange_hex_view_get_cursor(XCHANGE_HEX_VIEW(hexv));
		if (offset_cursor == (off_t)-1)
			offset_cursor = 0;
		off_t inicio_selecao, fim_selecao;
		gboolean selecionado = xchange_hex_view_get_selection_bounds(XCHANGE_HEX_VIEW(hexv), &inicio_selecao, &fim_selecao);
		offset_inicio = selecionado ? inicio_selecao : offset_cursor;
		size_t tamanho_selecao = fim_selecao - inicio_selecao + 1;

		if(xchange_hex_view_is_insertion_mode(XCHANGE_HEX_VIEW(hexv)))
		{
			// Modo inserção
			if (!xchange_hex_view_insert_bytes(XCHANGE_HEX_VIEW(hexv), bytes, offset_inicio, tamanho_bytes))
			{
				free(bytes);
				g_free(texto);
				return;
			}
			if (selecionado)
				xchange_hex_view_delete_bytes(XCHANGE_HEX_VIEW(hexv), inicio_selecao + tamanho_bytes, tamanho_selecao);
		}
		else
		{
			// Modo sobrescrita
			if (!selecionado)
			{
				if (!xchange_hex_view_replace_bytes(XCHANGE_HEX_VIEW(hexv), bytes, offset_inicio, tamanho_bytes))
				{
					if (!xchange_hex_view_insert_bytes(XCHANGE_HEX_VIEW(hexv), bytes, offset_inicio, tamanho_bytes))
					{
						free(bytes);
						g_free(texto);
						return;
					}
				}
			}
			else
			{
				if (tamanho_selecao < tamanho_bytes)
				{
					gint resposta = showYesNoCancelDialog(GTK_WINDOW(main_window), "A seleção é menor que a quantidade de bytes a substituir.\nDeseja acrescentar os bytes em excesso?");
					if (resposta == GTK_RESPONSE_NO)
					{
						if (!xchange_hex_view_replace_bytes(XCHANGE_HEX_VIEW(hexv), bytes, offset_inicio, tamanho_selecao))
						{
							free(bytes);
							g_free(texto);
							return;
						}
					}
					else if (resposta == GTK_RESPONSE_YES)
					{
						if (!xchange_hex_view_replace_bytes(XCHANGE_HEX_VIEW(hexv), bytes, offset_cursor, tamanho_selecao))
						{
							free(bytes);
							g_free(texto);
							return;
						}
						xchange_hex_view_insert_bytes(XCHANGE_HEX_VIEW(hexv), bytes + tamanho_selecao, offset_inicio + tamanho_selecao, tamanho_bytes - tamanho_selecao);
					}
				}
				else
				{
					if (!xchange_hex_view_replace_bytes(XCHANGE_HEX_VIEW(hexv), bytes, offset_inicio, tamanho_selecao))
					{
						free(bytes);
						g_free(texto);
						return;
					}
				}
			}
		}
	}
	g_free(bytes);
	g_free(texto);
}

G_MODULE_EXPORT
void on_action_recortar_activate(GtkAction *action, gpointer data)
{
	if (!xchange_hex_view_get_has_selection(XCHANGE_HEX_VIEW(hexv)))
		return;
	on_action_copiar_activate(action, data);
	xchange_hex_view_delete_selection(XCHANGE_HEX_VIEW(hexv));
}

G_MODULE_EXPORT
void on_action_sobre_activate(GtkAction *action, gpointer data)
{
	gtk_widget_set_parent_window (dialog_sobre, gtk_widget_get_window(main_window));
	gtk_dialog_run(GTK_DIALOG(dialog_sobre));
	gtk_widget_hide(dialog_sobre);
}

G_MODULE_EXPORT
void on_action_abrir_janela_localizar_activate(GtkAction *action, gpointer data)
{
	gtk_widget_hide(vbox_substituir);
	gtk_action_set_visible(action_substituir, FALSE);
	gtk_action_set_visible(action_substituir_todos, FALSE);
	gtk_action_set_visible(action_localizar, TRUE);

	gtk_dialog_run(GTK_DIALOG(dialog_localizar));
	//gtk_widget_hide(dialog_localizar);
}

static struct DadosLocalizar * obtem_parametros_localizar()
{
	off_t pos_inicial = 0;
	off_t pos_final = 0;
	uint8_t *bytes_chave = NULL;
	int tamanho_bytes = 0;
	gboolean localizar_proximo = !gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(checkbutton_localizar_para_tras));

	struct DadosLocalizar *parametrosLocalizar = malloc(sizeof(struct DadosLocalizar));
	if (parametrosLocalizar == NULL)
		return NULL;

	gboolean pesquisar_do_cursor = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(
			radiobutton_localizar_do_cursor));
	if (pesquisar_do_cursor)
		pos_inicial = xchange_hex_view_get_cursor(XCHANGE_HEX_VIEW(hexv));

	gboolean pesquisar_intervalo = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(
			radiobutton_localizar_no_intervalo));
	if (pesquisar_intervalo)
	{
		gchar *texto;

		texto = gtk_editable_get_chars(GTK_EDITABLE(spinbutton_inicio_localizar), 0, -1);
		pos_inicial = converte_texto_em_posicao(texto);
		g_free(texto);

		texto = gtk_editable_get_chars(GTK_EDITABLE(spinbutton_fim_localizar), 0, -1);
		pos_final = converte_texto_em_posicao(texto);
		g_free(texto);

	}
	else if (!localizar_proximo)
	{
		pos_final ^= pos_inicial;
		pos_inicial ^= pos_final;
		pos_final ^= pos_inicial;
	}

	limpa_barra_de_estado("Localização");

	const gchar *texto = gtk_entry_get_text(GTK_ENTRY(
			entry_valor_a_localizar));
	if (texto == NULL)
	{
		pipoca_na_barra_de_estado("Localização", "Não conseguiu obter texto a localizar.");
		free(parametrosLocalizar);
		return NULL;
	}

	gint tamanho_texto = strlen(texto);
	if (tamanho_texto == 0)
	{
		pipoca_na_barra_de_estado("Localização", "Sem conteúdo para localizar.");
		free(parametrosLocalizar);
		return NULL;
	}

	gboolean localiza_bytes = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(
			radiobutton_localizar_bytes));

	if (!localiza_bytes)
	{
		// Localiza texto
		bytes_chave = converte_pela_codificacao(texto, &tamanho_bytes, "Localização");
	}
	else
	{
		// A string é uma sequência de bytes
		bytes_chave = recupera_bytes_de_texto_hexa(texto, &tamanho_bytes, "Localização");
	}
	if (bytes_chave == NULL)
	{
		pipoca_na_barra_de_estado("Localização", "Não conseguiu interpretar o texto a localizar.");
		free(parametrosLocalizar);
		return NULL;
	}
	free(bytes_chave);

	parametrosLocalizar->texto_a_localizar = g_strdup(texto);
	parametrosLocalizar->localizando_bytes = localiza_bytes;
	parametrosLocalizar->localizando_proximo = localizar_proximo;
	parametrosLocalizar->inicio = pos_inicial;
	parametrosLocalizar->fim = pos_final;

	return parametrosLocalizar;
}

// Diz se os tamanhos dos textos a localizar e para substituir são iguais
static gboolean confere_tamanhos_textos_localizar_substituir(gboolean localizando_bytes)
{
	uint8_t *bytes = NULL, *bytes_substituidores = NULL;
	int tamanho_bytes = 0, tamanho_substituidor = 0;

	const gchar *texto = gtk_entry_get_text(GTK_ENTRY(entry_valor_a_localizar));
	const gchar *texto_substituidor = gtk_entry_get_text(GTK_ENTRY(entry_valor_para_substituir));

	if (localizando_bytes)
	{
		bytes = recupera_bytes_de_texto_hexa(texto, &tamanho_bytes, NULL);
		bytes_substituidores = recupera_bytes_de_texto_hexa(texto_substituidor, &tamanho_substituidor, NULL);
	}
	else
	{
		bytes = converte_pela_codificacao(texto, &tamanho_bytes, NULL);
		bytes_substituidores = converte_pela_codificacao(texto_substituidor, &tamanho_substituidor, NULL);
	}

	free(bytes);
	free(bytes_substituidores);

	if (bytes != NULL && bytes_substituidores == NULL)
		return FALSE;
	if (bytes != NULL && bytes_substituidores != NULL)
		return tamanho_bytes == tamanho_substituidor;
	return TRUE;
}


G_MODULE_EXPORT
void on_action_abrir_janela_substituir_activate(GtkAction *action, gpointer data)
{
	gtk_widget_show(vbox_substituir);
	gtk_action_set_visible(action_localizar, FALSE);
	gtk_action_set_visible(action_substituir, TRUE);
//	gtk_widget_set_sensitive(button_substituir, FALSE);
	gtk_action_set_visible(action_substituir_todos, TRUE);

//	gtk_widget_hide();

	if (gtk_widget_get_visible(button_substituir))
	{
		struct DadosLocalizar *parametrosLocalizar = obtem_parametros_localizar();
		if (parametrosLocalizar == NULL || confere_tamanhos_textos_localizar_substituir(parametrosLocalizar->localizando_bytes))
			gtk_widget_hide(vbox_substituir_por_tamanho_diferente);
		else
			gtk_widget_show(vbox_substituir_por_tamanho_diferente);

		if (parametrosLocalizar != NULL)
		{
			destroi_dados_localizar(parametrosLocalizar);
			free(parametrosLocalizar);
		}
	}

	gtk_dialog_run(GTK_DIALOG(dialog_localizar));
	//gtk_widget_hide(dialog_localizar);
}

G_MODULE_EXPORT
void on_button_localizar_cancelar_clicked(GtkButton *button, gpointer data)
{
	gtk_widget_hide(dialog_localizar);
}

static void inicia_dados_localizar(struct DadosLocalizar *localizar)
{
	localizar->texto_a_localizar = NULL;
	localizar->localizando_bytes = FALSE;
	localizar->localizando_proximo = TRUE;
	localizar->inicio = 0;
	localizar->fim = 0;
}


static void destroi_dados_localizar(struct DadosLocalizar *localizar)
{
	g_free(localizar->texto_a_localizar);
	inicia_dados_localizar(localizar);
	// Liberar?
}


// Verifica se ambos são iguais
static gboolean compara_parametros_localizar(const struct DadosLocalizar *p1, const struct DadosLocalizar *p2)
{
	g_assert(p1 != NULL && p2 != NULL);
	return (g_strcmp0(p1->texto_a_localizar, p2->texto_a_localizar) == 0)
			&& (p1->localizando_bytes == p2->localizando_bytes)
			&& (p1->localizando_proximo == p2->localizando_proximo)
			&& (p1->inicio == p2->inicio)
			&& (p1->fim == p2->fim);
}

/*
 * Converte o texto em bytes conforme codificação ativa.
 */
static uint8_t *converte_pela_codificacao(const char *texto, int *tamanho_bytes, const char *contexto)
{
	int tamanho;
	uint8_t *bytes_chave;

	if (tamanho_bytes != NULL)
		*tamanho_bytes = -1;

	if (texto == NULL)
		return NULL;

	int tamanho_texto = strlen(texto);
	if (tamanho_texto == 0)
		return NULL;

	const XChangeTable *xt = xchange_hex_view_get_table(XCHANGE_HEX_VIEW(hexv));

	tamanho = xchange_table_scan_stringUTF8(xt, texto, tamanho_texto,
			NULL);
	if (tamanho <= 0)
	{
		gdk_window_beep(gtk_widget_get_window(main_window));
		pipoca_na_barra_de_estado(contexto, "Não conseguiu converter o texto em bytes através da tabela!");
		return NULL;
	}
	bytes_chave = malloc(tamanho);
	if (bytes_chave == NULL)
	{
		gdk_window_beep(gtk_widget_get_window(main_window));
		pipoca_na_barra_de_estado(contexto, "Problema na alocação de memória!");
		return NULL;
	}
	xchange_table_scan_stringUTF8(xt, texto, tamanho_texto, bytes_chave);

	if (tamanho_bytes != NULL)
		*tamanho_bytes = tamanho;

	return bytes_chave;
}

static uint8_t *recupera_bytes_de_texto_hexa(const gchar *texto, int *tamanho_bytes, const gchar *contexto)
{
	if (tamanho_bytes != NULL)
		*tamanho_bytes = -1;
	if (texto == NULL)
		return NULL;

	if (!g_utf8_validate(texto, -1, NULL))
		return NULL;

	int tamanho_texto = g_utf8_strlen(texto, -1);
	if (tamanho_texto == 0)
		return NULL;

	uint8_t *bytes_chave = g_malloc((tamanho_texto+1)/2);
	if (bytes_chave == NULL)
	{
		gdk_window_beep(gtk_widget_get_window(main_window));
		if (contexto != NULL)
			pipoca_na_barra_de_estado(contexto, "Problema na alocação de memória!");
		return NULL;
	}

	int pos = 0;
	gboolean leu_primeiro_nibble = FALSE;
	gboolean digito_desconhecido = FALSE;
	const gchar *caractere = NULL;
	for (caractere = texto; caractere != NULL && *caractere != 0; caractere = g_utf8_next_char(caractere))
	{
		gunichar carac = g_utf8_get_char(caractere);
		if (g_unichar_isspace(carac))
		{
			if (leu_primeiro_nibble)
				break;
			else
				continue;
		}
		else if (g_unichar_isxdigit(carac))
		{
			gint digito = g_unichar_xdigit_value(carac);
			if (!leu_primeiro_nibble)
			{
				bytes_chave[pos] = digito;
			}
			else
			{
				bytes_chave[pos] <<= 4;
				bytes_chave[pos] |= digito;
				pos++;
			}
			leu_primeiro_nibble = !leu_primeiro_nibble;
			continue;
		}

		digito_desconhecido = TRUE;
		break;
	}

	if (leu_primeiro_nibble || digito_desconhecido)
	{
		g_free(bytes_chave);
		gdk_window_beep(gtk_widget_get_window(main_window));
		if (contexto != NULL)
			pipoca_na_barra_de_estado(contexto, "Não conseguiu converter o texto em bytes!");
		return NULL;
	}

	if (tamanho_bytes != NULL)
		*tamanho_bytes = pos;

	return bytes_chave;
}

static gboolean localiza_outro(off_t from, const XChangeFile *xf, const struct DadosLocalizar *localizar, gboolean permitir_reiniciar_intervalo)
{
	uint8_t *bytes_chave = NULL;
	int tamanho_bytes = 0;

	// Obtém sequência de bytes a buscar
	if (!localizar->localizando_bytes)
	{
		// Localiza texto
		bytes_chave = converte_pela_codificacao(localizar->texto_a_localizar, &tamanho_bytes, "Localização");
	}
	else
	{
		// A string é uma sequência de bytes
		bytes_chave = recupera_bytes_de_texto_hexa(localizar->texto_a_localizar, &tamanho_bytes, "Localização");
	}
	if (bytes_chave == NULL)
		return FALSE;

	// Busca enfim
	off_t achou;// = funcao_localizar(from, 0, bytes_chave, tamanho_bytes, xf);
	if (localizar->localizando_proximo)
		achou = xchange_find(xf, from, localizar->fim, bytes_chave, tamanho_bytes);
	else
		achou = xchange_find_backwards(xf, localizar->inicio, from, bytes_chave, tamanho_bytes);
	// Não encontrou: tentar de novo?
	if (achou == (off_t) -1 && permitir_reiniciar_intervalo)
	{
		if (localizar->localizando_proximo)
		{
			// Se deslocou-se alguma vez e não está mais no começo
			if (localizar->inicio != from)
			{
				// Se o fim da busca é o fim do arquivo
				if (localizar->fim == 0 || localizar->fim >= xchange_get_size(xf)-1)
				{
					if (showYesNoDialog(GTK_WINDOW(main_window), "A busca alcançou o fim do arquivo e não localizou a sequência.\nDeseja buscar a partir do início do arquivo?") == GTK_RESPONSE_YES)
						achou = xchange_find(xf, 0, 0, bytes_chave, tamanho_bytes);
				}
				else
				{
					if (showYesNoDialog(GTK_WINDOW(main_window), "A busca alcançou o fim do intervalo e não localizou a sequência.\nDeseja buscar a partir do início do intervalo?") == GTK_RESPONSE_YES)
						achou = xchange_find(xf, localizar->inicio, localizar->fim, bytes_chave, tamanho_bytes);
				}
			}
		}
		else
		{
			// Localizando anterior
			// Se deslocou-se alguma vez e não está mais no fim
			if (from < localizar->fim || (localizar->fim == 0 && from < xchange_get_size(xf) -1))
			{
				if (localizar->inicio == 0 && (localizar->fim == 0 || localizar->fim == xchange_get_size(xf) -1))
				{
					if (showYesNoDialog(GTK_WINDOW(main_window), "A busca alcançou o começo do arquivo e não localizou a sequência.\nDeseja buscar a partir do fim do arquivo?") == GTK_RESPONSE_YES)
						achou = xchange_find_backwards(xf, 0, xchange_get_size(xf) - 1, bytes_chave, tamanho_bytes);
				}
				else
				{
					if (showYesNoDialog(GTK_WINDOW(main_window), "A busca alcançou o começo do intervalo e não localizou a sequência.\nDeseja buscar a partir do fim do intervalo?") == GTK_RESPONSE_YES)
						achou = xchange_find_backwards(xf, localizar->inicio, localizar->fim, bytes_chave, tamanho_bytes);
				}
			}
		}
	}
	free(bytes_chave);
	if (achou == (off_t) -1)
	{
		gdk_window_beep(gtk_widget_get_window(main_window));
		pipoca_na_barra_de_estado("Localização", "Não encontrado!");
		return FALSE;
	}

	// Encontrou: vai para onde foi encontrado
	xchange_hex_view_goto(XCHANGE_HEX_VIEW(hexv), achou);
	// Selecionar a busca
	xchange_hex_view_select(XCHANGE_HEX_VIEW(hexv), achou, tamanho_bytes);
	// TODO: Move it to xchange_hex_view_select()
	gtk_widget_queue_draw(hexv);

	return TRUE;
}

G_MODULE_EXPORT
void on_action_localizar_proximo_activate(GtkAction *action, gpointer data);

G_MODULE_EXPORT
void on_action_localizar_anterior_activate(GtkAction *action, gpointer data);

G_MODULE_EXPORT
void on_action_localizar_activate(GtkAction *action, gpointer data)
{
	struct DadosLocalizar *parametrosLocalizar;

	const XChangeFile *xf = xchange_hex_view_get_file(XCHANGE_HEX_VIEW(hexv));

	if (xf == NULL)
		return;

	parametrosLocalizar = obtem_parametros_localizar();
	if (parametrosLocalizar == NULL)
		return;

	// Compara para ver se é a pesquisa anterior
	//  Se for, localiza próximo/anterior. Se não for, recomeça.
	if (compara_parametros_localizar(&dados_localizar, parametrosLocalizar))
	{
		destroi_dados_localizar(parametrosLocalizar);
		g_free(parametrosLocalizar);
		if (dados_localizar.localizando_proximo)
			on_action_localizar_proximo_activate(NULL, data);
		else
			on_action_localizar_anterior_activate(NULL, data);
		return;
	}

	// Libera dados em pesquisa
	destroi_dados_localizar(&dados_localizar);
	dados_localizar = *parametrosLocalizar;

	//destroi_dados_localizar(parametrosLocalizar);
	g_free(parametrosLocalizar);

	localiza_outro(dados_localizar.localizando_proximo ? dados_localizar.inicio : dados_localizar.fim, xf, &dados_localizar, TRUE);

//	gtk_widget_hide(dialog_localizar);
}

G_MODULE_EXPORT
void on_action_localizar_proximo_activate(GtkAction *action, gpointer data)
{
	// Se nada foi procurado ainda, abre o diálogo
	if (dados_localizar.texto_a_localizar == NULL)
	{
		on_action_abrir_janela_localizar_activate(action, data);
		return;
	}

	dados_localizar.localizando_proximo = TRUE;

	const XChangeFile *xf = xchange_hex_view_get_file(XCHANGE_HEX_VIEW(hexv));
	off_t posicao_cursor = xchange_hex_view_get_cursor(XCHANGE_HEX_VIEW(hexv));

	localiza_outro(posicao_cursor + 1, xf, &dados_localizar, TRUE);
}

G_MODULE_EXPORT
void on_action_localizar_anterior_activate(GtkAction *action, gpointer data)
{
	// Se nada foi procurado ainda, abre o diálogo
	if (dados_localizar.texto_a_localizar == NULL)
	{
		on_action_abrir_janela_localizar_activate(action, data);
		return;
	}

	dados_localizar.localizando_proximo = FALSE;

	const XChangeFile *xf = xchange_hex_view_get_file(XCHANGE_HEX_VIEW(hexv));
	off_t posicao_cursor = xchange_hex_view_get_cursor(XCHANGE_HEX_VIEW(hexv));
	off_t inicio_selecao, fim_selecao;
	if (xchange_hex_view_get_selection_bounds(XCHANGE_HEX_VIEW(hexv), &inicio_selecao, &fim_selecao))
	{
		if (posicao_cursor == inicio_selecao)
			posicao_cursor = fim_selecao-1;
	}
	else
		if (posicao_cursor == 0)
			posicao_cursor = xchange_get_size(xf);
		else
			posicao_cursor -= 1;
	localiza_outro(posicao_cursor, xf, &dados_localizar, TRUE);
}

G_MODULE_EXPORT
void on_radiobutton_localizar_no_intervalo_toggled(GtkToggleButton *toggle, gpointer data)
{
	gtk_widget_set_sensitive(table_localizar_no_intervalo, gtk_toggle_button_get_active(toggle));
}


static gboolean verifica_validade_sequencia_bytes()
{
	uint8_t *bytes_chave;
	const gchar *texto = gtk_entry_get_text(GTK_ENTRY(
			entry_valor_a_localizar));
	if (texto == NULL || g_utf8_strlen(texto, -1)==0)
	{
		return FALSE;
	}

	bytes_chave = recupera_bytes_de_texto_hexa(texto, NULL, "Localização___teste");
	if (bytes_chave == NULL)
	{
		limpa_barra_de_estado("Localização___teste");
		return FALSE;
	}
	else
	{
		free(bytes_chave);
		return TRUE;
	}
}

static gboolean verifica_validade_sequencia_caracteres()
{
	uint8_t *bytes_chave;
	const gchar *texto = gtk_entry_get_text(GTK_ENTRY(
			entry_valor_a_localizar));
	if (texto == NULL || g_utf8_strlen(texto, -1)==0)
	{
		return FALSE;
	}

	bytes_chave = converte_pela_codificacao(texto, NULL, "Localização___teste");
	if (bytes_chave == NULL)
	{
		limpa_barra_de_estado("Localização___teste");
		return FALSE;
	}
	else
	{
		free(bytes_chave);
		return TRUE;
	}
}

G_MODULE_EXPORT
void on_entry_valor_a_localizar_changed(GtkEntry *entry, gpointer data)
{
	const gchar *texto = gtk_entry_get_text(GTK_ENTRY(
			entry));
	if (texto == NULL || g_utf8_strlen(texto, -1)==0)
	{
		gtk_widget_set_sensitive(radiobutton_localizar_bytes, TRUE);
		gtk_widget_set_sensitive(radiobutton_localizar_texto, TRUE);
		gtk_action_set_sensitive(action_localizar, FALSE);
		gtk_action_set_sensitive(action_substituir, FALSE);
		gtk_action_set_sensitive(action_substituir_todos, FALSE);
		return;
	}

	gtk_action_set_sensitive(action_localizar, TRUE);
	gtk_action_set_sensitive(action_substituir, TRUE);
	gtk_action_set_sensitive(action_substituir_todos, TRUE);

	// Confere se pode ser uma sequência de bytes
	if (verifica_validade_sequencia_bytes())
	{
		gtk_widget_set_sensitive(radiobutton_localizar_bytes, TRUE);
	}
	else
	{
		gtk_widget_set_sensitive(radiobutton_localizar_bytes, FALSE);
		if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(radiobutton_localizar_bytes)))
		{
			gtk_action_set_sensitive(action_localizar, FALSE);
			gtk_action_set_sensitive(action_substituir, FALSE);
			gtk_action_set_sensitive(action_substituir_todos, FALSE);
		}
	}

	// Confere se pode ser texto mesmo
	if (verifica_validade_sequencia_caracteres())
	{
		gtk_widget_set_sensitive(radiobutton_localizar_texto, TRUE);
	}
	else
	{
		gtk_widget_set_sensitive(radiobutton_localizar_texto, FALSE);
		if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(radiobutton_localizar_texto)))
		{
			gtk_action_set_sensitive(action_localizar, FALSE);
			gtk_action_set_sensitive(action_substituir, FALSE);
			gtk_action_set_sensitive(action_substituir_todos, FALSE);
		}
	}

	if (gtk_widget_get_visible(button_substituir))
	{
		struct DadosLocalizar *parametrosLocalizar = obtem_parametros_localizar();
		if (confere_tamanhos_textos_localizar_substituir(parametrosLocalizar->localizando_bytes))
			gtk_widget_hide(vbox_substituir_por_tamanho_diferente);
		else
			gtk_widget_show(vbox_substituir_por_tamanho_diferente);

		destroi_dados_localizar(parametrosLocalizar);
		free(parametrosLocalizar);
	}
}

G_MODULE_EXPORT
void on_radiobutton_localizar_texto_toggled(GtkRadioButton *radiobutton, gpointer data)
{
	if ( !gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(radiobutton)))
		return;

	gtk_action_set_sensitive(action_localizar, TRUE);
	gtk_action_set_sensitive(action_substituir, TRUE);
	gtk_action_set_sensitive(action_substituir_todos, TRUE);

	if (verifica_validade_sequencia_caracteres())
	{
		gtk_widget_set_sensitive(GTK_WIDGET(radiobutton), TRUE);
	}
	else
	{
		gtk_widget_set_sensitive(GTK_WIDGET(radiobutton), FALSE);
		if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(radiobutton)))
		{
			gtk_action_set_sensitive(action_localizar, FALSE);
			gtk_action_set_sensitive(action_substituir, FALSE);
			gtk_action_set_sensitive(action_substituir_todos, FALSE);
		}
	}

	if (confere_tamanhos_textos_localizar_substituir(FALSE))
		gtk_widget_hide(vbox_substituir_por_tamanho_diferente);
	else
		gtk_widget_show(vbox_substituir_por_tamanho_diferente);
}

G_MODULE_EXPORT
void on_radiobutton_localizar_bytes_toggled(GtkRadioButton *radiobutton, gpointer data)
{
	if ( !gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(radiobutton)))
		return;

	gtk_action_set_sensitive(action_localizar, TRUE);
	gtk_action_set_sensitive(action_substituir, TRUE);
	gtk_action_set_sensitive(action_substituir_todos, TRUE);

	if (verifica_validade_sequencia_bytes())
	{
		gtk_widget_set_sensitive(GTK_WIDGET(radiobutton), TRUE);
	}
	else
	{
		gtk_widget_set_sensitive(GTK_WIDGET(radiobutton), FALSE);
		if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(radiobutton)))
		{
			gtk_action_set_sensitive(action_localizar, FALSE);
			gtk_action_set_sensitive(action_substituir, FALSE);
			gtk_action_set_sensitive(action_substituir_todos, FALSE);
		}
	}

	if (confere_tamanhos_textos_localizar_substituir(TRUE))
		gtk_widget_hide(vbox_substituir_por_tamanho_diferente);
	else
		gtk_widget_show(vbox_substituir_por_tamanho_diferente);
}

G_MODULE_EXPORT
void on_checkbutton_localizar_para_tras_toggled(GtkCheckButton *checkbutton, gpointer data)
{
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(checkbutton)))
		gtk_button_set_label(GTK_BUTTON(radiobutton_localizar_do_inicio), "Desde o fim");
	else
		gtk_button_set_label(GTK_BUTTON(radiobutton_localizar_do_inicio), "Desde o início");
}

G_MODULE_EXPORT
void on_action_substituir_activate(GtkAction *action, gpointer data)
{
	// Verificar se há algo selecionado e se ele corresponde ao texto que deve ser substituído
	//  Se for, substitui
	//  Senão, apenas localiza a próxima etapa coincidente

	uint8_t *bytes = NULL, *bytes_selecionados = NULL;
	int tamanho_bytes;
	size_t tamanho_selecionado;
	const gchar *texto = NULL;

	struct DadosLocalizar *parametrosLocalizar = obtem_parametros_localizar();
	if (parametrosLocalizar == NULL)
		return;

	gboolean pode_alterar_tamanho = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(checkbutton_aceitar_alterar_tamanho_ao_substituir));

	texto = gtk_entry_get_text(GTK_ENTRY(entry_valor_a_localizar));

	if (parametrosLocalizar->localizando_bytes)
	{
		if (!verifica_validade_sequencia_bytes())
		{
			showErrorMessage(GTK_WINDOW(main_window), "Sequência de bytes inválida!");
			destroi_dados_localizar(parametrosLocalizar);
			g_free(parametrosLocalizar);
			return;
		}
		else
		{
			bytes = recupera_bytes_de_texto_hexa(texto, &tamanho_bytes, NULL);
		}
	}
	else
	{
		if (!verifica_validade_sequencia_caracteres())
		{
			showErrorMessage(GTK_WINDOW(main_window), "Sequência de caracteres inválida!");
			destroi_dados_localizar(parametrosLocalizar);
			g_free(parametrosLocalizar);
			return;
		}
		else
		{
			bytes = converte_pela_codificacao(texto, &tamanho_bytes, NULL);
		}
	}

	if (bytes == NULL)
		return; // FIXME: Reclamar o motivo: texto em branco?

	bytes_selecionados = xchange_hex_view_get_selected_bytes(XCHANGE_HEX_VIEW(hexv), &tamanho_selecionado);

	gboolean precisa_selecionar = (bytes == NULL || bytes_selecionados == NULL || tamanho_bytes != tamanho_selecionado || memcmp(bytes, bytes_selecionados, tamanho_bytes) != 0);
	free(bytes);
	free(bytes_selecionados);

	const XChangeFile *xf = xchange_hex_view_get_file(XCHANGE_HEX_VIEW(hexv));

	if (precisa_selecionar)
	{
		off_t sel_inicio, sel_fim;
		off_t pos_cursor = xchange_hex_view_get_cursor(XCHANGE_HEX_VIEW(hexv));
		off_t pos_inicio;
		if (xchange_hex_view_get_selection_bounds(XCHANGE_HEX_VIEW(hexv), &sel_inicio, &sel_fim))
		{
			if (parametrosLocalizar->localizando_proximo)
				pos_inicio = sel_inicio;
			else
				pos_inicio = sel_fim;
		}
		else
			pos_inicio = pos_cursor;
		if (pos_inicio < parametrosLocalizar->inicio || (parametrosLocalizar->fim > 0 && pos_inicio > parametrosLocalizar->fim))
		{
			if (parametrosLocalizar->localizando_proximo)
				pos_inicio = parametrosLocalizar->inicio;
			else
				pos_inicio = parametrosLocalizar->fim;
		}
		else
		{
			if (parametrosLocalizar->localizando_proximo)
			{
				if (pos_inicio < parametrosLocalizar->fim)
					pos_inicio ++;
			}
			else
			{
				if (pos_inicio > parametrosLocalizar->inicio)
					pos_inicio --;
			}
		}
		localiza_outro(pos_inicio, xf, parametrosLocalizar, TRUE);
		destroi_dados_localizar(parametrosLocalizar);
		g_free(parametrosLocalizar);
		return;
	}

	off_t inicio_selecao, fim_selecao;
	uint8_t *bytes_substituidores = NULL;
	int tamanho_substituidor;
	const gchar *texto_para_substituir = gtk_entry_get_text(GTK_ENTRY(entry_valor_para_substituir));
	if (parametrosLocalizar->localizando_bytes)
	{
		bytes_substituidores = recupera_bytes_de_texto_hexa(texto_para_substituir, &tamanho_substituidor, NULL);
	}
	else
	{
		bytes_substituidores = converte_pela_codificacao(texto_para_substituir, &tamanho_substituidor, NULL);
	}
	if (tamanho_substituidor < 0)
		tamanho_substituidor = 0;


	xchange_hex_view_get_selection_bounds(XCHANGE_HEX_VIEW(hexv), &inicio_selecao, &fim_selecao);


	int menor_tamanho = MIN(tamanho_substituidor, tamanho_selecionado);
	xchange_hex_view_replace_bytes(XCHANGE_HEX_VIEW(hexv), bytes_substituidores, inicio_selecao, menor_tamanho);

	if (pode_alterar_tamanho)
	{
		// TODO: Agrupar ações de desfazer/refazer (para virar uma coisa só isso aqui: substituir e excluir/ou/inserir)
		if (tamanho_substituidor > tamanho_selecionado)
		{
			xchange_hex_view_insert_bytes(XCHANGE_HEX_VIEW(hexv), bytes_substituidores + tamanho_selecionado, inicio_selecao+ tamanho_selecionado, tamanho_substituidor - tamanho_selecionado);
			xchange_hex_view_select(XCHANGE_HEX_VIEW(hexv), inicio_selecao, tamanho_substituidor);
		}
		else if (tamanho_substituidor && tamanho_substituidor < tamanho_selecionado)
		{
			xchange_hex_view_delete_bytes(XCHANGE_HEX_VIEW(hexv), inicio_selecao+ tamanho_substituidor, tamanho_selecionado - tamanho_substituidor);
			xchange_hex_view_select(XCHANGE_HEX_VIEW(hexv), inicio_selecao, tamanho_substituidor);
		}
		xchange_hex_view_select(XCHANGE_HEX_VIEW(hexv), inicio_selecao, tamanho_substituidor);
	}

	free(bytes_substituidores);

	localiza_outro(parametrosLocalizar->localizando_proximo ? fim_selecao+1 : inicio_selecao - 1, xf, parametrosLocalizar, TRUE);

	destroi_dados_localizar(&dados_localizar);
	dados_localizar = *parametrosLocalizar;
	g_free(parametrosLocalizar);
}


G_MODULE_EXPORT
void on_action_fechar_arquivo_activate(GtkAction *action, gpointer data)
{
	filehandler_on_action_close_activate(action, data);
}

G_MODULE_EXPORT
gboolean on_spinbutton_cursor_output(GtkSpinButton *spin_button,
		gpointer user_data)
{
	GtkAdjustment *adj;
	gchar *text;
	int value;
	adj = gtk_spin_button_get_adjustment(spin_button);
	value = (int) gtk_adjustment_get_value(adj);
	text = g_strdup_printf("0x%08X", value);
	gtk_entry_set_text(GTK_ENTRY(spin_button), text);
	g_free(text);

	return TRUE;
}

G_MODULE_EXPORT
gint on_spinbutton_cursor_input(GtkSpinButton *spinbutton, gpointer arg1, gpointer data)
{
	const gchar *texto = gtk_entry_get_text(GTK_ENTRY(spinbutton));
	off_t valor;
	valor =	converte_texto_em_posicao(texto);
	if (valor == (off_t) -1)
		pipoca_na_barra_de_estado("Localizar", "Não há endereço para onde ir.");
	else if (valor == (off_t) -2)
		pipoca_na_barra_de_estado("Localizar", "Não conseguiu entender o endereço.");
	if (arg1 != NULL)
		*(gdouble*)arg1 = valor;
	return valor;
}

G_MODULE_EXPORT
void on_adjustment_cursor_value_changed(GtkAdjustment *adjustment, gpointer data)
{
	// TODO: Não é a melhor solução!
	xchange_hex_view_goto(XCHANGE_HEX_VIEW(hexv), gtk_adjustment_get_value(adjustment));
}

static void mudou_posicao_cursor(XChangeHexView *hexv, gpointer data)
{
	GtkAdjustment *adj;
	off_t pos_cursor = xchange_hex_view_get_cursor(hexv);
	size_t max_pos_cursor = xchange_get_size(xchange_hex_view_get_file(hexv));
	adj = gtk_spin_button_get_adjustment(GTK_SPIN_BUTTON(spinbutton_cursor));
	gtk_adjustment_set_upper(adj, max_pos_cursor - 1);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(spinbutton_cursor), pos_cursor);
	gtk_entry_set_progress_fraction(GTK_ENTRY(spinbutton_cursor), pos_cursor/(gdouble) max_pos_cursor);
}

G_MODULE_EXPORT
void on_toggleaction_modo_edicao_toggled(GtkToggleAction *toggleaction,
		gpointer data)
{
	gboolean insercao = gtk_toggle_action_get_active(toggleaction);
	xchange_hex_view_set_edition_mode(XCHANGE_HEX_VIEW(hexv), insercao);
}

static void mudou_modo_edicao(XChangeHexView *hexv, gpointer data)
{
	gtk_toggle_action_set_active(GTK_TOGGLE_ACTION(toggleaction_modo_insercao), xchange_hex_view_is_insertion_mode(hexv));
}

static void mudou_selecao(XChangeHexView *hexv, gpointer data)
{
	off_t inicio, fim;

	if (xchange_hex_view_get_selection_bounds(hexv, &inicio, &fim))
	{
		gchar *texto = g_strdup_printf("Seleção de x%1$08lX a x%2$08lX (%3$lu / x%3$lX bytes)", inicio, fim, fim-inicio+1);
		pipoca_na_barra_de_estado("Seleção", texto);
		g_free(texto);
	}
	else
	{
		limpa_barra_de_estado("Seleção");
	}
}

static void mudou_conteudo(XChangeHexView *hexv, gpointer data)
{
	Filehandler *fh = data; // TODO: Mudar para dados dos aplicativos

	const XChangeFile *xf = xchange_hex_view_get_file(XCHANGE_HEX_VIEW(hexv));
	gtk_action_set_sensitive(action_undo, xchange_has_undo(xf));
	gtk_action_set_sensitive(action_redo, xchange_has_redo(xf));


	limpa_barra_de_estado("Arquivo");

	if (changes_until_last_save == xchange_get_undo_list_size(xf))
	{
		file_changes_saved = TRUE;
		gtk_label_set_text(GTK_LABEL(label_arquivo_modificado), " ");
		filehandler_file_changed(fh, FALSE);
	}
	else
	{
		file_changes_saved = FALSE;
		gtk_label_set_text(GTK_LABEL(label_arquivo_modificado), "*");

		filehandler_file_changed(fh, TRUE);
	}
}

G_MODULE_EXPORT
void on_action_abre_preferencias_activate(GtkAction *action, gpointer data)
{
	if (!abrir_dialogo_preferencias(&preferencias))
		showErrorMessage(GTK_WINDOW(main_window), "Não conseguiu abrir a janela de preferências.");
}

