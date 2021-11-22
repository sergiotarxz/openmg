#include <glib-object.h>

#include <openmg/chapter.h>

#include <openmg/util/string.h>

struct _MgMangaChapter {
    GObject parent_instance;
    char *title;
    char *url;
    char *published_text;
};

static void
mg_manga_chapter_class_init (MgMangaChapterClass *class);
static void
mg_manga_chapter_class_init (MgMangaChapterClass *class);
static void
mg_manga_chapter_get_property (GObject *object,
        guint property_id,
        GValue *value,
        GParamSpec *pspec);
static void
mg_manga_chapter_set_property (GObject *object,
        guint property_id,
        const GValue *value,
        GParamSpec *pspec);

G_DEFINE_TYPE (MgMangaChapter, mg_manga_chapter,
        G_TYPE_OBJECT)

typedef enum {
    MG_MANGA_CHAPTER_TITLE = 1,
    MG_MANGA_CHAPTER_URL,
    MG_MANGA_CHAPTER_PUBLISHED_TEXT,
    MG_MANGA_CHAPTER_N_PROPERTIES
} MgMangaChapterProperties;

static GParamSpec *manga_chapter_properties[MG_MANGA_CHAPTER_N_PROPERTIES] = { NULL, };


MgMangaChapter *
mg_manga_chapter_new (const char *const title,
        const char *const published_text,
        const char *const url) {
    MgMangaChapter *self = NULL;
    MgUtilString *string_util = mg_util_string_new ();

    self = MG_MANGA_CHAPTER (
            g_object_new (MG_TYPE_MANGA_CHAPTER, NULL));

    self->title = mg_util_string_alloc_string (
            string_util, strlen (title));
    self->url = mg_util_string_alloc_string (
            string_util, strlen (url));
    self->published_text = mg_util_string_alloc_string (
            string_util, strlen (published_text));

    mg_util_string_copy_substring (string_util,
            title, self->title,
            strlen(title) + 1, 0, strlen (title));
    mg_util_string_copy_substring (string_util,
            url, self->url,
            strlen(url) + 1, 0, strlen (url));
    mg_util_string_copy_substring (string_util,
            published_text, self->published_text,
            strlen(published_text) + 1, 0,
            strlen (published_text));

    g_clear_object (&string_util);
    return self;
}

static void
mg_manga_chapter_class_init (MgMangaChapterClass *class) {
    GObjectClass *object_class = G_OBJECT_CLASS (class);
    object_class->set_property = mg_manga_chapter_set_property;
    object_class->get_property = mg_manga_chapter_get_property;

    manga_chapter_properties[MG_MANGA_CHAPTER_TITLE] = 
        g_param_spec_string ("title",
                "Title",
                "Title of the chapter.",
                NULL,
                G_PARAM_CONSTRUCT_ONLY 
                | G_PARAM_READWRITE);
    manga_chapter_properties[MG_MANGA_CHAPTER_URL] = 
        g_param_spec_string ("url",
                "URL",
                "URL of the chapter.",
                NULL,
                G_PARAM_CONSTRUCT_ONLY 
                | G_PARAM_READWRITE);
    manga_chapter_properties[MG_MANGA_CHAPTER_PUBLISHED_TEXT] = 
        g_param_spec_string ("published_text",
                "PublishedText",
                "Text of publication.",
                NULL,
                G_PARAM_CONSTRUCT_ONLY 
                | G_PARAM_READWRITE);
    g_object_class_install_properties (object_class,
            MG_MANGA_CHAPTER_N_PROPERTIES,
            manga_chapter_properties);
}

static void
mg_manga_chapter_init (MgMangaChapter *self) {
}

static void
mg_manga_chapter_get_property (GObject *object,
        guint property_id,
        GValue *value,
        GParamSpec *pspec) {
    MgMangaChapter *self = MG_MANGA_CHAPTER (object);
    switch ((MgMangaChapterProperties) property_id) {
        case MG_MANGA_CHAPTER_TITLE:
            g_value_set_string (value, self->title);
            break;
        case MG_MANGA_CHAPTER_URL:
            g_value_set_string (value, self->url);
            break;
        case MG_MANGA_CHAPTER_PUBLISHED_TEXT:
            g_value_set_string (value, self->published_text);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
            break;
    }
}

static void
mg_manga_chapter_set_property (GObject *object,
        guint property_id,
        const GValue *value,
        GParamSpec *pspec) {
    MgMangaChapter *self = MG_MANGA_CHAPTER (object);
    switch ((MgMangaChapterProperties) property_id) {
        case MG_MANGA_CHAPTER_TITLE:
            g_free (self->title);
            self->title = g_value_dup_string (value);
            break;
        case MG_MANGA_CHAPTER_URL:
            g_free (self->url);
            self->url = g_value_dup_string (value);
            break;
        case MG_MANGA_CHAPTER_PUBLISHED_TEXT:
            g_free (self->published_text);
            self->published_text = g_value_dup_string (value);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
            break;
    }
}

char *
mg_manga_chapter_get_title (MgMangaChapter *self) {
    GValue value = G_VALUE_INIT;
    char *return_value = NULL;
    g_value_init (&value, G_TYPE_STRING);
    g_object_get_property (G_OBJECT (self), "title", &value);
    return_value = g_value_dup_string (&value);
    g_value_unset (&value);
    return return_value;
}

char *
mg_manga_chapter_get_url (MgMangaChapter *self) {
    GValue value = G_VALUE_INIT;
    char *return_value = NULL;
    g_value_init (&value, G_TYPE_STRING);
    g_object_get_property (G_OBJECT (self), "url", &value);
    return_value = g_value_dup_string (&value);
    g_value_unset (&value);
    return return_value;
}
