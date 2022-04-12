project('me.sergiotarxz.openmg', 'c')

inc = include_directories('include')

openmgdeps = [
    dependency('libadwaita-1'),
    dependency('gtk4'),
    dependency('libsoup-2.4'),
    dependency('libxml-2.0'),
    dependency('libpcre2-8'),
    dependency('gio-2.0'),
    dependency('sqlite3'),
    dependency('json-glib-1.0'),
]

sources = [
    'src/view/chapter_view.c',
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
    'src/database.c',
    'src/database/statement.c',
    'src/view/explore.c',
    'src/view/search.c',
    'src/main.c',
]

link_arguments = [
    '-lm'
]

images_on_lists = get_option('images')
is_windows = get_option('windows')

if not is_windows
    link_arguments += ['-ldl']
endif

cArgs = []
if images_on_lists
    cArgs += ['-DLIST_IMAGES']
endif

# Install our icons in all the required sizes
icon_sizes = ['16', '24', '32', '48', '64', '128']

foreach i : icon_sizes
    install_data(
        'me.sergiotarxz.openmg.svg',
        install_dir: get_option('datadir') / 'icons' / 'hicolor' / i + 'x' + i / 'apps',
        rename: meson.project_name() + '.svg'
    )
    install_data(
        'me.sergiotarxz.openmg.svg',
        install_dir: get_option('datadir') / 'icons' / 'hicolor' / i + 'x' + i + '@2' / 'apps',
        rename: meson.project_name() + '.svg'
    )
endforeach

install_data(
    'me.sergiotarxz.openmg.desktop',
    install_dir: get_option('datadir') / 'applications',
)

executable('openmg',
    sources,
    dependencies : openmgdeps,
    include_directories : inc,
    install : true,
    link_args : link_arguments,
    c_args: cArgs
)
