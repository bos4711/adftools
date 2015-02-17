/* adfcreate.c - Create and format new image-files
 *
 * adftools - A complete package for maintaining image-files for the best
 * Amiga-emulator out there: UAE - http://www.freiburg.linux.de/~uae/
 *
 * Copyright (C)2002-2015 Rikard Bosnjakovic <bos@hack.org>
 */
#include <adflib.h>
#include <errno.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "error.h"
#include "misc.h"
#include "version.h"

/* the name of this program */
char *program_name = ADFCREATE;

/* controls whether to use high density or not */
static int opt_high_density;

/* long options that have no short eqvivalent short option */
enum {
  HD_OPTION = 1
};

/* options */
static struct option long_options[] =
{
  {"file-system",	required_argument,	0, 'f'},
  {"hd",		no_argument,		0, HD_OPTION},
  {"help",		no_argument,		0, 'h'},
  {"label",		required_argument,	0, 'l'},
  {"version",		no_argument,		0, 'V'},

  /* end of options */
  {NULL, 0, NULL, 0}
};

/********************************************************************/
/*                        disk file-functions                       */
/********************************************************************/
/* create a disk file with the correct size and put a filesystem on it */
int
create_disk_image (char *filename, char *disklabel, int filesystem)
{
  int n_tracks  = TRACKS;					/* 80 */
  int n_heads   = HEADS;					/* 2 */
  int n_sectors = SECTORS;					/* 11 for DD, 22 for HD */
  struct Device* device;

  if (opt_high_density)
    n_sectors *= 2;

  device = adfCreateDumpDevice (filename, n_tracks, n_heads, n_sectors);
  if (!device) {
    error (0, "Can't open '%s': %s", filename, strerror (errno));
    return 0;
  }

  if (adfCreateFlop (device, disklabel, filesystem) != RC_OK) {
    error (0, "Can't format '%s': %s", filename, strerror (errno));
    free (device);
    return 0;
  }

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
    printf ("Usage: %s [OPTIONS]... FILE...\n", program_name);
    printf ("Create and format new adf-files.\n\n");
    printf ("\t-f, --file-system=INT\tfile-system for the disk\n");
    printf ("\t                     \t  0 - OFS (default)\n");
    printf ("\t                     \t  1 - FFS\n");
    printf ("\t                     \t  2 - I-OFS\n");
    printf ("\t                     \t  3 - I-FFS\n");
    printf ("\t                     \t  4 - DC-OFS\n");
    printf ("\t                     \t  5 - DC-FFS\n");
    printf ("\t-H  --hd             \tformat with high density\n");
    printf ("\t-l, --label=NAME     \tuse NAME as disk label\n");
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
  char *label_buf = "";
  int c;
  int filesystem = 0;
  int n_files;

  init_adflib();

  /* parse the options */
  while ((c = getopt_long (argc, argv, "f:hl:V", long_options, NULL)) != -1) {
    switch (c) {
      case 0:
	break;

      case 'f':
	if (!isdigits (optarg))
	  error (1, "Value specified ('%s') for filesystem is not 1-5", optarg);

	filesystem = atoi (optarg);
	if (filesystem > 5) filesystem = 5;
	if (filesystem < 0) filesystem = 0;
	break;

      case 'h':
	print_usage (1);
	break;

      case 'l':
	/* disk label */
	label_buf = optarg;
	break;

      case 'V':
	print_version ();
	exit (0);

      case HD_OPTION:
	/* format high density disks */
	opt_high_density = 1;
	break;

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
/*     int i = 1; */

    while (optind < argc) {
      /* label and filename for the image */
      char label[MAXNAMELEN+1];
      char *name = argv[optind++];

      if (!strncmp (label_buf, "", sizeof (label)))
	/* no label is specified */
//	snprintf (label, sizeof (label), "Empty #%d (%s %s)", i++, PACKAGE_NAME, PACKAGE_VERSION);
	snprintf (label, sizeof (label), "%s %s <bos@hack.org>", PACKAGE_NAME, PACKAGE_VERSION);
      else {
	strncpy (label, label_buf, sizeof (label));
      }

      if (create_disk_image (name, label, filesystem))
	notify ("Created %s.\n", name);
    }
  }

  notify ("Done.\n");

  cleanup_adflib();
  return 1;
}
