#include "ui.h"
#include "db.h"
#include "generic-row.h"
#include "schema-row.h"
#include <gtk/gtk.h>

typedef struct
{
  GtkWidget *schema_view;
  GtkWidget *data_view;

  GListStore *schema_store;
  GListStore *data_store;

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
  // Add horizontal margins for better spacing
  gtk_widget_set_margin_start (label, 8);
  gtk_widget_set_margin_end (label, 8);
  gtk_widget_set_margin_top (label, 2);
  gtk_widget_set_margin_bottom (label, 2);

  gtk_list_item_set_child (item, label);
}

static void
schema_name_bind (GtkListItemFactory *factory,
                  GtkListItem *item,
                  gpointer user_data)
{
  (void) factory;
  (void) user_data;

  GtkWidget *label = gtk_list_item_get_child (item);

  SchemaRow *row = gtk_list_item_get_item (item);

  gtk_label_set_text (GTK_LABEL (label), schema_row_get_name (row));
}

static void
schema_type_bind (GtkListItemFactory *factory,
                  GtkListItem *item,
                  gpointer user_data)
{
  (void) factory;
  (void) user_data;

  GtkWidget *label = gtk_list_item_get_child (item);

  SchemaRow *row = gtk_list_item_get_item (item);

  gtk_label_set_text (GTK_LABEL (label), schema_row_get_type_str (row));
}

static void
schema_nullable_bind (GtkListItemFactory *factory,
                      GtkListItem *item,
                      gpointer user_data)
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

  gtk_label_set_text (GTK_LABEL (label),
                      generic_row_get_value (row, column_index));
}

static void
on_table_clicked (GtkWidget *button, gpointer user_data)
{
  (void) button;

  AppWidgets *app = user_data;

  const char *table_name = gtk_button_get_label (GTK_BUTTON (button));

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

  guint rows = g_list_model_get_n_items (G_LIST_MODEL (new_data));

  if (rows == 0)
    {
      g_object_unref (new_data);
      return;
    }

  GenericRow *first = g_list_model_get_item (G_LIST_MODEL (new_data), 0);

  int cols = generic_row_get_n_columns (first);

  g_object_unref (first);

  for (int c = 0; c < cols; c++)
    {
      GtkListItemFactory *factory = gtk_signal_list_item_factory_new ();

      g_signal_connect (factory, "setup", G_CALLBACK (label_setup), NULL);

      g_signal_connect (factory, "bind", G_CALLBACK (data_bind),
                        GINT_TO_POINTER (c));

      char title[32];
      g_snprintf (title, sizeof (title), "Col %d", c + 1);

      GtkColumnViewColumn *column = gtk_column_view_column_new (title, factory);

      gtk_column_view_append_column (GTK_COLUMN_VIEW (app->data_view), column);
    }

  for (guint i = 0; i < rows; i++)
    {
      gpointer item = g_list_model_get_item (G_LIST_MODEL (new_data), i);

      g_list_store_append (app->data_store, item);

      g_object_unref (item);
    }

  g_object_unref (new_data);
}

void
ui_activate (GtkApplication *app, gpointer user_data)
{
  (void) user_data;

  GtkWidget *window = gtk_application_window_new (app);

  gtk_window_set_title (GTK_WINDOW (window), "PgBrowsr");

  gtk_window_set_default_size (GTK_WINDOW (window), 1100, 700);

  AppWidgets *widgets = g_new0 (AppWidgets, 1);

  GtkWidget *main_box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 8);

  gtk_window_set_child (GTK_WINDOW (window), main_box);

  GtkWidget *left_box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 4);

  gtk_widget_set_size_request (left_box, 200, -1);

  gtk_box_append (GTK_BOX (main_box), left_box);

  GtkWidget *right_box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_widget_set_size_request (right_box, 800, -1);

  gtk_box_append (GTK_BOX (main_box), right_box);

  widgets->schema_store = g_list_store_new (TYPE_SCHEMA_ROW);

  widgets->data_store = g_list_store_new (TYPE_GENERIC_ROW);

  GtkSelectionModel *schema_sel = GTK_SELECTION_MODEL (
      gtk_single_selection_new (G_LIST_MODEL (widgets->schema_store)));

  widgets->schema_view = gtk_column_view_new (schema_sel);

  gtk_widget_set_vexpand (widgets->schema_view, TRUE);

  gtk_box_append (GTK_BOX (right_box), widgets->schema_view);

  const char *titles[] = { "Name", "Type", "Nullable" };

  GCallback binds[] = { G_CALLBACK (schema_name_bind),
                        G_CALLBACK (schema_type_bind),
                        G_CALLBACK (schema_nullable_bind) };

  for (int i = 0; i < 3; i++)
    {
      GtkListItemFactory *factory = gtk_signal_list_item_factory_new ();

      g_signal_connect (factory, "setup", G_CALLBACK (label_setup), NULL);

      g_signal_connect (factory, "bind", binds[i], NULL);

      GtkColumnViewColumn *col =
          gtk_column_view_column_new (titles[i], factory);

      gtk_column_view_append_column (GTK_COLUMN_VIEW (widgets->schema_view),
                                     col);
    }

  GtkWidget *fetch_btn = gtk_button_new_with_label ("Fetch Top 100");

  gtk_box_append (GTK_BOX (right_box), fetch_btn);

  g_signal_connect (fetch_btn, "clicked", G_CALLBACK (on_fetch_clicked),
                    widgets);

  GtkSelectionModel *data_sel = GTK_SELECTION_MODEL (
      gtk_single_selection_new (G_LIST_MODEL (widgets->data_store)));

  widgets->data_view = gtk_column_view_new (data_sel);

  gtk_widget_set_vexpand (widgets->data_view, TRUE);

  gtk_box_append (GTK_BOX (right_box), widgets->data_view);

  GPtrArray *tables = db_fetch_table_names ();

  if (tables)
    {
      for (guint i = 0; i < tables->len; i++)
        {
          const char *name = g_ptr_array_index (tables, i);

          GtkWidget *btn = gtk_button_new_with_label (name);

          g_signal_connect (btn, "clicked", G_CALLBACK (on_table_clicked),
                            widgets);

          gtk_box_append (GTK_BOX (left_box), btn);
        }

      g_ptr_array_free (tables, TRUE);
    }

  gtk_window_present (GTK_WINDOW (window));
}