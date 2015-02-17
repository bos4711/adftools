/* version.c - common version printing routines
 *
 * adftools - A complete package for maintaining image-files for the best
 * Amiga-emulator out there: UAE - http://www.freiburg.linux.de/~uae/
 *
 * Copyright (C)2002-2015 Rikard Bosnjakovic <bos@hack.org>
 */
#include <stdio.h>

#include "version.h"

/* set by all programs in the adftools-package */
extern char *program_name;

void
print_header ()
{
  printf ("%s (%s) %s\n", program_name, PACKAGE_NAME, PACKAGE_VERSION);
  printf ("Copyright (C) %s Rikard Bosnjakovic.\n\n", PACKAGE_DATE);

  printf ("This is free software; see the source for copying conditions.  There is no\n");
  printf ("warranty; not even for merchantability or fitness for a particular purpose.\n\n");
}

void
print_footer ()
{
  printf ("Report bugs to <bos@hack.org>.\n");
}

void
print_version ()
{
  print_header ();
  print_footer ();
}

