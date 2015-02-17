/* adfinfo.c - Display information about an adf-image
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

/*  #include "adfextract.h" */
#include "error.h"
#include "misc.h"
#include "version.h"
#include "zfile.h"

/* the name of this program */
char *program_name = ADFINFO;

/* options */
static struct option long_options[] =
{
  {"info",	no_argument,		0, 'i'},
  {"help",	no_argument,		0, 'h'},
  {"version",	no_argument,		0, 'V'},

  /* end of options */
  {NULL, 0, NULL, 0}
};

/********************************************************************/
/*                        disk file-functions                       */
/********************************************************************/
int
print_image_info (char *filename)
{
  int i;
  struct Device *dev;
  struct Volume *vol;

  mount_adf(filename, &dev, &vol, READ_ONLY);

  /* dev = adfMountDev (n_zfile_open(filename, "r"), 1); */
  if ( (!dev) || (!vol) )
    return 0;
  /* if (!dev) */
  /*   return 0; */

  /* vol = adfMount (dev, 0, 1); */
  /* if (!vol) */
  /*   return 0; */

  for (i = 0; i < dev->nVol; i++) {
    register long j;
    printf ("%s\n", filename);
    for (j = 0; j < strlen (filename); j++)
      putchar ('=');
    printf ("\nLabel       : %-30s\n", (dev->volList[i]->volName) ? dev->volList[i]->volName : "(Unknown)");
    printf ("Type        : ");

    switch (vol->dev->devType) {
      case DEVTYPE_FLOPDD:
	printf ("Floppy Double Density - 880Kb\n");
	break;
      case DEVTYPE_FLOPHD:
	printf ("Floppy High Density - 1760Kb\n");
	break;
      case DEVTYPE_HARDDISK:
	printf ("Hard Disk partition - %3.1fKb\n",
		((vol->lastBlock - vol->firstBlock + 1) * 512.0) / 1024.0);
	break;
      case DEVTYPE_HARDFILE:
	printf ("Hardfile - %3.1fKb\n",
		((vol->lastBlock - vol->firstBlock + 1) * 512.0) / 1024.0);
	break;
      default:
	printf ("Unknown device type.\n");
    }

    printf ("Filesystem  : %s\n", get_adf_dostype (dev->volList[0]->dosType));
    printf ("Cylinders   : %ld\n", (long int)dev->cylinders);
    printf ("Heads       : %ld\n", (long int)dev->heads);
    printf ("Sectors/Cyl : %ld\n", (long int)dev->sectors);
    printf ("Blocks      : %ld (%ld - %ld)\n", (long int)(vol->lastBlock + 1), (long int)vol->firstBlock, (long int)vol->lastBlock);
    j = adfCountFreeBlocks (vol);
    printf ("Blocks used : %ld\n", (vol->lastBlock - j) + 1);
    printf ("Blocks free : %ld\n\n", j);
  }

  adfUnMount (vol);
  adfUnMountDev (dev);
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
    printf ("Usage: %s FILE(s)...\n", program_name);
    printf ("Display information about an adf-image.\n\n");
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
    error (0, "Nothing to do");
    print_usage (0);
  }

  /* all remaining arguments should be files */
  if (optind < argc)
    while (optind < argc) {
      char *filename = argv[optind++];

      print_image_info (filename);
      /* if (!print_image_info (filename)) */
      /*   notify ("Can't mount '%s', not a DOS-disk (or not an adf-file).\n", filename); */
    }

  printf ("All Done.\n");

  cleanup_adflib();
  return 1;
}
