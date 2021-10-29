CC := gcc
LIBS := libadwaita-1 gtk4 libsoup-2.4 libxml-2.0 libpcre2-8 gio-2.0
INCDIR := -I ./include
CFLAGS := $(shell pkg-config --cflags ${LIBS}) -Wall
LDFLAGS := $(shell pkg-config --libs ${LIBS}) 
CC_COMMAND := ${CC} ${INCDIR} ${CFLAGS}
all: build
build:
	${CC_COMMAND} src/view/list_view_manga.c src/view/main_view.c src/manga.c src/backend/readmng.c manga.c main.c -o main ${LDFLAGS} -ggdb
