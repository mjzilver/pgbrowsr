#include "ui.h"
#include "db.h"
#include "generic-row.h"
#include "gtk/gtkshortcut.h"
#include "schema-row.h"

#include <gtk/gtk.h>
#include <stdio.h>

typedef struct
{
  GtkWidget *schema_view;
  GtkWidget *data_view;

  GListStore *schema_store;
  GListStore *data_store;

  GListStore *query_store;
  GtkWidget *query_view;
  GtkWidget *sql_view;

  char *current_table;
} AppWidgets;

static void
clear_column_view (GtkColumnView *view)
{
  GListModel *columns = gtk_column_view_get_columns (view);

  while (g_list_model_get_n_items (columns) > 0)
    {
      GtkColumnViewColumn *col = g_list_model_get_item (columns, 0);

      gtk_column_view_remove_column (view, col);
      g_object_unref (col);
    }
}

static void
label_setup (GtkListItemFactory *factory, GtkListItem *item, gpointer user_data)
{
  (void) factory;
  (void) user_data;

  GtkWidget *label = gtk_label_new (NULL);

  gtk_label_set_xalign (GTK_LABEL (label), 0.0f);

  gtk_widget_set_margin_start (label, 8);
  gtk_widget_set_margin_end (label, 8);
  gtk_widget_set_margin_top (label, 2);
  gtk_widget_set_margin_bottom (label, 2);

  gtk_list_item_set_child (item, label);
}

static void
schema_name_bind (GtkListItemFactory *factory, GtkListItem *item, gpointer user_data)
{
  (void) factory;
  (void) user_data;

  GtkWidget *label = gtk_list_item_get_child (item);
  SchemaRow *row = gtk_list_item_get_item (item);

  gtk_label_set_text (GTK_LABEL (label), schema_row_get_name (row));
}

static void
schema_type_bind (GtkListItemFactory *factory, GtkListItem *item, gpointer user_data)
{
  (void) factory;
  (void) user_data;

  GtkWidget *label = gtk_list_item_get_child (item);
  SchemaRow *row = gtk_list_item_get_item (item);

  gtk_label_set_text (GTK_LABEL (label), schema_row_get_type_str (row));
}

static void
schema_nullable_bind (GtkListItemFactory *factory, GtkListItem *item, gpointer user_data)
{
  (void) factory;
  (void) user_data;

  GtkWidget *label = gtk_list_item_get_child (item);
  SchemaRow *row = gtk_list_item_get_item (item);

  gtk_label_set_text (GTK_LABEL (label), schema_row_get_nullable (row));
}

static void
data_bind (GtkListItemFactory *factory, GtkListItem *item, gpointer user_data)
{
  (void) factory;

  int column_index = GPOINTER_TO_INT (user_data);

  GtkWidget *label = gtk_list_item_get_child (item);
  GenericRow *row = gtk_list_item_get_item (item);

  gtk_label_set_text (GTK_LABEL (label), generic_row_get_value (row, column_index));
}

static void
append_schema_columns (GtkColumnView *view)
{
  const char *titles[] = { "Name", "Type", "Nullable" };

  GCallback binds[] = { G_CALLBACK (schema_name_bind), G_CALLBACK (schema_type_bind),
                        G_CALLBACK (schema_nullable_bind) };

  for (int i = 0; i < 3; i++)
    {
      GtkListItemFactory *factory = gtk_signal_list_item_factory_new ();

      g_signal_connect (factory, "setup", G_CALLBACK (label_setup), NULL);

      g_signal_connect (factory, "bind", binds[i], NULL);

      GtkColumnViewColumn *col = gtk_column_view_column_new (titles[i], factory);

      gtk_column_view_append_column (view, col);
    }
}

static void
append_data_columns (GtkColumnView *view, GListStore *data_store, GListStore *schema_store)
{
  guint rows = g_list_model_get_n_items (G_LIST_MODEL (data_store));
  if (rows == 0)
    return;

  GenericRow *first = g_list_model_get_item (G_LIST_MODEL (data_store), 0);

  int cols = generic_row_get_n_columns (first);
  g_object_unref (first);

  for (int c = 0; c < cols; c++)
    {
      GtkListItemFactory *factory = gtk_signal_list_item_factory_new ();

      g_signal_connect (factory, "setup", G_CALLBACK (label_setup), NULL);

      g_signal_connect (factory, "bind", G_CALLBACK (data_bind), GINT_TO_POINTER (c));

      const char *col_name = NULL;

      if (schema_store && g_list_model_get_n_items (G_LIST_MODEL (schema_store)) > (guint) c)
        {
          SchemaRow *schema_row = g_list_model_get_item (G_LIST_MODEL (schema_store), c);

          col_name = schema_row_get_name (schema_row);
          g_object_unref (schema_row);
        }

      GtkColumnViewColumn *column = gtk_column_view_column_new (col_name, factory);

      gtk_column_view_append_column (view, column);
    }
}

static void
on_table_clicked (GtkWidget *button, gpointer user_data)
{
  AppWidgets *app = user_data;

  const char *table_name = gtk_button_get_label (GTK_BUTTON (button));

  if (app->current_table && strcmp (table_name, app->current_table) == 0)
    return;

  g_list_store_remove_all (app->data_store);
  clear_column_view (GTK_COLUMN_VIEW (app->data_view));

  g_free (app->current_table);
  app->current_table = g_strdup (table_name);

  GListStore *new_schema = db_fetch_schema (table_name);

  if (!new_schema)
    return;

  g_list_store_remove_all (app->schema_store);

  guint n = g_list_model_get_n_items (G_LIST_MODEL (new_schema));

  for (guint i = 0; i < n; i++)
    {
      gpointer item = g_list_model_get_item (G_LIST_MODEL (new_schema), i);

      g_list_store_append (app->schema_store, item);
      g_object_unref (item);
    }

  g_object_unref (new_schema);
}

static void
populate_table_buttons (GtkWidget *left_box, AppWidgets *widgets)
{
  GPtrArray *tables = db_fetch_table_names ();
  if (!tables)
    return;

  for (guint i = 0; i < tables->len; i++)
    {
      const char *name = g_ptr_array_index (tables, i);
      GtkWidget *btn = gtk_button_new_with_label (name);

      g_signal_connect (btn, "clicked", G_CALLBACK (on_table_clicked), widgets);
      gtk_box_append (GTK_BOX (left_box), btn);
    }

  g_ptr_array_free (tables, TRUE);
}

static void
on_fetch_clicked (GtkWidget *button, gpointer user_data)
{
  (void) button;

  AppWidgets *app = user_data;

  if (!app->current_table)
    return;

  GListStore *new_data = db_fetch_top_100 (app->current_table);

  if (!new_data)
    return;

  g_list_store_remove_all (app->data_store);
  clear_column_view (GTK_COLUMN_VIEW (app->data_view));

  append_data_columns (GTK_COLUMN_VIEW (app->data_view), new_data, app->data_store);

  guint rows = g_list_model_get_n_items (G_LIST_MODEL (new_data));

  for (guint i = 0; i < rows; i++)
    {
      gpointer item = g_list_model_get_item (G_LIST_MODEL (new_data), i);

      g_list_store_append (app->data_store, item);
      g_object_unref (item);
    }

  g_object_unref (new_data);
}
static void
on_run_query_clicked (GtkWidget *button, gpointer user_data)
{
  (void) button;

  AppWidgets *app = user_data;

  if (!app->sql_view)
    return;

  GtkTextBuffer *buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (app->sql_view));

  if (!buffer)
    return;

  GtkTextIter start, end;
  gtk_text_buffer_get_start_iter (buffer, &start);
  gtk_text_buffer_get_end_iter (buffer, &end);

  char *text = gtk_text_buffer_get_text (buffer, &start, &end, FALSE);

  if (!text || text[0] == '\0')
    {
      g_free (text);
      return;
    }


  GListStore *new_data = db_run_query (text);

  g_free (text);

  if (!new_data)
    return;

  g_list_store_remove_all (app->query_store);
  clear_column_view (GTK_COLUMN_VIEW (app->query_view));

  append_data_columns (GTK_COLUMN_VIEW (app->query_view), new_data, app->query_store);

  guint rows = g_list_model_get_n_items (G_LIST_MODEL (new_data));

  for (guint i = 0; i < rows; i++)
    {
      gpointer item = g_list_model_get_item (G_LIST_MODEL (new_data), i);

      g_list_store_append (app->query_store, item);
      g_object_unref (item);
    }

  g_object_unref (new_data);
}

static GtkWidget *
build_left_sidebar (AppWidgets *widgets)
{
  GtkWidget *left_box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 4);

  GtkWidget *left_top_box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 4);

  GtkWidget *left_bottom_box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 4);

  gtk_box_append (GTK_BOX (left_box), left_top_box);
  gtk_box_append (GTK_BOX (left_box), left_bottom_box);

  populate_table_buttons (left_top_box, widgets);

  GtkSelectionModel *schema_sel =
      GTK_SELECTION_MODEL (gtk_single_selection_new (G_LIST_MODEL (widgets->schema_store)));

  widgets->schema_view = gtk_column_view_new (schema_sel);

  gtk_widget_set_vexpand (widgets->schema_view, TRUE);

  append_schema_columns (GTK_COLUMN_VIEW (widgets->schema_view));

  gtk_box_append (GTK_BOX (left_bottom_box), widgets->schema_view);

  gtk_widget_set_hexpand (left_box, FALSE);

  return left_box;
}

static GtkWidget *
build_top100_tab (AppWidgets *widgets)
{
  GtkSelectionModel *data_sel =
      GTK_SELECTION_MODEL (gtk_single_selection_new (G_LIST_MODEL (widgets->data_store)));

  widgets->data_view = gtk_column_view_new (data_sel);

  gtk_widget_set_vexpand (widgets->data_view, TRUE);

  GtkWidget *scroll = gtk_scrolled_window_new ();
  gtk_widget_set_vexpand (scroll, TRUE);

  gtk_scrolled_window_set_child (GTK_SCROLLED_WINDOW (scroll), widgets->data_view);

  GtkWidget *box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 8);

  GtkWidget *fetch_btn = gtk_button_new_with_label ("Fetch Top 100");

  gtk_widget_set_halign (fetch_btn, GTK_ALIGN_START);

  g_signal_connect (fetch_btn, "clicked", G_CALLBACK (on_fetch_clicked), widgets);

  gtk_box_append (GTK_BOX (box), fetch_btn);
  gtk_box_append (GTK_BOX (box), scroll);

  return box;
}

static GtkWidget *
build_query_tab (AppWidgets *widgets)
{
  GtkSelectionModel *query_sel =
      GTK_SELECTION_MODEL (gtk_single_selection_new (G_LIST_MODEL (widgets->query_store)));

  widgets->query_view = gtk_column_view_new (query_sel);

  gtk_widget_set_vexpand (widgets->query_view, TRUE);

  /* SQL editor */
  widgets->sql_view = gtk_text_view_new ();
  gtk_text_view_set_monospace (GTK_TEXT_VIEW (widgets->sql_view), TRUE);

  GtkWidget *sql_scroll = gtk_scrolled_window_new ();
  gtk_widget_set_vexpand (sql_scroll, TRUE);

  gtk_scrolled_window_set_child (GTK_SCROLLED_WINDOW (sql_scroll), widgets->sql_view);

  /* Results table */
  GtkWidget *results_scroll = gtk_scrolled_window_new ();
  gtk_widget_set_vexpand (results_scroll, TRUE);

  gtk_scrolled_window_set_child (GTK_SCROLLED_WINDOW (results_scroll), widgets->query_view);

  GtkWidget *run_btn = gtk_button_new_with_label ("Run Query");

  gtk_widget_set_halign (run_btn, GTK_ALIGN_START);

  g_signal_connect (run_btn, "clicked", G_CALLBACK (on_run_query_clicked), widgets);

  GtkWidget *bottom_box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 8);

  gtk_box_append (GTK_BOX (bottom_box), run_btn);
  gtk_box_append (GTK_BOX (bottom_box), results_scroll);

  GtkWidget *paned = gtk_paned_new (GTK_ORIENTATION_VERTICAL);

  gtk_paned_set_start_child (GTK_PANED (paned), sql_scroll);

  gtk_paned_set_end_child (GTK_PANED (paned), bottom_box);

  gtk_paned_set_position (GTK_PANED (paned), 200);

  return paned;
}

void
ui_activate (GtkApplication *app, gpointer user_data)
{
  (void) user_data;

  GtkWidget *window = gtk_application_window_new (app);

  gtk_window_set_title (GTK_WINDOW (window), "PgBrowsr");
  gtk_window_set_default_size (GTK_WINDOW (window), 1100, 700);

  AppWidgets *widgets = g_new0 (AppWidgets, 1);

  widgets->schema_store = g_list_store_new (TYPE_SCHEMA_ROW);
  widgets->data_store = g_list_store_new (TYPE_GENERIC_ROW);
  widgets->query_store = g_list_store_new (TYPE_GENERIC_ROW);

  GtkWidget *main_box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 8);

  gtk_window_set_child (GTK_WINDOW (window), main_box);

  GtkWidget *left = build_left_sidebar (widgets);
  GtkWidget *right = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);

  gtk_widget_set_hexpand (right, TRUE);

  /* Notebook */
  GtkWidget *notebook = gtk_notebook_new ();

  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), build_top100_tab (widgets),
                            gtk_label_new ("Top 100"));

  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), build_query_tab (widgets),
                            gtk_label_new ("Query Results"));

  gtk_box_append (GTK_BOX (right), notebook);

  gtk_box_append (GTK_BOX (main_box), left);
  gtk_box_append (GTK_BOX (main_box), right);

  gtk_window_present (GTK_WINDOW (window));
}