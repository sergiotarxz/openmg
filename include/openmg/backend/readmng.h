#pragma once

#include <glib-object.h>

#include <gio/gio.h>

#include <openmg/manga.h>
#include <openmg/chapter.h>

G_BEGIN_DECLS;

/*
 * Type declaration
 */
#define MG_TYPE_BACKEND_READMNG mg_backend_readmng_get_type ()
G_DECLARE_FINAL_TYPE (MgBackendReadmng, mg_backend_readmng, MG, BACKEND_READMNG, GObject)

void
mg_backend_readmng_set_property (GObject *object, guint property_id, 
        const GValue *value, GParamSpec *pspec);
void
mg_backend_readmng_get_property (GObject *object, guint property_id, 
        GValue *value, GParamSpec *pspec);
/*
 * Method definitions.
 */
MgBackendReadmng *
mg_backend_readmng_new (void);

GListStore *
mg_backend_readmng_get_featured_manga (MgBackendReadmng *self);

GListStore *
mg_backend_readmng_search (MgBackendReadmng *self,
        const char *search_query);

void
mg_backend_readmng_retrieve_manga_details (MgBackendReadmng *self,
        MgManga *manga);
GListModel *
mg_backend_readmng_get_chapter_images (MgBackendReadmng *self, MgMangaChapter *chapter);
GListStore *
mg_backend_readmng_search (MgBackendReadmng *self,
        const char *search_query);

G_END_DECLS
