#ifndef SCHEMA_ROW_H
#define SCHEMA_ROW_H

#include <glib-object.h>

#define TYPE_SCHEMA_ROW (schema_row_get_type ())
G_DECLARE_FINAL_TYPE (SchemaRow, schema_row, SCHEMA, ROW, GObject)

SchemaRow *
schema_row_new (const char *name, const char *type, const char *nullable);

const char *schema_row_get_name (SchemaRow *row);
const char *schema_row_get_type_str (SchemaRow *row);
const char *schema_row_get_nullable (SchemaRow *row);

#endif