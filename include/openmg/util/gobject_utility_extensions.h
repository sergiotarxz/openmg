#pragma once
#include <glib-object.h>

void
g_object_set_property_int(GObject *object, char *property_key, gint value);
void
g_object_set_property_double(GObject *object, char *property_key, gdouble value);
