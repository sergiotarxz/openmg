#include <glib-object.h>

void
g_object_set_property_int(GObject *object, char *property_key, gint value) {
    GValue property = G_VALUE_INIT;
    g_value_init (&property, G_TYPE_INT);
    g_value_set_int (&property, value);
    g_object_set_property (object, property_key, &property);
}


void
g_object_set_property_double(GObject *object, char *property_key, gdouble value) {
    GValue property = G_VALUE_INIT;
    g_value_init (&property, G_TYPE_DOUBLE);
    g_value_set_double (&property, value);
    g_object_set_property (object, property_key, &property);
}
