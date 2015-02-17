/* adfdump.c - Read/dump bootblocks from adf-images
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

#include "bootblocks.h"
#include "error.h"
#include "misc.h"
#include "version.h"

/* the name of this program */
char *program_name = ADFDUMP;

/* controls whether to dump to stdout or not */
static int opt_dump_to_stdout;

/* options */
static struct option long_options[] =
{
  {"dir",	required_argument,	0, 'd'},
  {"stdout",	no_argument,		0, 's'},
  {"help",	no_argument,		0, 'h'},
  {"version",	no_argument,		0, 'V'},

  /* end of options */
  {NULL, 0, NULL, 0}
};

/********************************************************************/
/*                        disk file-functions                       */
/********************************************************************/
/* TODO: create 'path' if it does not exist */
/* perhaps use realpath() instead of this sloppy solution? */
int
dump_bootblock_to_file (unsigned char *bootblock, char *path, char *filename)
{
  char path_and_filename[BUFSIZE];
  char *tmp_filename;
  FILE *file;

  tmp_filename = malloc (strlen (filename) + 3);
  if (!tmp_filename)
    return 0;

  /* strip extension from file and wipe the path (if any) from it */
  tmp_filename = strip_extension (basename (filename));

  /* if the user did not specify to dump to a dir, path will be NULL */
  if (!path)
    path = "";

  if (strlen (path) == 0)
    snprintf (path_and_filename, BUFSIZE, "%s.bb", tmp_filename);
  else
    snprintf (path_and_filename, BUFSIZE, "%s%c%s.bb", strip_trailing_slashes (path), DIRSEP, tmp_filename);

  file = fopen (path_and_filename, "w");
  if (!file) {
    free (tmp_filename);
    return 0;
  }

  if (fwrite (bootblock, BOOTBLOCK_SIZE, 1, file) != 1) {
    free (tmp_filename);
    fclose (file);
    return 0;
  }

  notify ("Saved to '%s'.\n", path_and_filename);

  free (tmp_filename);
  fclose (file);
  return 1;
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
    printf ("Dump bootblock from adf-files.\n\n");
    printf ("\t-d, --dir=NAME       \tdump bootblock(s) to directory NAME\n");
    printf ("\t-s, --stdout         \twrite everything to stdout\n");
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
  char *dir = NULL;
  int c;
  int n_files;

  init_adflib();

  /* parse the options */
  while ((c = getopt_long (argc, argv, "sd:hV", long_options, NULL)) != -1) {
    switch (c) {
      case 0:
	break;

      case 'd':
	dir = optarg;
	break;

      case 'h':
	print_usage (1);
	break;

      case 's':
	opt_dump_to_stdout = 1;
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
    error (0, "No files specified, nothing to do");
    print_usage (0);
  }

  /* all remaining arguments should be files */
  if (optind < argc) {
    while (optind < argc) {
      int ret;
      char *filename = argv[optind++];
      unsigned char *bootblock = NULL;

      bootblock = read_bootblock (filename);
      if (!bootblock) {
        error (0, "Can't read bootblock from '%s': %s", filename, strerror (errno));
        continue;
      }

      if (!is_adf_file (bootblock)) {
        error (0, "'%s' doesn't look like an adf-file to me", filename);
        continue;
      }

      /* output dir was specified, check if we can write into it */
      if (dir) {
        if (access (dir, F_OK) == -1) {
          /* dir does not exist, create it */
          if (mkdir (dir, 0755) == -1) {
            error (1, "Can't create '%s': %s", filename, strerror (errno));
          } else {
            notify ("Created '%s'.\n", dir);
          }
        }
      }

      notify ("Dumping bootblock of '%s': ", filename);
      if (opt_dump_to_stdout)
        write (1, bootblock, 1024);
      else {
        /* grab the bootblock */
        /* if dir was specified, the path will be in 'dir' */
        ret = dump_bootblock_to_file (bootblock, dir, filename);

        if (!ret) {
          notify ("Error when dumping: %s", strerror (errno));
          free (bootblock);
          continue;
        }
      }

      free (bootblock);
    }
  }

  notify ("All Done.\n");

  cleanup_adflib();
  return 1;
}
