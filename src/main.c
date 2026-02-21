#include <gtk/gtk.h>
#include <libpq-fe.h>
#include <stdio.h>

static PGconn *db_conn = NULL;

#define TYPE_SCHEMA_ROW (schema_row_get_type ())
G_DECLARE_FINAL_TYPE (SchemaRow, schema_row, SCHEMA, ROW, GObject)

struct _SchemaRow
{
  GObject parent_instance;

  char *column;
  char *type;
  char *nullable;
};

G_DEFINE_TYPE (SchemaRow, schema_row, G_TYPE_OBJECT)

static void
schema_row_dispose (GObject *object)
{
  SchemaRow *row = SCHEMA_ROW (object);

  g_clear_pointer (&row->column, g_free);
  g_clear_pointer (&row->type, g_free);
  g_clear_pointer (&row->nullable, g_free);

  G_OBJECT_CLASS (schema_row_parent_class)->dispose (object);
}

static void
schema_row_class_init (SchemaRowClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  object_class->dispose = schema_row_dispose;
}

static void
schema_row_init (SchemaRow *self)
{
  (void) self;
}

static GPtrArray *
db_fetch_table_names (void)
{
  static const char *query = "SELECT table_name "
                             "FROM information_schema.tables "
                             "WHERE table_schema = 'public'";

  PGresult *res = PQexec (db_conn, query);

  if (PQresultStatus (res) != PGRES_TUPLES_OK)
    {
      g_printerr ("Table query failed: %s\n", PQerrorMessage (db_conn));
      PQclear (res);
      return NULL;
    }

  GPtrArray *tables = g_ptr_array_new_with_free_func (g_free);

  int rows = PQntuples (res);

  for (int i = 0; i < rows; i++)
    {
      g_ptr_array_add (tables, g_strdup (PQgetvalue (res, i, 0)));
    }

  PQclear (res);
  return tables;
}

typedef struct
{
  char *table_name;
  GListStore *schema_store;
} TableClickContext;

static void
on_table_button_clicked (GtkWidget *widget, gpointer user_data)
{
  (void) widget;
  TableClickContext *ctx = (TableClickContext *) user_data;

  g_print ("Loading schema for table: %s\n", ctx->table_name);

  g_list_store_remove_all (ctx->schema_store);

  char *query = g_strdup_printf ("SELECT column_name, data_type, is_nullable "
                                 "FROM information_schema.columns "
                                 "WHERE table_name = '%s'",
                                 ctx->table_name);

  PGresult *res = PQexec (db_conn, query);
  g_free (query);

  if (PQresultStatus (res) != PGRES_TUPLES_OK)
    {
      g_printerr ("Schema query failed: %s\n", PQerrorMessage (db_conn));
      PQclear (res);
      return;
    }

  int rows = PQntuples (res);

  for (int i = 0; i < rows; i++)
    {

      const char *column = PQgetvalue (res, i, 0);

      const char *type = PQgetvalue (res, i, 1);

      const char *nullable = PQgetvalue (res, i, 2);

      SchemaRow *row = g_object_new (TYPE_SCHEMA_ROW, NULL);

      row->column = g_strdup (column);
      row->type = g_strdup (type);
      row->nullable = g_strdup (nullable);

      g_list_store_append (ctx->schema_store, row);

      g_object_unref (row);
    }

  PQclear (res);
}

static void
table_click_context_free (gpointer data, GClosure *closure)
{
  (void) closure;
  TableClickContext *ctx = data;

  g_free (ctx->table_name);
  g_free (ctx);
}

static void
create_table_button (const char *table_name,
                     GtkWidget *box,
                     GListStore *schema_store)
{
  GtkWidget *button = gtk_button_new_with_label (table_name);

  TableClickContext *ctx = g_new0 (TableClickContext, 1);

  ctx->table_name = g_strdup (table_name);
  ctx->schema_store = schema_store;

  g_signal_connect_data (button, "clicked",
                         G_CALLBACK (on_table_button_clicked), ctx,
                         table_click_context_free, 0);

  gtk_box_append (GTK_BOX (box), button);
}

static void
name_column_setup (GtkListItemFactory *factory,
                   GtkListItem *item,
                   gpointer user_data)
{
  (void) factory;
  (void) user_data;

  GtkWidget *label = gtk_label_new (NULL);
  gtk_list_item_set_child (item, label);
}

static void
name_column_bind (GtkListItemFactory *factory,
                  GtkListItem *item,
                  gpointer user_data)
{
  (void) factory;
  (void) user_data;
  GtkWidget *label = gtk_list_item_get_child (item);

  SchemaRow *row = gtk_list_item_get_item (item);

  gtk_label_set_text (GTK_LABEL (label), row->column);
}

static void
type_column_setup (GtkListItemFactory *factory,
                   GtkListItem *item,
                   gpointer user_data)
{
  (void) factory;
  (void) user_data;

  GtkWidget *label = gtk_label_new (NULL);
  gtk_list_item_set_child (item, label);
}

static void
type_column_bind (GtkListItemFactory *factory,
                  GtkListItem *item,
                  gpointer user_data)
{
  (void) factory;
  (void) user_data;
  GtkWidget *label = gtk_list_item_get_child (item);

  SchemaRow *row = gtk_list_item_get_item (item);

  gtk_label_set_text (GTK_LABEL (label), row->type);
}

static void
nullable_column_setup (GtkListItemFactory *factory,
                       GtkListItem *item,
                       gpointer user_data)
{
  (void) factory;
  (void) user_data;

  GtkWidget *label = gtk_label_new (NULL);
  gtk_list_item_set_child (item, label);
}

static void
nullable_column_bind (GtkListItemFactory *factory,
                      GtkListItem *item,
                      gpointer user_data)
{
  (void) factory;
  (void) user_data;
  GtkWidget *label = gtk_list_item_get_child (item);

  SchemaRow *row = gtk_list_item_get_item (item);

  gtk_label_set_text (GTK_LABEL (label), row->nullable);
}

static void
on_app_activate (GtkApplication *app, gpointer user_data)
{
  (void) user_data;
  GtkWidget *window = gtk_application_window_new (app);

  gtk_window_set_title (GTK_WINDOW (window), "PgBrowsr");

  gtk_window_set_default_size (GTK_WINDOW (window), 1000, 600);

  GtkWidget *main_box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 10);

  gtk_window_set_child (GTK_WINDOW (window), main_box);

  GtkWidget *left_box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 5);

  gtk_widget_set_hexpand (left_box, TRUE);

  gtk_box_append (GTK_BOX (main_box), left_box);

  GListStore *schema_store = g_list_store_new (TYPE_SCHEMA_ROW);

  GtkSelectionModel *selection = GTK_SELECTION_MODEL (
      gtk_single_selection_new (G_LIST_MODEL (schema_store)));

  GtkWidget *column_view = gtk_column_view_new (selection);

  gtk_widget_set_hexpand (column_view, TRUE);

  gtk_box_append (GTK_BOX (main_box), column_view);

  GtkListItemFactory *name_factory = gtk_signal_list_item_factory_new ();

  g_signal_connect (name_factory, "setup", G_CALLBACK (name_column_setup),
                    NULL);

  g_signal_connect (name_factory, "bind", G_CALLBACK (name_column_bind), NULL);

  GtkColumnViewColumn *name_column =
      gtk_column_view_column_new ("Name", name_factory);

  gtk_column_view_append_column (GTK_COLUMN_VIEW (column_view), name_column);

  GtkListItemFactory *type_factory = gtk_signal_list_item_factory_new ();

  g_signal_connect (type_factory, "setup", G_CALLBACK (type_column_setup),
                    NULL);

  g_signal_connect (type_factory, "bind", G_CALLBACK (type_column_bind), NULL);

  GtkColumnViewColumn *type_column =
      gtk_column_view_column_new ("Type", type_factory);

  gtk_column_view_append_column (GTK_COLUMN_VIEW (column_view), type_column);

  GtkListItemFactory *nullable_factory = gtk_signal_list_item_factory_new ();

  g_signal_connect (nullable_factory, "setup",
                    G_CALLBACK (nullable_column_setup), NULL);

  g_signal_connect (nullable_factory, "bind", G_CALLBACK (nullable_column_bind),
                    NULL);

  GtkColumnViewColumn *nullable_column =
      gtk_column_view_column_new ("Nullable", nullable_factory);

  gtk_column_view_append_column (GTK_COLUMN_VIEW (column_view),
                                 nullable_column);

  GPtrArray *tables = db_fetch_table_names ();

  if (!tables)
    return;

  for (guint i = 0; i < tables->len; i++)
    {
      create_table_button (g_ptr_array_index (tables, i), left_box,
                           schema_store);
    }

  g_ptr_array_free (tables, TRUE);

  gtk_window_present (GTK_WINDOW (window));
}

int
main (int argc, char **argv)
{
  GtkApplication *app;
  int status;

  db_conn = PQconnectdb ("host=localhost "
                         "port=5432 "
                         "dbname=botdb "
                         "user=botje "
                         "password=rata");

  if (PQstatus (db_conn) != CONNECTION_OK)
    {
      g_printerr ("DB connection failed: %s\n", PQerrorMessage (db_conn));
      return 1;
    }

  app = gtk_application_new ("org.example.dbexplorer",
                             G_APPLICATION_DEFAULT_FLAGS);

  g_signal_connect (app, "activate", G_CALLBACK (on_app_activate), NULL);

  status = g_application_run (G_APPLICATION (app), argc, argv);

  g_object_unref (app);
  PQfinish (db_conn);

  return status;
}