#include "db.h"
#include "generic-row.h"
#include "schema-row.h"

static PGconn *db_conn = NULL;

gboolean
db_connect (void)
{
  db_conn = PQconnectdb ("host=localhost "
                         "port=5432 "
                         "dbname=botdb "
                         "user=botje "
                         "password=rata");

  if (PQstatus (db_conn) != CONNECTION_OK)
    {
      g_printerr ("DB connection failed: %s\n", PQerrorMessage (db_conn));
      return FALSE;
    }

  return TRUE;
}

void
db_disconnect (void)
{
  if (db_conn)
    PQfinish (db_conn);
}

GPtrArray *
db_fetch_table_names (void)
{
  const char *query = "SELECT table_name "
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
    g_ptr_array_add (tables, g_strdup (PQgetvalue (res, i, 0)));

  PQclear (res);
  return tables;
}

GListStore *
db_fetch_schema (const char *table_name)
{
  char *escaped = PQescapeLiteral (db_conn, table_name, strlen (table_name));

  char *query = g_strdup_printf ("SELECT column_name, data_type, is_nullable "
                                 "FROM information_schema.columns "
                                 "WHERE table_name = %s",
                                 escaped);

  PQfreemem (escaped);

  PGresult *res = PQexec (db_conn, query);
  g_free (query);

  if (PQresultStatus (res) != PGRES_TUPLES_OK)
    {
      g_printerr ("Schema query failed: %s\n", PQerrorMessage (db_conn));
      PQclear (res);
      return NULL;
    }

  GListStore *store = g_list_store_new (TYPE_SCHEMA_ROW);

  int rows = PQntuples (res);

  for (int i = 0; i < rows; i++)
    {
      SchemaRow *row =
          schema_row_new (PQgetvalue (res, i, 0), PQgetvalue (res, i, 1),
                          PQgetvalue (res, i, 2));

      g_list_store_append (store, row);
      g_object_unref (row);
    }

  PQclear (res);
  return store;
}

GListStore *
db_fetch_top_100 (const char *table_name)
{
  char *escaped = PQescapeIdentifier (db_conn, table_name, strlen (table_name));

  char *query = g_strdup_printf ("SELECT * FROM %s LIMIT 100", escaped);

  PQfreemem (escaped);

  PGresult *res = PQexec (db_conn, query);
  g_free (query);

  if (PQresultStatus (res) != PGRES_TUPLES_OK)
    {
      g_printerr ("Query failed: %s\n", PQerrorMessage (db_conn));
      PQclear (res);
      return NULL;
    }

  int rows = PQntuples (res);
  int cols = PQnfields (res);

  GListStore *store = g_list_store_new (TYPE_GENERIC_ROW);

  for (int i = 0; i < rows; i++)
    {
      GenericRow *row = generic_row_new (cols);

      for (int c = 0; c < cols; c++)
        {
          const char *val = PQgetvalue (res, i, c);

          generic_row_set_value (row, c, val ? val : "");
        }

      g_list_store_append (store, row);
      g_object_unref (row);
    }

  PQclear (res);
  return store;
}