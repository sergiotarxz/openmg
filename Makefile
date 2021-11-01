CC := gcc
LIBS := libadwaita-1 gtk4 libsoup-2.4 libxml-2.0 libpcre2-8 gio-2.0
INCDIR := -I ./include
CFLAGS := $(shell pkg-config --cflags ${LIBS}) -Wall
LDFLAGS := $(shell pkg-config --libs ${LIBS}) 
CC_COMMAND := ${CC} ${INCDIR} ${CFLAGS}
all: build
build:
	${CC_COMMAND} src/view/picture.c src/util/gobject_utility_extensions.c src/view/detail_manga.c src/util/regex.c src/util/string.c src/util/xml.c src/util/soup.c src/view/list_view_manga.c src/view/main_view.c src/manga.c src/backend/readmng.c src/main.c -o main ${LDFLAGS} -ggdb
