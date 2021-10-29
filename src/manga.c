#include <glib-object.h>

#include <manga.h>
#include <openmg/manga.h>

struct _MgManga {
    GObject parent_instance;
    char *image_url;
    char *title;
};

G_DEFINE_TYPE (MgManga, mg_manga, G_TYPE_OBJECT)

typedef enum {
    MG_MANGA_IMAGE_URL = 1,
    MG_MANGA_TITLE,
    MG_MANGA_N_PROPERTIES
} MgMangaProperties;

static GParamSpec *manga_properties[MG_MANGA_N_PROPERTIES] = { NULL, };

static void
mg_manga_set_property (GObject *object,
        guint property_id,
        const GValue *value,
        GParamSpec *pspec);
static void
mg_manga_get_property (GObject *object,
        guint property_id,
        GValue *value,
        GParamSpec *pspec);

static void
mg_manga_class_init (MgMangaClass *class) {
    GObjectClass *object_class = G_OBJECT_CLASS (class);
    object_class->set_property = mg_manga_set_property;
    object_class->get_property = mg_manga_get_property;

    manga_properties[MG_MANGA_IMAGE_URL] = g_param_spec_string ("image_url",
            "ImageURL",
            "Url of the image.",
            NULL,
            G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE);
    manga_properties[MG_MANGA_TITLE] = g_param_spec_string ("title",
            "Title",
            "Title of the manga.",
            NULL,
            G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE);

    g_object_class_install_properties (object_class,
            MG_MANGA_N_PROPERTIES,
            manga_properties);
}

static void
mg_manga_init (MgManga *self) {
}

char *
mg_manga_get_image_url(MgManga *self) {
    GValue value = G_VALUE_INIT;
    g_value_init (&value, G_TYPE_STRING);
    g_object_get_property (G_OBJECT (self),
            "image_url",
            &value);
    return g_value_dup_string (&value);
}

char *
mg_manga_get_title(MgManga *self) {
    GValue value = G_VALUE_INIT;
    g_value_init (&value, G_TYPE_STRING);
    g_object_get_property (G_OBJECT (self),
            "title",
            &value);
    return g_value_dup_string (&value);
}

static void
mg_manga_set_property (GObject *object,
        guint property_id,
        const GValue *value,
        GParamSpec *pspec) {
    MgManga *self = MG_MANGA (object);
    switch ((MgMangaProperties) property_id) {
        case MG_MANGA_IMAGE_URL:
            g_free (self->image_url);
            self->image_url = g_value_dup_string (value);
            break;
        case MG_MANGA_TITLE:
            g_free (self->title);
            self->title = g_value_dup_string (value);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
            break;
    }
}

static void
mg_manga_get_property (GObject *object,
        guint property_id,
        GValue *value,
        GParamSpec *pspec) {
    MgManga *self = MG_MANGA (object);
    switch ((MgMangaProperties) property_id) {
        case MG_MANGA_IMAGE_URL:
            g_value_set_string (value, self->image_url);
            break;
        case MG_MANGA_TITLE:
            g_value_set_string (value, self->title);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
            break;
    }
}

MgManga *
mg_manga_new (const char *const image_url, const char *const title) {
    MgManga *self = NULL;
    self = (MG_MANGA) (g_object_new (MG_TYPE_MANGA, NULL));
    self->image_url = alloc_string (strlen (image_url));
    self->title = alloc_string (strlen (title));
    copy_substring (image_url, self->image_url, strlen(image_url) + 1, 0, strlen (image_url));
    copy_substring (title, self->title, strlen(title) + 1, 0, strlen (title));
    return self;
}