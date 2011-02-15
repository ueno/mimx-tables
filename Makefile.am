ACLOCAL_AMFLAGS = -I m4
moduledir = $(M17N_MODULE_DIR)
module_LTLIBRARIES = libmimx-ibus-table.la
libmimx_ibus_table_la_SOURCES = mimx-ibus-table.c
libmimx_ibus_table_la_CFLAGS = $(M17N_CFLAGS) $(SQLITE3_CFLAGS)
libmimx_ibus_table_la_LIBADD = $(M17N_LIBS) $(SQLITE3_LIBS)
libmimx_ibus_table_la_LDFLAGS = -avoid-version -module
EXTRA_DIST =					\
	latex.mim				\
	mr-inscript-typing-booster.mim		\
	mr-phonetic-typing-booster.mim
