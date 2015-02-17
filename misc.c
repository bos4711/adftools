/* misc.c - common routines
 *
 * adftools - A complete package for maintaining image-files for the best
 * Amiga-emulator out there: UAE - http://www.freiburg.linux.de/~uae/
 *
 * Copyright (C)2002-2015 Rikard Bosnjakovic <bos@hack.org>
 */
#include <adflib.h>
#include <ctype.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "error.h"
#include "misc.h"
#include "zfile.h"

/* "12345678" -> 1, "12354foo1234" -> 0 */
int
isdigits (char *str)
{
  register int i;

  for (i = 0; i < strlen (str); i++)
    if (!isdigit (str[i]))
      return 0;

  return 1;
}

/* checks if the buffer (the bootblock) is an adf-file */
int
is_adf_file (unsigned char *buf)
{
  if ((buf[0] != 'D') &&
      (buf[1] != 'O') &&
      (buf[2] != 'S'))
    return 0;

  return 1;
}

/* "apan.apan.ap.jpg.gurka.iff" -> "apan.apan.ap.jpg.gurka" */
char *
strip_extension (char *str)
{
  char *p, *s;

  s = strdup (str);
  p = rindex (s, '.');
  if (p != NULL)
    /* found extension */
    *p = 0;

  return s;
}

char *
strip_trailing_slashes (char *path)
{
  char *p = path;

  while (p[strlen (p)-1] == '/')
    p[strlen (p)-1] = '\0';

  return p;
}

/* "/foo/bar/gazonk.jpg" -> "gazonk.jpg" */
char *
basename (char *filename)
{
  char *p;

  for (p = filename; *p == '/'; ++p)
    /* do nothing if "/", "//" etc */
    ;

  if (*p == '\0')
    return (p - 1);

  p = strrchr (filename, '/');
  if (p == NULL)
    return (filename);
  else
    return (p + 1);
}

/* old code */
/*  char * */
/*  basename (char *filename) */
/*  { */
/*    char *p, *s; */
/*    s = strdup (filename); */
/*    p = rindex (s, '/'); */
/*    if (p != NULL) */
/*      s = (p + 1); */
/*    return s; */
/*  } */

/* "/foo/bar/gazonk.jpg" -> "/foo/bar" */
/*  char * */
/*  dirname (char *filename) */
/*  { */
/*    char *p, *s; */
/*    s = strdup (filename); */
/*    p = rindex (s, '/'); */
/*    if (p != NULL) */
/*      *p = 0; */
/*    return s; */
/*  } */

/* string representation of dosType */
char *
get_adf_dostype (char dostype)
{
  switch (dostype) {
    case 0:
      return "DOS0 (OFS)";
    case 1:
      return "DOS1 (FFS)";
    case 2:
      return "DOS2 (I-OFS))";
    case 3:
      return "DOS3 (I-FFS)";
    case 4:
      return "DOS4 (DC-OFS)";
    case 5:
      return "DOS5 (DC-FFS)";
    default:
      return "Unknown-FS";
  }

  return "ApanAP-FS";
}

/* mounts an adf-image */
int
mount_adf (char *filename, struct Device **dev, struct Volume **vol, int rw)
{
  char *errmsg = "Can't mount the device '%s' (perhaps not a DOS-disk or adf-file)";

  /* check existence and readability of the file */
  if (access (filename, F_OK | R_OK) == -1) {
    notify ("Can't access '%s': %s.\n", filename, strerror (errno));
    return 0;
  }

  if (rw == READ_WRITE)
    *dev = adfMountDev (n_zfile_open(filename, "rw", 1), rw);
  else
    *dev = adfMountDev (n_zfile_open(filename, "r", 0), rw);

  if (!*dev) {
    error (0, errmsg, filename);
    return 0;
  }

  *vol = adfMount (*dev, 0, rw);
  if (!*vol) {
    error (0, errmsg, filename);
    adfUnMountDev (*dev);
    return 0;
  }

  return 1;
}

/* prints a nice header for the adf-image. it *must* be mounted */
void
print_volume_header (char *filename, struct Volume *volume)
{
  register int i;

  if (!volume)
    return;

  /* print header, consisting of filename and the (mounted) volume name */
  /* volName can be NULL for some hardfiles, no idea why.               */
  if (volume->volName) {
    printf ("%s (%s)\n", filename, volume->volName);
    for (i = 0; i < (strlen (filename) + 3 + strlen (volume->volName)); i++)
      putchar ('=');
    putchar ('\n');
  } else {
    printf ("%s\n", filename);
    for (i = 0; i < strlen (filename); i++)
      putchar ('=');
    putchar ('\n');
  }
}

/* a null-function callback for adf-lib */
static void
null_function (void)
{
};

/* initialize the adf-lib */
/* TODO: add an atexit()  */
void
init_adflib (void)
{
  int true = 1;

  adfEnvInitDefault();
  /* redirect errors and warnings to the big black void */
  adfChgEnvProp (PR_EFCT, null_function);
  adfChgEnvProp (PR_WFCT, null_function);

  /* yes, we want to use directory caching */
  adfChgEnvProp (PR_USEDIRC, (void *)&true);
}

/* shut down the adflib */
void
cleanup_adflib (void)
{
  adfEnvCleanUp();
  zfile_exit();
}

/* puts access bits in a readable string */
char *
access2str (long access)
{
  static char str[8+1];

  strcpy (str, "----RWED");
  if (hasD (access)) str[7] = '-';
  if (hasE (access)) str[6] = '-';
  if (hasW (access)) str[5] = '-';
  if (hasR (access)) str[4] = '-';
  if (hasA (access)) str[3] = 'A';
  if (hasP (access)) str[2] = 'P';
  if (hasS (access)) str[1] = 'S';
  if (hasH (access)) str[0] = 'H';

  return (str);
}

/* a temporary function until it's implemeted in adflib */
void
change_to_root_dir (struct Volume *volume)
{
  register int i;

  for (i = 0; i < 42; i++)
    adfParentDir (volume);
}

/* split first word off of rest and put it in first */
char *
splitc (char *first, char *rest)
{
  char *p;
  p = strchr (rest, '/');
  if (p == NULL) {
    if ((first != rest) && (first != NULL))
      first[0] = 0;
    return NULL;
  }

  *p = 0;
  if (first != NULL) strcpy (first, rest);
  if (first != rest) strcpy (rest, p + 1);

  return rest;
}

/* allocate a buffer big enough for a bootblock */
unsigned char *
allocate_bootblock_buf (void)
{
  unsigned char *bootblock;

  bootblock = malloc (BOOTBLOCK_SIZE);
  if (!bootblock)
    return NULL;

  memset (bootblock, 0, BOOTBLOCK_SIZE);
  return bootblock;
}

/* read bootblock bytes into a buffer */
unsigned char *
read_bootblock (char *filename)
{
  unsigned char *bootblock;
  FILE *file;

  bootblock = allocate_bootblock_buf();
  if (!bootblock)
    return NULL;

  memset (bootblock, 0, BOOTBLOCK_SIZE);
  file = f_zfile_open (filename, "r", 0);
  if (!file) {
    free (bootblock);
    return NULL;
  }

  if (fread (bootblock, BOOTBLOCK_SIZE, 1, file) != 1) {
    /* error? */
    if (ferror (file)) {
      /* yes */
      free (bootblock);
      fclose (file);
      return NULL;
    } else if (!feof (file))
      /* no error, but not end of file either. bloody hell! */
      return NULL;
  }

  /* all went fine */
  fclose (file);
  return bootblock;
}
