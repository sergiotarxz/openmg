#pragma once
#include <glib-object.h>

G_BEGIN_DECLS;

#define MG_TYPE_DATABASE_STATEMENT mg_database_statement_get_type()
G_DECLARE_FINAL_TYPE (MgDatabaseStatement, mg_database_statement, MG, DATABASE_STATEMENT, GObject)

MgDatabaseStatement *mg_database_statement_new ();

int
mg_database_statement_bind_text (MgDatabaseStatement *self,
        int index, const char *value);
int
mg_database_statement_step (MgDatabaseStatement *self);

const unsigned char *
mg_database_statement_column_text (MgDatabaseStatement *self,
        int i_col);

int
mg_database_statement_column_int (MgDatabaseStatement *self,
        int i_col);

G_END_DECLS

