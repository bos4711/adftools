/* adfinstall.c - Create bootblocks on adf-images
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
char *program_name = ADFINSTALL;

/* install using a specified bootblock */
static int opt_install;

/* install w/o argument => install a default bootblock */
static int opt_install_no_arg;

/* long options that have no short eqvivalent short option */
enum {
  INSTALL_OPTION = 1
};

/* options */
static struct option long_options[] =
{
  {"install",	required_argument,	0, INSTALL_OPTION},
  {"help",	no_argument,		0, 'h'},
  {"version",	no_argument,		0, 'V'},

  /* end of options */
  {NULL, 0, NULL, 0}
};

/********************************************************************/
/*                        disk file-functions                       */
/********************************************************************/
/* the reason for not using adfInstallBootBlock() is that the function */
/* requires a mounted image, which in turn requires a filesystem. the  */
/* way we're doing below handles trackdisks as well, i.e. reinstalling */
/* a bootblock for a corrupt trackdisk-demo will be completely legal.  */

/* FIXME: add a check in the bootblock-writer for the 1024-limit */
int
install_bootblock (unsigned char *bootblock, char *filename)
{
  FILE *file;
  long seek_pos = 0;

  file = fopen (filename, "r+");
  if (!file)
    return 0;

  /* if and only if the disk already contains a filesystem, skip ahead over */
  /* the marker ('D', 'O', 'S' + filesystem-flag) to let them remain intact */
  if (is_adf_file (bootblock))
    seek_pos = 4;

  /* we must do this to let the 4 bytes ("DOS"+filesystem) remain intact */
  if (fseek (file, seek_pos, SEEK_SET) == -1) {
    fclose (file);
    return 0;
  }

  if (fwrite ((bootblock + seek_pos), (BOOTBLOCK_SIZE - seek_pos), 1, file) != 1) {
    if (ferror (file)) {
      /* error */
      fclose (file);
      return 0;
    } else if (!feof (file)) {
      /* no error and not end of file(?) */
      fclose (file);
      return 0;
    }
  }

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
    printf ("Usage: %s [OPTIONS]... FILE...\n", program_name);
    printf ("Install bootblock on adf-files.\n\n");
    printf ("\t    --install[=FILE] \tinstall bootblock `FILE'\n");
    printf ("\t-i                   \tlike --install, but does not accept an argument and\n");
    printf ("\t                     \ta standard OS1.3-bootblock will be installed (default)\n");
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
  char *bootblock_filename = NULL;
  int c;
  int n_files;

  init_adflib();

  /* parse the options */
  while ((c = getopt_long (argc, argv, "isd:hV", long_options, NULL)) != -1) {
    switch (c) {
      case 0:
	break;

      case INSTALL_OPTION:
	bootblock_filename = optarg;
	opt_install = 1;
	break;

      case 'h':
	print_usage (1);
	break;

      case 'i':
	opt_install_no_arg = 1;
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

  /* -i is default */
  if (!opt_install && !opt_install_no_arg)
    opt_install_no_arg = 1;

  /* all remaining arguments should be files */
  if (optind < argc)
    while (optind < argc) {
      int ret;
      char *filename = argv[optind++];
      unsigned char *bootblock = NULL;

      /* read bootblock from the adf-file that should be installed */
      unsigned char *diskbuf = read_bootblock (filename);

      if (!diskbuf) {
        error (0, "Can't read bootblock from '%s': %s", filename, strerror (errno));
        continue;
      }

      if (!is_adf_file (diskbuf)) {
        error (0, "The file '%s' is not a valid adf-file", filename);
        continue;
      }

      if (bootblock_filename && opt_install && !opt_install_no_arg) {
        /**************************/
        /* case 1: --install FILE */
        /**************************/
        bootblock = read_bootblock (bootblock_filename);
        if (!bootblock) {
          error (0, "Can't read bootblock from '%s': %s", filename, strerror (errno));
          continue;
        }

        if (!is_adf_file (bootblock)) {
          error (0, "The file '%s' is not an valid bootblock", bootblock_filename);
          continue;
        }
      } else if (opt_install_no_arg && !opt_install) {
        /**************/
        /* case 2: -i */
        /**************/
        extern unsigned char OS13_bootblock[49];

        bootblock = allocate_bootblock_buf();
        if (!bootblock)
          error (1, "Can't allocate memory for bootblock: %s", strerror (errno));

        memcpy (bootblock, &OS13_bootblock, sizeof (OS13_bootblock));
      } else if (opt_install_no_arg && opt_install) {
        /****************************/
        /* case 3: -i and --install */
        /****************************/
        error (0, "Both --install and -i? You make me confused");
        print_usage (0);
      } else if (!opt_install_no_arg && !opt_install) {
        /*******************************************/
        /* case 4: The meaning of life disappeared */
        /*******************************************/
        notify ("Internal error. What the heck did you do?");
        abort ();
      }

      ret = install_bootblock (bootblock, filename);
      if (!ret)
        notify ("FAILED - ", strerror (errno));

      if (opt_install_no_arg)
        notify ("Installing an OS1.3-bootblock to '%s': ", filename);
      else if (opt_install)
        notify ("Installing '%s' to '%s': ", bootblock_filename, filename);

      if (ret)
        notify ("Done.\n");
      else
        notify ("%s - FAILED.\n", strerror (errno));
    }

  printf ("All Done.\n");

  cleanup_adflib();
  return 1;
}
