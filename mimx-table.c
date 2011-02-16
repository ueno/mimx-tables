/* mimx-table.c -- ibus-table input method external module for m17n-lib
 * Copyright (C) 2011 Daiki Ueno <ueno@unixuser.org>
 * Copyright (C) 2011 Red Hat, Inc.
 * Copyright (C) 2003, 2004
 *   National Institute of Advanced Industrial Science and Technology (AIST)
 *   Registration Number H15PRO112
 *
 * This program is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program.  If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif	/* HAVE_CONFIG_H */

#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <m17n.h>
#include <sqlite3.h>

#undef DEBUG
#define MLEN 4			/* max key length */

static const struct {
  int c, n;
} phrase_dict[] = {
  { '0', 0 },
  { 'a', 1 }, { 'b', 2 }, { 'c', 3 }, { 'd', 4 }, { 'e', 5 }, 
  { 'f', 6 }, { 'g', 7 }, { 'h', 8 }, { 'i', 9 }, { 'j', 10 },
  { 'k', 11 }, { 'l', 12 }, { 'm', 13 }, { 'n', 14 }, { 'o', 15 },
  { 'p', 16 }, { 'q', 17 }, { 'r', 18 }, { 's', 19 }, { 't', 20 },
  { 'u', 21 }, { 'v', 22 }, { 'w', 23 }, { 'x', 24 }, { 'y', 25 },
  { 'z', 26 }, { '\'', 27 }, { ';', 28 }, { '`', 29 }, { '~', 30 }, 
  { '!', 31 }, { '@', 32 }, { '#', 33 }, { '$', 34 }, { '%', 35 },
  { '^', 36 }, { '&', 37 }, { '*', 38 }, { '(', 39 }, { ')', 40 },
  { '-', 41 }, { '_', 42 }, { '=', 43 }, { '+', 44 }, { '[', 45 },
  { ']', 46 }, { '{', 47 }, { '}', 48 }, { '|', 49 }, { '/', 50 },
  { ':', 51 }, { '"', 52 },  { '<', 53 }, { '>', 54 }, { ',', 55 },
  { '.', 56 }, { '?', 57 }, { '\\', 58 }, { 'A', 59 }, { 'B', 60 },
  { 'C', 61 }, { 'D', 62 }, { 'E', 63 }, { 'F', 64 }, { 'G', 65 },
  { 'H', 66 }, { 'I', 67 }, { 'J', 68 }, { 'K', 69 }, { 'L', 70 },
  { 'M', 71 }, { 'N', 72 }, { 'O', 73 }, { 'P', 74 }, { 'Q', 75 },
  { 'R', 76 }, { 'S', 77 }, { 'T', 78 }, { 'U', 79 }, { 'V', 80 },
  { 'W', 81 }, { 'X', 82 }, { 'Y', 83 }, { 'Z', 84 }, { '0', 85 },
  { '1', 86 }, { '2', 87 }, { '3', 88 }, { '4', 89 }, { '5', 90 },
  { '6', 91 }, { '7', 92 }, { '8', 93 }, { '9', 94 }
};

static int phrase_enc_dict[128];
static int phrase_dec_dict[128];

typedef struct _TableContext TableContext;
typedef MPlist *(*TableFunc) (TableContext *context, MPlist *args);

struct _TableDescription {
  const char *name;
  TableFunc open;
  TableFunc lookup;
};
typedef struct _TableDescription TableDescription;

struct _TableContext {
  const TableDescription *desc;
  MInputContext *ic;
  MConverter *converter;

  /* ibus-table */
  char *file;
  sqlite3 *db;
  size_t xlen;
  size_t max_candidates;
};

static MPlist *open_ibus (TableContext *context, MPlist *args);
static MPlist *lookup_ibus (TableContext *context, MPlist *args);

static const TableDescription table_descriptions[] =
  {
    { "ibus", open_ibus, lookup_ibus },
    { NULL }
  };

static MSymbol Mtable, Mibus;
static int initialized = 0;

static void
init_phrase_dict (void)
{
  int i;

  for (i = 0; i < 128; i++)
    phrase_enc_dict[i] = -1;

  memset (phrase_dec_dict, 0, sizeof phrase_dec_dict);
  for (i = 0; i < 94; i++)
    {
      phrase_enc_dict[phrase_dict[i].c] = phrase_dict[i].n;
      phrase_dec_dict[phrase_dict[i].n] = phrase_dict[i].c;
    }
}

static int
encode_phrase (const unsigned char *phrase, int **m)
{
  const unsigned char *p;

  if (m)
    {
      *m = calloc (sizeof (int), strlen ((const char *)phrase));
      if (!*m)
	return -1;
    }

  for (p = phrase; *p != '\0'; p++)
    {
      if (*p >= 128 || phrase_enc_dict[(int)*p] == -1)
	{
	  free (*m);
	  return -1;
	}
      (*m)[p - phrase] = phrase_enc_dict[(int)*p];
    }

  return 0;
}

#if 0
static int
decode_phrase (const int *m, size_t mlen, unsigned char **phrase)
{
  int i;

  if (phrase)
    {
      *phrase = calloc (sizeof (char), mlen);
      if (!*phrase)
	return -1;
    }

  for (i = 0; i < mlen; i++)
    {
      if (m[i] < 0 || m[i] > 94)
	{
	  free (*phrase);
	  return -1;
	}
      (*phrase)[i] = phrase_dec_dict[m[i]];
    }

  return 0;
}
#endif

static MPlist *
add_action (MPlist *actions, MSymbol name, MSymbol key, void *val)
{
  MPlist *action = mplist ();

  mplist_add (action, Msymbol, name);
  if (key != Mnil)
    mplist_add (action, key, val);
  mplist_add (actions, Mplist, action);
  m17n_object_unref (action);
  return actions;
}

static TableContext *
get_context (MInputContext *ic)
{
  MPlist *plist = ic->plist;
  TableContext *context;

  for (; plist && mplist_key (plist) != Mnil; plist = mplist_next (plist))
    {
      if (mplist_key (plist) != Mtable)
	continue;
      context = mplist_value (plist);
      if (context->ic == ic)
	return context;
    }
  return NULL;
}

MPlist *
init (MPlist *args)
{
  MInputContext *ic = mplist_value (args);
  TableContext *context;

  fflush (stderr);
  if (! initialized++)
    {
      init_phrase_dict ();
      Mtable = msymbol (" table");
      Mibus = msymbol ("ibus");
    }

  context = calloc (sizeof (TableContext), 1);
  context->ic = ic;
  context->converter = mconv_buffer_converter (Mcoding_utf_8, NULL, 0);

  if (context)
    mplist_push (ic->plist, Mtable, context);
  return NULL;
}

MPlist *
fini (MPlist *args)
{
  MInputContext *ic = mplist_value (args);
  TableContext *context = get_context (ic);

  if (context)
    {
      if (context->db)
	sqlite3_close (context->db);
      mconv_free_converter (context->converter);
      free (context->file);
      free (context);
    }
  return NULL;
}

static MText *
mtext_from_utf8 (TableContext *context, const unsigned char *buf, size_t size)
{
  mconv_reset_converter (context->converter);
  mconv_rebind_buffer (context->converter, buf, size);
  return mconv_decode (context->converter, mtext ());
}

static ssize_t
mtext_to_utf8 (TableContext *context, MText *mt, unsigned char *buf,
	       size_t size)
{
  ssize_t nbytes;

  mconv_reset_converter (context->converter);
  mconv_rebind_buffer (context->converter, (const unsigned char *)buf, size);
  nbytes = mconv_encode (context->converter, mt);
  buf[nbytes] = '\0';

  return nbytes;
}

static int
get_ime_attr_int (TableContext *context, const char *attr)
{
  sqlite3_stmt *stmt;
  char *sql;
  int retval = -1, rc;

  sql = sqlite3_mprintf ("SELECT val FROM ime WHERE attr = \"%q\"", attr);
  rc = sqlite3_prepare (context->db, sql, strlen (sql), &stmt, NULL);
  sqlite3_free (sql);
  if (rc != SQLITE_OK)
    {
      sqlite3_finalize (stmt);
      return -1;
    }
  rc = sqlite3_step (stmt);
  if (rc == SQLITE_ROW)
    retval = sqlite3_column_int (stmt, 0);
  sqlite3_finalize (stmt);

  return retval;
}

static MPlist *
open_ibus (TableContext *context, MPlist *args)
{
  MText *mt;
  int rc;
  char *file = NULL;
  unsigned char buf[256];

  mt = (MText *) mplist_value (args);
  rc = mtext_to_utf8 (context, mt, buf, sizeof (buf));
  if (rc < 0)
    return NULL;
  file = strdup ((const char *)buf);
  args = mplist_next (args);
  context->xlen = (long) mplist_value (args);
  args = mplist_next (args);
  context->max_candidates = (long) mplist_value (args);

  if (context->db && context->file && strcmp (context->file, file) != 0)
    {
      sqlite3_close (context->db);
      context->db = NULL;
      free (context->file);
    }
  context->file = file;

  if (!context->db)
    {
      rc = sqlite3_open_v2 (file, &context->db, SQLITE_OPEN_READONLY, NULL);
      if (rc)
	{
	  sqlite3_close (context->db);
	  context->db = NULL;
	  free (context->file);
	  context->file = NULL;
	}
    }

  return NULL;
}

MPlist *
open (MPlist *args)
{
  MInputContext *ic;
  TableContext *context;
  MSymbol type;
  int i;

  ic = mplist_value (args);
  context = get_context (ic);

  args = mplist_next (args);
  type = (MSymbol) mplist_value (args);
  args = mplist_next (args);

  for (i = 0;
       i < sizeof (table_descriptions) / sizeof (*table_descriptions);
       i++)
    if (strcmp (table_descriptions[i].name, msymbol_name (type)) == 0)
      {
	context->desc = &table_descriptions[i];
	break;
      }

  if (context->desc)
    (*context->desc->open) (context, args);

  return NULL;
}

static MPlist *
lookup_ibus (TableContext *context, MPlist *args)
{
  unsigned char buf[256];
  char *word = NULL, *sql = NULL, *msql = NULL;
  MPlist *candidates = mplist ();
  size_t len, xlen, wlen, mlen;
  int i, rc;
  int *m = NULL;
  sqlite3_stmt *stmt;
  MText *mt;

  if (!context->db)
    goto out;

  rc = mtext_to_utf8 (context, context->ic->preedit, buf, sizeof (buf));
  if (rc < 0)
    goto out;
  word = strdup ((const char *)buf);
  len = rc;

  mlen = get_ime_attr_int (context, "max_key_length");
  if (mlen < 0)
    mlen = MLEN;

  rc = encode_phrase ((const unsigned char *)word, &m);
  if (rc)
    goto out;

  /* strlen(" AND mXX = XX") = 13 */
  if (len > mlen)
    len = mlen;
  msql = calloc (sizeof (char), 13 * len + 1);
  if (!msql)
    goto out;
  for (i = 0; i < len; i++)
    {
      char s[14];
      sqlite3_snprintf (13, s, " AND m%d = %d", i, m[i]);
      strcat (msql, s);
    }

  sql = calloc (sizeof (char), 128 + strlen (msql) + 1);
  if (!sql)
    goto out;

  /* issue query repeatedly until at least one candidates are found or
     the key length is exceeds mlen */
  xlen = context->xlen;
  wlen = mlen - len + 1;
  for (; xlen <= wlen + 1; xlen++)
    {
      sqlite3_snprintf (128,
			sql,
			"SELECT id, phrase FROM phrases WHERE mlen < %lu",
			len + xlen);
      strcat (sql, msql);
      sqlite3_snprintf (128,
			sql + strlen (sql),
			" ORDER BY mlen ASC, user_freq DESC, freq DESC, id ASC"
			" LIMIT %lu",
			context->max_candidates);
#ifdef DEBUG
      fprintf (stderr, "%s\n", sql);
#endif
      rc = sqlite3_prepare (context->db, sql, strlen (sql), &stmt, NULL);
      if (rc != SQLITE_OK)
	{
	  sqlite3_finalize (stmt);
	  goto out;
	}

      while (sqlite3_step (stmt) == SQLITE_ROW)
	{
	  const unsigned char *text;

	  text = sqlite3_column_text (stmt, 1);
#ifdef DEBUG
	  fprintf (stderr, " %s\n", text);
#endif
	  mt = mtext_from_utf8 (context, text, strlen ((const char *)text));
	  mplist_add (candidates, Mtext, mt);
	  m17n_object_unref (mt);
	}
      sqlite3_finalize (stmt);
      if (mplist_length (candidates) > 0)
	break;
    }

 out:
  if (word)
    free (word);
  if (m)
    free (m);
  if (sql)
    free (sql);
  if (msql)
    free (msql);

  return candidates;
}

static MPlist *
paginate (MPlist *candidates)
{
  MPlist *p = candidates, *pl = mplist (), *plist = mplist ();
  int i;

  for (i = 0; mplist_key (p) == Mtext; p = mplist_next (p), i++)
    {
      mplist_add (pl, Mtext, mplist_value (p));
      if (i % 10 == 9)
	{
	  mplist_add (plist, Mplist, pl);
	  m17n_object_unref (pl);
	  pl = mplist ();
	}
    }

  if (mplist_key (pl) != Mnil)
    mplist_add (plist, Mplist, pl);
  m17n_object_unref (pl);
  return plist;
}

MPlist *
lookup (MPlist *args)
{
  MInputContext *ic;
  MPlist *actions = NULL, *candidates, *plist;
  MSymbol init_state;
  MSymbol select_state;
  TableContext *context;
  MText *mt;

  ic = mplist_value (args);
  context = get_context (ic);

  args = mplist_next (args);
  init_state = (MSymbol) mplist_value (args);
  args = mplist_next (args);
  select_state = (MSymbol) mplist_value (args);

  if (context->desc)
    candidates = (*context->desc->lookup) (context, args);
  else
    candidates = mplist ();

  mt = mtext_dup (ic->preedit);
  mplist_push (candidates, Mtext, mt);
  m17n_object_unref (mt);
  plist = paginate (candidates);
  m17n_object_unref (candidates);

  actions = mplist ();
  add_action (actions, msymbol ("delete"), Msymbol,  msymbol ("@<"));
  mplist_add (actions, Mplist, plist);
  m17n_object_unref (plist);
  add_action (actions, msymbol ("show"), Mnil, NULL);
  add_action (actions, msymbol ("shift"), Msymbol, select_state);

  return actions;
}
