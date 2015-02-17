/*
 * UAE - The Un*x Amiga Emulator
 *
 * routines to handle compressed file automatically
 *
 * (c) 1996 Samuel Devulder, Tim Gunn
 *
 * Modified 2013-11-22 by Rikard Bosnjakovic <bos@hack.org> for
 * use in the adftools package.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "zfile.h"

static struct zfile
{
  struct zfile *next;
  FILE *f;
  unsigned short compressed;
  unsigned short re_compress;
  char orgname[256];
  char name[256];
} *zlist;

/*
 * gzip decompression
 */
static int
gunzip (const char *decompress, const char *src, const char *dst)
{
    char cmd[1024];
    if (!dst)
	return 1;
    sprintf (cmd, "%s -c -d \"%s\" > \"%s\"", decompress, src, dst);

    return !system (cmd);
}

/*
 * decompresses the file (or check if dest is null)
 */
static int
uncompress (const char *name, char *dest)
{
    char *ext = strrchr (name, '.');
    char nam[1024];

    if (ext != NULL && access (name, 0) >= 0) {
	ext++;
	if (strcasecmp (ext, "z") == 0
	    || strcasecmp (ext, "gz") == 0
	    || strcasecmp (ext, "adz") == 0
	    || strcasecmp (ext, "roz") == 0)
	    return gunzip ("gzip", name, dest);
	if (strcasecmp (ext, "bz") == 0)
	    return gunzip ("bzip", name, dest);
	if (strcasecmp (ext, "bz2") == 0)
	    return gunzip ("bzip2", name, dest);
    }

    if (access (strcat (strcpy (nam, name), ".z"), 0) >= 0
        || access (strcat (strcpy (nam, name), ".Z"), 0) >= 0
        || access (strcat (strcpy (nam, name), ".gz"), 0) >= 0
        || access (strcat (strcpy (nam, name), ".GZ"), 0) >= 0
        || access (strcat (strcpy (nam, name), ".adz"), 0) >= 0
        || access (strcat (strcpy (nam, name), ".roz"), 0) >= 0)
        return gunzip ("gzip", nam, dest);

    if (access (strcat (strcpy (nam, name), ".bz"), 0) >= 0
        || access (strcat (strcpy (nam, name), ".BZ"), 0) >= 0)
        return gunzip ("bzip", nam, dest);

    if (access (strcat (strcpy (nam, name), ".bz2"), 0) >= 0
        || access (strcat (strcpy (nam, name), ".BZ2"), 0) >= 0)
        return gunzip ("bzip2", nam, dest);

    return 0;
}

static int
compress (const char *src, const char *dst)
{
  char cmd[1024];
  if (!dst)
    return 1;

  if (access (dst, W_OK) == 0) {
    sprintf (cmd, "gzip -9nc \"%s\" > \"%s\"", src, dst);
    return !system (cmd);
  }

  return 0;
}

/*
 * fopen() for a compressed file
 */
struct zfile *
zfile_open (const char *name, const char *mode, unsigned short re_compress)
{
    struct zfile *l;
    int fd = 0;

    l = malloc (sizeof *l);
    if (!l)
	return NULL;

    strcpy (l->orgname, name);
    if (!uncompress (name, NULL)) {
      strcpy (l->name, name);
      l->f = fopen (l->name, mode);
      l->compressed = 0;

      return l;
    }

    tmpnam (l->name);

    /* On the amiga this would make ixemul loose the break handler */
    /* ==> fixed in exmul v4.6 */
    fd = creat (l->name, 0666);
    if (fd < 0)
	return NULL;

    if (!uncompress (name, l->name)) {
	free (l);
	close (fd);
	unlink (l->name);
	return NULL;
    } else {
      l->compressed = 1;
      l->re_compress = re_compress;
    }

    l->f = fopen (l->name, mode);

    if (l->f == NULL) {
	free (l);
	return NULL;
    }

    l->next = zlist;
    zlist   = l;

    return l;
}

FILE *
f_zfile_open (const char *name, const char *mode, unsigned short re_compress)
{
  return zfile_open (name, mode, re_compress)->f;
}

char *
n_zfile_open (const char *name, const char *mode, unsigned short re_compress)
{
  return zfile_open (name, mode, re_compress)->name;
}

/*
 * called on exit()
 */
void
zfile_exit (void)
{
    struct zfile *l;

    while ((l = zlist)) {
      zlist = l->next;

      /* try to compress uncompressed files before cleaning up */
      if (l->compressed && l->re_compress)
        compress(l->name, l->orgname);

      fclose(l->f);
      unlink(l->name); /* sam: in case unlink() after fopen() fails */
      free(l);
    }
}

/*
 * fclose() but for a compressed file
 */
int
zfile_close (FILE *f)
{
    struct zfile *pl = NULL;
    struct zfile *l  = zlist;
    int ret;

    while(l && l->f!=f) {
	pl = l;
	l = l->next;
    }
    if (!l)
	return fclose(f);
    ret = fclose(l->f);

    if(!pl)
	zlist = l->next;
    else
	pl->next = l->next;
    free(l);

    return ret;
}
