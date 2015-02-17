/* adfdelete.c - Delete file(s) from an adf-image
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
char *program_name = ADFDELETE;

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
/* the actual remover. the last argument is only used in case of an error */
void
do_delete_file (struct Volume *volume, SECTNUM parent, char *file, char *fullpath)
{
  struct File *f;

  /* check if the file exists, since adfRemoveEntry() only returns YES or NO */
  f = adfOpenFile (volume, file, "r");
  if (f == NULL)
    error (0, "No such file or directory '%s'", fullpath);
  else if (adfRemoveEntry (volume, parent, file) != RC_OK) {
    if (adfChangeDir (volume, file) == RC_OK)
      error (0, "non-empty directory '%s'. Register to be able to delete recursively :)", fullpath);
    else
      error (0, "Could not delete '%s'. No idea why", fullpath);

    adfCloseFile (f);
  } else
    notify ("Removed '%s'.\n", fullpath);
}

/* entry function */
void
delete_file (struct Volume *volume, char *file)
{
  char *fullpath = strdup(file);
  SECTNUM parent;

  /* we need two versions */
  if (!strchr (file, '/')) {
    /******************************************************/
    /* CASE 1: no subdirs specified (no "/" in file name) */
    /******************************************************/
    parent = volume->curDirPtr;

    do_delete_file (volume, parent, file, fullpath);
  } else {
    char *tmp = strdup (file);

    while (splitc (tmp, file))
      if (adfChangeDir (volume, tmp) != RC_OK) {
        notify ("Could not enter directory '%s', no idea why.", tmp);
        free (tmp);

        return;
      }

    parent = volume->curDirPtr;

    do_delete_file (volume, parent, file, fullpath);

    free (tmp);
  }

  free (fullpath);
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
    printf ("Usage: %s ADF-IMAGE FILE...\n", program_name);
    printf ("Delete FILE from ADF-IMAGE. Full path to FILE is required.\n\n");
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
  char *firstarg;
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

  /* first argument is (should be) the adf-file */
  firstarg = argv[optind++];

  n_files = argc - optind;
  if (n_files < 0) {
    error (0, "No adf-file specified");
    print_usage (0);
  } else if (n_files == 0) {
    error (0, "Too few arguments");
    print_usage (0);
  }

  /* all remaining arguments should be directories to create */
  if (optind < argc) {
    /* mount the adf-file */
    if (!mount_adf (firstarg, &device, &volume, READ_WRITE))
      exit(1);

    /* step through the remaining arguments */
    notify ("\nProcessing %s:\n", firstarg);

    while (optind < argc) {
      char *file_to_delete = argv[optind++];

      change_to_root_dir (volume);
      delete_file (volume, file_to_delete);
    }
  }

  printf ("All Done.\n");

  cleanup_adflib();
  return 1;
}
