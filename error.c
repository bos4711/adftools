/* error.c - error and debug-routines
 *
 * adftools - A complete package for maintaining image-files for the best
 * Amiga-emulator out there: UAE - http://www.freiburg.linux.de/~uae/
 *
 * Copyright (C)2002-2015 Rikard Bosnjakovic <bos@hack.org>
 */
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>

#include "misc.h"

/* set by all programs in the adftools-package */
extern char *program_name;

/* notify the user something harmless (to stderr) */
void
notify (char *fmt, ...)
{
  char buf[BUFSIZE];
  char *logbuffer;
  va_list ap;

  memset (&buf, 0, BUFSIZE);
  va_start (ap, fmt);
  vsnprintf (buf, BUFSIZE, fmt, ap);

  logbuffer = (char *) malloc (sizeof (buf));
  strcpy (logbuffer, buf);

  fprintf (stderr, "%s", logbuffer);

  free (logbuffer);
}

/* something happened */
void
error (int action, char *fmt, ...)
{
  char buf[BUFSIZE];
  char *logbuffer;
  va_list ap;

  memset (&buf, 0, BUFSIZE);
  va_start (ap, fmt);
  vsnprintf (buf, BUFSIZE, fmt, ap);

  logbuffer = (char *) malloc (sizeof (buf));
  strcpy (logbuffer, buf);

  fprintf (stderr, "%s: %s.\n", program_name, logbuffer);

  free (logbuffer);

  /* something *real* bad happened! */
  if (action)
    exit (42);
}
