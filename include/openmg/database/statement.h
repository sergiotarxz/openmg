#pragma once
#include <glib-object.h>

G_BEGIN_DECLS;

#define MG_TYPE_DATABASE_STATEMENT mg_database_statement_get_type()
G_DECLARE_FINAL_TYPE (MgDatabaseStatement, mg_database_statement, MG, DATABASE_STATEMENT, GObject)

MgDatabaseStatement *mg_database_statement_new ();

void
mg_database_statement_bind_text (MgDatabaseStatement *self, int index, char *value);

G_END_DECLS

