#include <sqlite3.h>
#include <glib-object.h>

#include <openmg/database.h>
#include <openmg/database/statement.h>

struct _MgDatabaseStatement {
    GObject parent_instance;
    MgDatabase *owner;
    sqlite3_stmt *stmt;
};

G_DEFINE_TYPE (MgDatabaseStatement, mg_database_statement, G_TYPE_OBJECT)

typedef enum {
    MG_DATABASE_STATEMENT_OWNER = 1,
    MG_DATABASE_STATEMENT_STMT,
    MG_DATABASE_STATEMENT_N_PROPERTIES
} MgDatabaseStatementProperties;

static GParamSpec
*database_statement_properties[MG_DATABASE_STATEMENT_N_PROPERTIES] = { NULL, };

static void
mg_database_statement_set_property (GObject *object,
        guint property_id,
        const GValue *value,
        GParamSpec *pspec);
static void
mg_database_statement_get_property (GObject *object,
        guint property_id,
        GValue *value,
        GParamSpec *pspec);
static void
mg_database_statement_dispose (GObject *object);

MgDatabaseStatement *
mg_database_statement_new (MgDatabase *owner, sqlite3_stmt *statement) {
    MgDatabaseStatement *self = NULL;
    self = MG_DATABASE_STATEMENT ((g_object_new (MG_TYPE_DATABASE_STATEMENT,
                    "owner", owner, "stmt", statement, NULL)));
    return self;
}

static void
mg_database_statement_class_init (MgDatabaseStatementClass *class) {
    GObjectClass *object_class = G_OBJECT_CLASS (class);
    object_class->set_property = mg_database_statement_set_property;
    object_class->get_property = mg_database_statement_get_property;
    object_class->dispose = mg_database_statement_dispose;
    database_statement_properties[MG_DATABASE_STATEMENT_OWNER] = g_param_spec_object (
            "owner",
            "Owner",
            "Owner MgDatabase.",
            MG_TYPE_DATABASE,
            G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE);
    database_statement_properties[MG_DATABASE_STATEMENT_STMT] = g_param_spec_pointer (
            "stmt",
            "STMT",
            "Statement pointer",
            G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE);
    g_object_class_install_properties
        (object_class, MG_DATABASE_STATEMENT_N_PROPERTIES,
         database_statement_properties);
}

static void
mg_database_statement_get_property (GObject *object,
        guint property_id,
        GValue *value,
        GParamSpec *pspec) {
    MgDatabaseStatement *self = MG_DATABASE_STATEMENT (object); 
    switch ((MgDatabaseStatementProperties) property_id) {
        case MG_DATABASE_STATEMENT_OWNER:
            g_value_set_instance (value, self->owner);
            break;
        case MG_DATABASE_STATEMENT_STMT:
            g_value_set_pointer (value, self->stmt);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
            break;
    }
}

static void
mg_database_statement_set_property (GObject *object,
        guint property_id,
        const GValue *value,
        GParamSpec *pspec) {
    MgDatabaseStatement *self = MG_DATABASE_STATEMENT (object);
    switch ((MgDatabaseStatementProperties) property_id) {
        case MG_DATABASE_STATEMENT_OWNER:
            if (self->owner) {
                g_clear_object (&(self->owner));
            }
            self->owner = g_value_peek_pointer (value);
            g_object_ref (self->owner);
            break;
        case MG_DATABASE_STATEMENT_STMT:
            if (self->stmt) {
                sqlite3_finalize (self->stmt);
            }
            self->stmt = g_value_get_pointer (value);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
            break;
    }
}

static sqlite3_stmt *
mg_database_statement_get_stmt (MgDatabaseStatement *self) {
    sqlite3_stmt *stmt;
    GValue value = G_VALUE_INIT;
    g_value_init (&value, G_TYPE_POINTER);
    g_object_get_property (G_OBJECT (self),
            "stmt",
            &value);
    stmt = g_value_get_pointer (&value);
    g_value_unset (&value);
    return stmt;
}

static void
mg_database_statement_dispose (GObject *object) {
    MgDatabaseStatement *self = MG_DATABASE_STATEMENT (object);
    if (self->owner) {
        g_clear_object (&(self->owner));
    }
    sqlite3_finalize (self->stmt);
}

int
mg_database_statement_bind_text (MgDatabaseStatement *self,
        int index, const char *value) {
    sqlite3_stmt *stmt = mg_database_statement_get_stmt (self);
    int error = sqlite3_bind_text (stmt, index, value, -1, SQLITE_TRANSIENT);
    if ( error != SQLITE_OK ) {
        g_error (mg_database_get_error_string (self->owner));
    }
    return error;
}

int
mg_database_statement_step (MgDatabaseStatement *self) {
    sqlite3_stmt *stmt = mg_database_statement_get_stmt (self);
    int error;
    while ((error = sqlite3_step (stmt)) == SQLITE_BUSY) {
    }
    if (error != SQLITE_DONE && error != SQLITE_ROW) {
        g_error (mg_database_get_error_string (self->owner));
    }
    return error;
}

const unsigned char *
mg_database_statement_column_text (MgDatabaseStatement *self,
        int i_col) {
    sqlite3_stmt *stmt = mg_database_statement_get_stmt (self);
    return sqlite3_column_text (stmt, i_col);
}

int
mg_database_statement_column_int (MgDatabaseStatement *self,
        int i_col) {
    sqlite3_stmt *stmt = mg_database_statement_get_stmt (self);
    return sqlite3_column_int (stmt, i_col);
}

static void
mg_database_statement_init (MgDatabaseStatement *self) {
}
