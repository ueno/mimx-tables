ACLOCAL_AMFLAGS = -I m4
moduledir = $(M17N_MODULE_DIR)
module_LTLIBRARIES = libmimx-table.la

libmimx_table_la_SOURCES = mimx-table.c
libmimx_table_la_CFLAGS = $(M17N_CFLAGS) $(SQLITE3_CFLAGS)
libmimx_table_la_LIBADD = $(M17N_LIBS) $(SQLITE3_LIBS)
libmimx_table_la_LDFLAGS = -avoid-version -module

mimdir = $(datadir)/m17n
mim_DATA =					\
	latex.mim				\
	mr-inscript-typing-booster.mim		\
	mr-phonetic-typing-booster.mim
dist_mim_DATA = \
	table-util.mim
DISTCLEANFILES = $(mim_DATA)

.mim.in.mim:
	$(AM_V_GEN) sed 's!@ibus_table_dir''@!'$(ibus_table_dir)'!g' $< > $@
SUFFIXES = .mim.in .mim

EXTRA_DIST = 					\
	latex.mim.in				\
	mr-inscript-typing-booster.mim.in	\
	mr-phonetic-typing-booster.mim.in
