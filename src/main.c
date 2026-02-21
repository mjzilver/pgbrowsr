#include "db.h"
#include "ui.h"
#include <gtk/gtk.h>

int
main (int argc, char **argv)
{
  if (!db_connect ())
    return 1;

  GtkApplication *app = gtk_application_new ("org.example.dbexplorer",
                                             G_APPLICATION_DEFAULT_FLAGS);

  g_signal_connect (app, "activate", G_CALLBACK (ui_activate), NULL);

  int status = g_application_run (G_APPLICATION (app), argc, argv);

  g_object_unref (app);
  db_disconnect ();

  return status;
}