#include "generic-row.h"

struct _GenericRow
{
  GObject parent_instance;

  int n_columns;
  char **values;
};

G_DEFINE_TYPE (GenericRow, generic_row, G_TYPE_OBJECT)

static void
generic_row_dispose (GObject *object)
{
  GenericRow *row = GENERIC_ROW (object);

  if (row->values)
    {
      for (int i = 0; i < row->n_columns; i++)
        g_free (row->values[i]);

      g_free (row->values);
    }

  G_OBJECT_CLASS (generic_row_parent_class)->dispose (object);
}

static void
generic_row_class_init (GenericRowClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = generic_row_dispose;
}

static void
generic_row_init (GenericRow *self)
{
  self->n_columns = 0;
  self->values = NULL;
}

GenericRow *
generic_row_new (int n_columns)
{
  GenericRow *row = g_object_new (TYPE_GENERIC_ROW, NULL);

  row->n_columns = n_columns;
  row->values = g_new0 (char *, n_columns);

  return row;
}

void
generic_row_set_value (GenericRow *row, int index, const char *value)
{
  if (index >= row->n_columns)
    return;

  row->values[index] = g_strdup (value);
}

const char *
generic_row_get_value (GenericRow *row, int index)
{
  if (index >= row->n_columns)
    return "";

  return row->values[index];
}

int
generic_row_get_n_columns (GenericRow *row)
{
  return row->n_columns;
}