#include "db_config.h"

#include <stdio.h>
#include <string.h>
#include <yaml.h>

int
load_db_config (const char *filename, DbConfig *config)
{
  FILE *fh = fopen (filename, "r");
  if (!fh)
    {
      perror ("fopen");
      return 0;
    }

  memset (config, 0, sizeof (DbConfig));

  yaml_parser_t parser;
  yaml_event_t event;

  if (!yaml_parser_initialize (&parser))
    {
      fprintf (stderr, "Failed to initialize YAML parser\n");
      fclose (fh);
      return 0;
    }

  yaml_parser_set_input_file (&parser, fh);

  char current_key[64] = { 0 };
  int in_database = 0;

  while (1)
    {
      if (!yaml_parser_parse (&parser, &event))
        break;

      if (event.type == YAML_SCALAR_EVENT)
        {
          char *value = (char *) event.data.scalar.value;

          if (strcmp (value, "database") == 0)
            {
              in_database = 1;
            }
          else if (in_database && current_key[0] == '\0')
            {
              strncpy (current_key, value, sizeof (current_key) - 1);
            }
          else if (in_database)
            {
              if (strcmp (current_key, "host") == 0)
                strncpy (config->host, value, sizeof (config->host) - 1);
              else if (strcmp (current_key, "port") == 0)
                strncpy (config->port, value, sizeof (config->port) - 1);
              else if (strcmp (current_key, "dbname") == 0)
                strncpy (config->dbname, value, sizeof (config->dbname) - 1);
              else if (strcmp (current_key, "user") == 0)
                strncpy (config->user, value, sizeof (config->user) - 1);
              else if (strcmp (current_key, "password") == 0)
                strncpy (config->password, value, sizeof (config->password) - 1);

              current_key[0] = '\0';
            }
        }

      if (event.type == YAML_STREAM_END_EVENT)
        {
          yaml_event_delete (&event);
          break;
        }

      yaml_event_delete (&event);
    }

  yaml_parser_delete (&parser);
  fclose (fh);

  return 1;
}

PGconn *
db_connect_from_config (const DbConfig *config)
{
  char conninfo[512];

  snprintf (conninfo, sizeof (conninfo), "host=%s port=%s dbname=%s user=%s password=%s",
            config->host, config->port, config->dbname, config->user, config->password);

  printf ("%s\n", conninfo);

  PGconn *conn = PQconnectdb (conninfo);

  if (PQstatus (conn) != CONNECTION_OK)
    {
      fprintf (stderr, "Connection failed: %s\n", PQerrorMessage (conn));
    }

  return conn;
}