#pragma once
#include <glib-object.h>

G_BEGIN_DECLS;

/*
 * Type declaration
 */
#define MG_TYPE_MANGA mg_manga_get_type()
G_DECLARE_FINAL_TYPE (MgManga, mg_manga, MG, MANGA, GObject)

/*
 * Method definitions.
 */
char *mg_manga_get_image_url(MgManga *mg_manga);
char *mg_manga_get_title(MgManga *mg_manga);

MgManga *mg_manga_new (const char *const image_url, const char *const title);

G_END_DECLS
