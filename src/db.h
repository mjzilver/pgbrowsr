#ifndef DB_H
#define DB_H

#include <gio/gio.h>
#include <glib.h>
#include <libpq-fe.h>

gboolean db_connect (void);
void db_disconnect (void);

GPtrArray *db_fetch_table_names (void);
GListStore *db_fetch_schema (const char *table_name);
GListStore *db_fetch_top_100 (const char *table_name);
GListStore *db_run_query (const char *query);

#endif