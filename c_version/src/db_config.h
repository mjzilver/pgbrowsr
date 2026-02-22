#ifndef DB_CONFIG_H
#define DB_CONFIG_H

#include <libpq-fe.h>

typedef struct
{
  char host[64];
  char port[16];
  char dbname[64];
  char user[64];
  char password[64];
} DbConfig;

int load_db_config (const char *filename, DbConfig *config);
PGconn *db_connect_from_config (const DbConfig *config);

#endif