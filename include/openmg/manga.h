#pragma once
#include <glib-object.h>

G_BEGIN_DECLS;

#define MG_TYPE_MANGA mg_manga_get_type()
G_DECLARE_FINAL_TYPE (MgManga, mg_manga, MG, MANGA, GObject)

char *
mg_manga_get_image_url (MgManga *mg_manga);
char *
mg_manga_get_title (MgManga *mg_manga);
char *
mg_manga_get_id (MgManga *mg_manga);
char *
mg_manga_get_id (MgManga *mg_manga);
char *
mg_manga_get_description (MgManga *mg_manga);
void
mg_manga_set_description (MgManga *mg_manga, const char *description);
int
mg_manga_has_details (MgManga *self);
void
mg_manga_details_recovered (MgManga *self);

MgManga *mg_manga_new (const char *const image_url, const char *const title, const char *const id);

G_END_DECLS
