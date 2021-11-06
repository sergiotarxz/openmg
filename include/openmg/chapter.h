#pragma once
#include <glib-object.h>

G_BEGIN_DECLS;

#define MG_TYPE_MANGA_CHAPTER mg_manga_chapter_get_type()
G_DECLARE_FINAL_TYPE (MgMangaChapter, mg_manga_chapter, MG, MANGA_CHAPTER, GObject)

MgMangaChapter *
mg_manga_chapter_new (const char *const title,
        const char *const published_text,
        const char *const url);
char *
mg_manga_chapter_get_title (MgMangaChapter *self);

G_END_DECLS
