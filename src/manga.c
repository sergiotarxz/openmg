#include <stdio.h>

#include <glib-object.h>
#include <gio/gio.h>

#include <openmg/util/string.h>
#include <openmg/manga.h>
#include <openmg/chapter.h>

struct _MgManga {
    GObject parent_instance;
    char *image_url;
    char *title;
    char *id;
    char *description;
    GListStore *chapter_list;
    int has_details;
};

G_DEFINE_TYPE (MgManga, mg_manga, G_TYPE_OBJECT)

typedef enum {
    MG_MANGA_IMAGE_URL = 1,
    MG_MANGA_TITLE,
    MG_MANGA_ID,
    MG_MANGA_DESCRIPTION,
    MG_MANGA_CHAPTER_LIST,
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
mg_manga_dispose (GObject *object);

static void
mg_manga_class_init (MgMangaClass *class) {
    GObjectClass *object_class = G_OBJECT_CLASS (class);
    object_class->set_property = mg_manga_set_property;
    object_class->get_property = mg_manga_get_property;
    object_class->dispose = mg_manga_dispose;

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
    manga_properties[MG_MANGA_ID] = g_param_spec_string ("id",
            "Id",
            "Id of the manga.",
            NULL,
            G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE);
    manga_properties[MG_MANGA_DESCRIPTION] = g_param_spec_string (
            "description",
            "Description",
            "Description of the manga.",
            NULL,
            G_PARAM_READWRITE);
    manga_properties[MG_MANGA_CHAPTER_LIST] = g_param_spec_object (
            "chapter_list",
            "ChapterList",
            "List of chapters.",
            G_TYPE_LIST_STORE,
            G_PARAM_READWRITE);


    g_object_class_install_properties (object_class,
            MG_MANGA_N_PROPERTIES,
            manga_properties);
}

static void
mg_manga_init (MgManga *self) {
    self->has_details = 0;
}

static void
mg_manga_dispose (GObject *object) {
    MgManga *self = MG_MANGA (object);
    if (self->description) {
        g_free (self->description);
        self->description = NULL;
    }
    if (self->title) {
        g_free (self->title);
        self->title = NULL;
    }
    if (self->id) {
        g_free (self->id);
        self->id = NULL;
    }
     if (self->image_url) {
        g_free (self->image_url);
        self->image_url = NULL;
    }

    g_clear_object (&(self->chapter_list));
}

int
mg_manga_has_details (MgManga *self) {
    return self->has_details;
}

void
mg_manga_details_recovered (MgManga *self) {
    self->has_details = 1;
}

char *
mg_manga_get_id (MgManga *self) {
    GValue value = G_VALUE_INIT;
    char *return_value = NULL;
    g_value_init (&value, G_TYPE_STRING);
    g_object_get_property (G_OBJECT (self),
            "id",
            &value);
    return_value = g_value_dup_string (&value);
    g_value_unset (&value);
    return return_value;
}

char *
mg_manga_get_image_url (MgManga *self) {
    GValue value = G_VALUE_INIT;
    char *return_value = NULL;
    g_value_init (&value, G_TYPE_STRING);
    g_object_get_property (G_OBJECT (self),
            "image_url",
            &value);
    return_value = g_value_dup_string (&value);
    g_value_unset (&value);
    return return_value;
}

char *
mg_manga_get_title (MgManga *self) {
    GValue value = G_VALUE_INIT;
    char *return_value = NULL;
    g_value_init (&value, G_TYPE_STRING);
    g_object_get_property (G_OBJECT (self),
            "title",
            &value);
    return_value = g_value_dup_string (&value);
    g_value_unset (&value);
    return return_value;
}

char *
mg_manga_get_description (MgManga *self) {
    GValue value = G_VALUE_INIT;
    char *result;
    g_value_init (&value, G_TYPE_STRING);
    g_object_get_property (G_OBJECT (self),
            "description",
            &value);
    result = g_value_dup_string (&value);
    g_value_unset (&value);
    return result;
}

GListStore *
mg_manga_get_chapter_list (MgManga *self) {
    if (!mg_manga_has_details (self)) {
        fprintf(stderr, "Manga has still not details\n");
        return NULL;
    }
    GValue value = G_VALUE_INIT;
    GListStore *return_value;

    g_value_init (&value, G_TYPE_LIST_STORE);
    g_object_get_property (G_OBJECT (self),
            "chapter_list",
            &value);
    return_value = G_LIST_STORE (g_value_peek_pointer (&value));
    g_object_ref (G_OBJECT (return_value));
    g_value_unset (&value);

    return return_value;
}

void
mg_manga_set_description (MgManga *self, const char *description) {
    GValue value = G_VALUE_INIT;
    g_value_init (&value, G_TYPE_STRING);
    g_value_set_string (&value, description);
    g_object_set_property (G_OBJECT (self), "description", &value);
    g_value_unset (&value);
}

void
mg_manga_set_chapter_list (MgManga *self, GListStore *chapter_list) {
    GValue value = G_VALUE_INIT;
    g_object_ref (G_OBJECT (chapter_list));
    g_value_init (&value, G_TYPE_LIST_STORE);
    g_value_set_instance (&value, chapter_list);
    g_object_set_property (G_OBJECT (self), "chapter_list", &value);
    g_value_unset (&value);
}

static void
mg_manga_set_property (GObject *object,
        guint property_id,
        const GValue *value,
        GParamSpec *pspec) {
    MgManga *self = MG_MANGA (object);
    GListStore *chapter_list;
    switch ((MgMangaProperties) property_id) {
        case MG_MANGA_IMAGE_URL:
            g_free (self->image_url);
            self->image_url = g_value_dup_string (value);
            break;
        case MG_MANGA_TITLE:
            g_free (self->title);
            self->title = g_value_dup_string (value);
            break;
        case MG_MANGA_ID:
            g_free (self->id);
            self->id = g_value_dup_string (value);
            break;
        case MG_MANGA_DESCRIPTION:
            g_free (self->description);
            self->description = g_value_dup_string (value);
            break;
        case MG_MANGA_CHAPTER_LIST:
            g_free (self->chapter_list);
            chapter_list = g_value_peek_pointer (value);
            self->chapter_list = chapter_list;
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
        case MG_MANGA_ID:
            g_value_set_string (value, self->id);
            break;
        case MG_MANGA_DESCRIPTION:
            g_value_set_string (value, self->description);
            break;
        case MG_MANGA_CHAPTER_LIST:
            g_value_set_instance (value, self->chapter_list);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
            break;
    }
}

MgManga *
mg_manga_new (const char *const image_url, const char *const title, const char *id) {
    MgManga *self = NULL;
    MgUtilString *string_util = mg_util_string_new ();
    self = MG_MANGA ((g_object_new (MG_TYPE_MANGA, NULL)));
    self->image_url = mg_util_string_alloc_string (string_util,
            strlen (image_url));
    self->title = mg_util_string_alloc_string (string_util,
            strlen (title));
    self->id = mg_util_string_alloc_string (string_util,
            strlen (id));
    mg_util_string_copy_substring (string_util,
            image_url, self->image_url,
            strlen(image_url) + 1, 0, strlen (image_url));
    mg_util_string_copy_substring (string_util,
            title, self->title, strlen(title) + 1, 0, strlen (title));
    mg_util_string_copy_substring (string_util,
            id, self->id, strlen(id) + 1, 0, strlen (id));
    g_clear_object (&string_util);
    return self;
}
