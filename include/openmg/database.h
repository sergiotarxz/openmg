#pragma once
#include <glib-object.h>

#include <openmg/database/statement.h>

G_BEGIN_DECLS;

#define MG_TYPE_DATABASE mg_database_get_type()
G_DECLARE_FINAL_TYPE (MgDatabase, mg_database, MG, DATABASE, GObject)

MgDatabase *mg_database_new ();

MgDatabaseStatement *
mg_database_prepare (MgDatabase *self, char *z_sql, const char **pz_tail);

int
mg_database_get_affected_rows (MgDatabase *self);

char *
mg_database_get_error_string (MgDatabase *self);

G_END_DECLS

