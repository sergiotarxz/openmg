project('openmg', 'c')

inc = include_directories('include')

openmgdeps = [
    dependency('libadwaita-1'),
    dependency('gtk4'),
    dependency('libsoup-2.4'),
    dependency('libxml-2.0'),
    dependency('libpcre2-8'),
    dependency('gio-2.0')
]

sources = [
    'src/view/list_view_chapter.c',
    'src/view/picture.c',
    'src/util/gobject_utility_extensions.c',
    'src/view/detail_manga.c',
    'src/util/regex.c',
    'src/util/string.c',
    'src/util/xml.c',
    'src/util/soup.c',
    'src/view/list_view_manga.c',
    'src/view/main_view.c',
    'src/manga.c',
    'src/chapter.c',
    'src/backend/readmng.c',
    'src/main.c',
]
                        
executable('openmg',
    sources,
    dependencies : openmgdeps,
    include_directories : inc,
    install : true
)
