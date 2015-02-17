/* adflist.c - List files in adf-images
 *
 * adftools - A complete package for maintaining image-files for the best
 * Amiga-emulator out there: UAE - http://www.freiburg.linux.de/~uae/
 *
 * Copyright (C)2002-2015 Rikard Bosnjakovic <bos@hack.org>
 */
#include <adflib.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "error.h"
#include "misc.h"
#include "version.h"

/* the name of this program */
char *program_name = ADFLIST;

/* size of all files */
static long total_size;
static int num_dirs;
static int num_files;

/* options */
static struct option long_options[] =
{
  {"help",	no_argument,		0, 'h'},
  {"version",	no_argument,		0, 'V'},

  /* end of options */
  {NULL, 0, NULL, 0}
};

/********************************************************************/
/*                        disk file-functions                       */
/********************************************************************/
/* output information for/from an Entry-block */
void
print_entry (struct Entry* entry, char *path)
{
  /* skip links, ADFlib do not support them properly (yet) */
  if ((entry->type == ST_LFILE) ||
      (entry->type == ST_LDIR) ||
      (entry->type == ST_LSOFT))
    return;

  /* directories do not have sizes */
  if (entry->type == ST_DIR) {
    printf ("   (DIR)  ");
    num_dirs++;
  } else {
    printf ("%8ld  ", (long int) entry->size);
    total_size += entry->size;
    num_files++;
  }

  printf ("%4d-%02d-%02d  %02d:%02d:%02d ",
	  entry->year, entry->month, entry->days,
	  entry->hour, entry->mins, entry->secs);

  if (strlen (path) > 0)
    printf("%s", path);

  /* append a slash to directories */
  if (entry->type == ST_DIR) {
    size_t len = strlen(entry->name);
    entry->name[len] = '/';
    entry->name[len + 1] = '\0';
  }

  printf ("%-31s  ", entry->name);

  /* no comment */
  if (entry->comment && strlen (entry->comment))
    printf ("(%s)", entry->comment);

  putchar('\n');
}

/* print entire directory tree (recursive) */
void
print_dir (struct Volume *volume, struct List *tree, char *path)
{
  char *buf;
  struct Entry* entry;

  while (tree) {
    print_entry (tree->content, path);
    if (tree->subdir) {
      entry = tree->content;
      if (strlen (path)) {
	buf = malloc (strlen (path) + 1 + strlen (entry->name) + 1);
	if (!buf) {
	  error (0, "Can't allocate memory: %s", strerror (errno));
	  return;
	}
	sprintf (buf, "%s%s", path, entry->name);
	print_dir (volume, tree->subdir, buf);
	free (buf);
      }
      else
	print_dir (volume, tree->subdir, entry->name);
    }
    tree = tree->next;
  }
}

/* entry function */
void
list_root_tree (char *filename, struct Volume *volume)
{
  char *path = "";
  float proc = 0.0;
  register short int i;
//  long disk_size;
  struct List *cell, *list;

  /* print the name of the volume */
  print_volume_header (filename, volume);

  /* get directory tree, recursively */
  cell = list = adfGetRDirEnt (volume, volume->curDirPtr, 1);
  print_dir (volume, cell, path);

  for (i = 0; i < 30; i++)
    putchar ('=');

//  proc = (float)total_size / (float)((volume->lastBlock + 1) * 512);
  proc = (float)total_size / (float)((volume->lastBlock + 1) * volume->blockSize);
//  disk_size = (volume->lastBlock + 1) * volume->blockSize;

  putchar ('\n');
  printf ("%8d file(s)\n", num_files);
  printf ("%8d dir(s)\n", num_dirs);
  printf ("%8ld bytes used (%5.2f%% full)\n", total_size, proc * 100.0);
//  printf ("\n%8ld of %ld bytes used (%5.2f%% full)\n", total_size, disk_size, proc * 100.0);

//  adfFreeDirList(list);
  putchar ('\n');
}

/********************************************************************/
/*                     print version, usage, etc                    */
/********************************************************************/
void
print_usage (int status)
{
  if (!status) {
    notify ("Try '%s --help' for more information.\n", program_name);
  } else {
    printf ("Usage: %s [OPTIONS]... FILE(s)...\n", program_name);
    printf ("List files in an adf-image.\n\n");
    printf ("\t-h, --help           \tdisplay this help and exit\n");
    printf ("\t-V, --version        \tdisplay version information and exit\n");
    printf ("\n");
    print_footer ();
  }

  exit (0);
}

/********************************************************************/
/*                            here we go                            */
/********************************************************************/
int
main (int argc, char *argv[])
{
  int c;
  int n_files;
  struct Device *device;
  struct Volume *volume;

  init_adflib();

  /* parse the options */
  while ((c = getopt_long (argc, argv, "hV", long_options, NULL)) != -1) {
    switch (c) {
      case 0:
	break;

      case 'h':
	print_usage (1);
	break;

      case 'V':
	print_version ();
	exit (0);

      default:
	print_usage (0);
    }
  }

  n_files = argc - optind;
  if (n_files == 0) {
    error (0, "No files specified");
    print_usage (0);
  }

  /* all remaining arguments should be files */
  if (optind < argc)
    while (optind < argc) {
      char *filename = argv[optind++];

      /* lazy way to mount both the device and the volume (if possible) */
      if (!mount_adf (filename, &device, &volume, READ_ONLY))
	continue;

      total_size = 0;
      num_files = 0;
      num_dirs = 0;
      list_root_tree (filename, volume);
    }

  printf ("All Done.\n");

  cleanup_adflib();
  return 1;
}
