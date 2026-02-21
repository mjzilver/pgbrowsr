#ifndef GENERIC_ROW_H
#define GENERIC_ROW_H

#include <glib-object.h>

#define TYPE_GENERIC_ROW (generic_row_get_type ())
G_DECLARE_FINAL_TYPE (GenericRow, generic_row, GENERIC, ROW, GObject)

GenericRow *generic_row_new (int n_columns);
void generic_row_set_value (GenericRow *row, int index, const char *value);

const char *generic_row_get_value (GenericRow *row, int index);
int generic_row_get_n_columns (GenericRow *row);

#endif