#include "schema-row.h"

struct _SchemaRow
{
  GObject parent_instance;

  char *name;
  char *type;
  char *nullable;
};

G_DEFINE_TYPE (SchemaRow, schema_row, G_TYPE_OBJECT)

static void
schema_row_dispose (GObject *object)
{
  SchemaRow *row = SCHEMA_ROW (object);

  g_clear_pointer (&row->name, g_free);
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

SchemaRow *
schema_row_new (const char *name, const char *type, const char *nullable)
{
  SchemaRow *row = g_object_new (TYPE_SCHEMA_ROW, NULL);

  row->name = g_strdup (name);
  row->type = g_strdup (type);
  row->nullable = g_strdup (nullable);

  return row;
}

const char *
schema_row_get_name (SchemaRow *row)
{
  return row->name;
}

const char *
schema_row_get_type_str (SchemaRow *row)
{
  return row->type;
}

const char *
schema_row_get_nullable (SchemaRow *row)
{
  return row->nullable;
}