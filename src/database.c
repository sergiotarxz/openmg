#include <stdio.h>

#include <sqlite3.h>
#include <glib-object.h>
#include <glib.h>

#include <openmg/database/statement.h>
#include <openmg/database/migrations.h>
#include <openmg/database.h>

const char *const SQLITE_PATH_FORMAT_STRING =
"%s/.local/var/openmg.sqlite";
struct _MgDatabase {
    GObject parent_instance;
    sqlite3 *sqlite;
};

G_DEFINE_TYPE (MgDatabase, mg_database, G_TYPE_OBJECT)

typedef enum {
    MG_DATABASE_SQLITE = 1,
    MG_DATABASE_N_PROPERTIES
} MgDatabaseProperties;

static GParamSpec *database_properties[MG_DATABASE_N_PROPERTIES] = { NULL, };

static void
mg_database_set_property (GObject *object,
        guint property_id,
        const GValue *value,
        GParamSpec *pspec);
static void
mg_database_get_property (GObject *object,
        guint property_id,
        GValue *value,
        GParamSpec *pspec);

static void
mg_database_dispose (GObject *object);
static gboolean
mg_database_apply_single_migration (MgDatabase *self,
        sqlite3 *sqlite, int migration_number);
static int
mg_database_retrieve_next_migration (MgDatabase *self, sqlite3 *sqlite);
static gboolean
mg_database_register_next_migration (MgDatabase *self, sqlite3 *sqlite,
        int next_migration);
static void
mg_database_apply_migrations (MgDatabase *self, sqlite3 *sqlite);

MgDatabase *
mg_database_new (void) {
    MgDatabase *self = NULL;
    sqlite3 *sqlite_handle = NULL;
    const char *home_dir = g_get_home_dir();
    size_t database_path_len;
    char *database_path;
    char *database_path_directory;

    database_path_len = snprintf
        (NULL, 0, SQLITE_PATH_FORMAT_STRING,
         home_dir);
    database_path = g_malloc
        (sizeof *database_path
         * (database_path_len + 1));
    snprintf (database_path, database_path_len,
            SQLITE_PATH_FORMAT_STRING,
            home_dir);
    database_path_directory = g_path_get_dirname
        (database_path);
    // TODO: Control the possible error.
    g_mkdir_with_parents
        (database_path_directory, 00700);

    sqlite3_open (database_path, &sqlite_handle);
    g_free (database_path);
    self = MG_DATABASE ((g_object_new (MG_TYPE_DATABASE,
                    "sqlite", sqlite_handle, NULL)));
    return self;
}

static void
mg_database_class_init (MgDatabaseClass *class) {
    GObjectClass *object_class = G_OBJECT_CLASS (class);
    object_class->set_property = mg_database_set_property;
    object_class->get_property = mg_database_get_property;
    object_class->dispose = mg_database_dispose;
    database_properties[MG_DATABASE_SQLITE] = g_param_spec_pointer (
            "sqlite",
            "SQLite",
            "SQLite handle.",
            G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE);
    g_object_class_install_properties
        (object_class, MG_DATABASE_N_PROPERTIES,
         database_properties);
}

static void
mg_database_set_property (GObject *object,
        guint property_id,
        const GValue *value,
        GParamSpec *pspec) {
    MgDatabase *self = MG_DATABASE (object);
    switch ((MgDatabaseProperties) property_id) {
        case MG_DATABASE_SQLITE:
            if (self->sqlite) {
                sqlite3_close (self->sqlite);
            }
            self->sqlite = g_value_get_pointer (value);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
            break;
    }
}

static void
mg_database_get_property  (GObject *object,
        guint property_id,
        GValue *value,
        GParamSpec *pspec) {
    MgDatabase *self = MG_DATABASE (object);
    switch ((MgDatabaseProperties) property_id) {
        case MG_DATABASE_SQLITE:
            g_value_set_pointer (value, self->sqlite);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
            break;
    }
}

static void
mg_database_init (MgDatabase *self) {
}

static sqlite3 *
mg_database_get_sqlite (MgDatabase *self) {
    sqlite3 *sqlite;
    GValue value = G_VALUE_INIT;
    g_value_init (&value, G_TYPE_POINTER);
    g_object_get_property (G_OBJECT (self),
            "sqlite",
            &value);
    sqlite = g_value_get_pointer (&value);
    g_value_unset (&value);
    mg_database_apply_migrations (self, sqlite);
    return sqlite;
}

static void
mg_database_apply_migrations (MgDatabase *self, sqlite3 *sqlite) {
    size_t migrations_len = sizeof MIGRATIONS / sizeof *MIGRATIONS;
    int next_migration = mg_database_retrieve_next_migration (self, sqlite);
    for (int i = next_migration; i < migrations_len; i++) {
        if (!mg_database_apply_single_migration (self, sqlite, next_migration)) {
            return;
        }
        next_migration++;
        mg_database_register_next_migration (self, sqlite, next_migration);
    }
}

/*
 * Returns affected rows.
 */
static int
mg_database_register_next_migration_atttempt_insert (MgDatabase *self, sqlite3 *sqlite, int next_migration) {
    sqlite3_stmt *statement = NULL;
    int return_value = 0;

    int error = sqlite3_prepare_v2 (sqlite, "insert into options (key, value) values (?, ?);",
            -1, &statement, NULL);
    if (error != SQLITE_OK) {
        g_warning (sqlite3_errmsg (sqlite));
        goto cleanup_mg_database_register_next_migration_attempt_insert;
    }
    error = sqlite3_bind_text (statement, 1, "migration", -1, SQLITE_TRANSIENT);
    if (error != SQLITE_OK) {
        g_warning (sqlite3_errmsg (sqlite));
        goto cleanup_mg_database_register_next_migration_attempt_insert;
    }
    error = sqlite3_bind_int (statement, 2, next_migration);
    if (error != SQLITE_OK) {
        g_warning (sqlite3_errmsg (sqlite));
        goto cleanup_mg_database_register_next_migration_attempt_insert;
    }
    error = sqlite3_step (statement);
    if (error != SQLITE_DONE) {
        g_warning (sqlite3_errmsg (sqlite));
        goto cleanup_mg_database_register_next_migration_attempt_insert;
    }
    return_value = sqlite3_changes (sqlite);
cleanup_mg_database_register_next_migration_attempt_insert:
    sqlite3_finalize (statement);
    return return_value;
}

static int
mg_database_register_next_migration_update (MgDatabase *self, sqlite3 *sqlite,
        int next_migration) {
    int return_value = 0;
    sqlite3_stmt *statement = NULL;
    int error = sqlite3_prepare_v2 (sqlite, "update options set value = ? where key = ?;",
            -1, &statement, NULL);
    if (error != SQLITE_OK) {
        g_warning (sqlite3_errmsg (sqlite));
        goto cleanup_mg_database_register_next_migration_update;
    }
    error = sqlite3_bind_int (statement, 1, next_migration);
    if (error != SQLITE_OK) {
        g_warning (sqlite3_errmsg (sqlite));
        goto cleanup_mg_database_register_next_migration_update;
    }
    error = sqlite3_bind_text (statement, 2, "migration", -1, SQLITE_TRANSIENT);
    if (error != SQLITE_OK) {
        g_warning (sqlite3_errmsg (sqlite));
        goto cleanup_mg_database_register_next_migration_update;
    }
    error = sqlite3_step (statement);
    if (error != SQLITE_DONE) {
        g_warning (sqlite3_errmsg (sqlite));
        goto cleanup_mg_database_register_next_migration_update;
    }
    return_value = sqlite3_changes (sqlite);
cleanup_mg_database_register_next_migration_update:
    sqlite3_finalize (statement);
    return return_value;
}

static gboolean
mg_database_register_next_migration (MgDatabase *self, sqlite3 *sqlite, int next_migration) {
    int affected_rows =  mg_database_register_next_migration_atttempt_insert
        (self, sqlite, next_migration);
    if (affected_rows) {
        return 1;
    }
    return !!mg_database_register_next_migration_update (self, sqlite, next_migration);
}

static int
mg_database_retrieve_next_migration (MgDatabase *self, sqlite3 *sqlite) {
    int next_migration = 0;
    int error;
    sqlite3_stmt *statement = NULL;
    error = sqlite3_prepare_v2 (sqlite, "select value from options where key = ?;", -1, &statement, NULL);
    if (error != SQLITE_OK) {
        g_warning (sqlite3_errmsg (sqlite));
        goto cleanup_mg_database_retrieve_next_migration;
    }
    error = sqlite3_bind_text (statement, 1, "migration", -1, SQLITE_TRANSIENT);
    if (error != SQLITE_OK) {
        g_warning (sqlite3_errmsg (sqlite));
        goto cleanup_mg_database_retrieve_next_migration;
    }
    error = sqlite3_step (statement);
    if (error == SQLITE_ROW) {
        next_migration = sqlite3_column_int (statement, 0);
    }
cleanup_mg_database_retrieve_next_migration:
    sqlite3_finalize (statement);
    return next_migration;
}

static gboolean
mg_database_apply_single_migration (MgDatabase *self, sqlite3 *sqlite, int migration_number) {
    sqlite3_stmt *statement = NULL;
    const char *current_migration = MIGRATIONS[migration_number];
    gboolean return_value = 0;
    int error = sqlite3_prepare_v2 (sqlite, current_migration, -1, &statement, NULL);
    if (error != SQLITE_OK) {
        g_warning (sqlite3_errmsg (sqlite));
        goto cleanup_mg_database_apply_single_migration;
    }
    error = sqlite3_step (statement);
    if (error != SQLITE_DONE) {
        g_warning (sqlite3_errmsg (sqlite));
        goto cleanup_mg_database_apply_single_migration;
    }
    return_value = 1;
cleanup_mg_database_apply_single_migration:
    if (statement) {
        sqlite3_finalize (statement);
    }
    return return_value;
}

const char *
mg_database_get_error_string (MgDatabase *self) {
    sqlite3 *sqlite = mg_database_get_sqlite (self);
    return sqlite3_errmsg (sqlite);
}

static void
mg_database_dispose (GObject *object) {
    MgDatabase *self = MG_DATABASE (object);
    sqlite3_close (self->sqlite);
}

MgDatabaseStatement *
mg_database_prepare (MgDatabase *self, char *z_sql, const char **pz_tail) {
    sqlite3_stmt *statement = NULL;
    sqlite3 *sqlite = mg_database_get_sqlite (self);
    int error = sqlite3_prepare_v2 (sqlite, z_sql, -1, &statement, pz_tail);
    if (error != SQLITE_OK) {
        g_warning (sqlite3_errmsg (sqlite));
        return NULL;
    }
    return mg_database_statement_new (self, statement);
}

int
mg_database_get_affected_rows (MgDatabase *self) {
    sqlite3 *sqlite = mg_database_get_sqlite (self);
    return sqlite3_changes (sqlite);
}
