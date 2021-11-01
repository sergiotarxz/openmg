#include <glib-object.h>

void
g_object_set_property_int(GObject *object, char *property_key, int value) {
    GValue property = G_VALUE_INIT;
    g_value_init (&property, G_TYPE_INT);
    g_value_set_int (&property, value);
    g_object_set_property (object, property_key, &property);
}


