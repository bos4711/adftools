/* adfextract.c - Copy/extract files from adf-images
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
#include <time.h>
#include <unistd.h>
#include <utime.h>

#include "error.h"
#include "misc.h"
#include "version.h"

/* the name of this program */
char *program_name = ADFEXTRACT;

/* long options that have no short eqvivalent short option */
enum {
  EXTRACT_OPTION = 1
};

/* options */
static struct option long_options[] =
{
  {"extract",	required_argument,	0, EXTRACT_OPTION},
  {"list",	no_argument,		0, 'l'},
  {"tree",	no_argument,		0, 'r'},
  {"help",	no_argument,		0, 'h'},
  {"version",	no_argument,		0, 'V'},

  /* end of options */
  {NULL, 0, NULL, 0}
};

/* struct array for timestamps */
#define MAX_FTS 8

struct fts {
  char   *filename;
  struct utimbuf utime_buf;
};

struct fts *file_timestamps;
static int n_fts = 0;
static int max_fts = MAX_FTS;

/********************************************************************/
/*                        disk file-functions                       */
/********************************************************************/
/* save the timestamp from the adf-file for later update */
void
update_timestamp(char *filename, struct Entry *entry)
{
  struct tm time_str;
  struct utimbuf *utime_buf;
  time_t time_ret;

  if (n_fts == max_fts) {
    /* array is full, increase it */
    max_fts += MAX_FTS;
    file_timestamps = realloc (file_timestamps, sizeof (struct fts) * max_fts);
  }

  utime_buf = &file_timestamps[n_fts].utime_buf;

  time_str.tm_year = entry->year - 1900;
  time_str.tm_mon = entry->month - 1;
  time_str.tm_mday = entry->days;
  time_str.tm_hour = entry->hour;
  time_str.tm_min = entry->mins;
  time_str.tm_sec = entry->secs;
  time_str.tm_isdst = -1;

  time_ret = mktime(&time_str);
  if (time_ret == -1)
    error (0, "cannot set timestamp for %s", filename);
  else {
    utime_buf->actime = time_ret;
    utime_buf->modtime = time_ret;
    file_timestamps[n_fts].filename = strdup (filename);
/*     strcpy(file_timestamps[n_fts].filename, filename); */
    n_fts++;
  }
}
void
do_extract_file(struct Volume *vol, struct Entry *entry, char* path, unsigned char *extbuf)
{
  char *filename;
  char *name = entry->name;
  FILE *out;
  long n_bytes;
  struct File *file;

  /* allocate memory for the path, separator, filename and the trailing separator */
  filename = malloc (strlen (path) + 1 + strlen (name) + 1);
  if (!filename) {
    error (0, "%s: %s", name, strerror (errno));
    return;
  }

  /* construct a full pathname for the file to be extracted (to) */
  sprintf (filename, "%s%c%s", path, DIRSEP, name);
  out = fopen (filename, "wb");
  if (!out) {
    error (0, "%s: Can't open file for output: %s", filename, strerror (errno));
    return;
  }

  /* open the file in the image */
  file = adfOpenFile (vol, name, "r");
  if (!file) {
    error (0, "%s: Can't read file from image. Access bits: '%s'", filename, access2str (entry->access));
    fclose (out);
    return;
  }

  /* read the file from the image */
  n_bytes = adfReadFile(file, BUFSIZE, extbuf);
  while (!adfEndOfFile (file)) {
    fwrite (extbuf, sizeof (unsigned char), n_bytes, out);
    n_bytes = adfReadFile (file, BUFSIZE, extbuf);
  }

  /* write any remaining bytes */
  if (n_bytes > 0)
    fwrite (extbuf, sizeof (unsigned char), n_bytes, out);

  /* we're done.  print some extracting info */
  printf ("Extracted file '%s'\n", filename);

  /* save a copy of the timestamp */
  update_timestamp (filename, entry);

  /* close both files */
  adfCloseFile (file);
  fclose (out);

  free (filename);
}

/* the Recursive Extracter(tm) */
void
do_extract_tree (struct Volume *vol, struct List* tree, char *path, unsigned char *extbuf)
{
  struct Entry* entry;
  char *dir;

  while(tree) {
    entry = tree->content;
    if (entry->type == ST_DIR) {
      /* dir */
      dir = malloc (strlen (path) + 1 + strlen (entry->name) + 1);
      if (!dir) {
	error (0, "Can't allocate memory for %s: %s", entry->name, strerror (errno));
	return;
      }

      /* dir to create */
      sprintf (dir, "%s%c%s", path, DIRSEP, entry->name);

      /* show extracting information */
//      printf ("%s%c\n", dir, DIRSEP);

      if (access (dir, F_OK) == -1) {
	/* dir does not exist, let's create it */
	if (mkdir (dir, 0755) == -1) {
	  error (1, "Can't create '%s': %s", dir, strerror (errno));
	} else {
	  notify ("Created dir '%s'.\n", dir);
          update_timestamp (dir, entry);
	}
      }

      if (tree->subdir != NULL) {
	if (adfChangeDir (vol, entry->name) == RC_OK) {
	  if (dir) {
	    do_extract_tree (vol, tree->subdir, dir, extbuf);
	    free (dir);
	  } else
	    do_extract_tree (vol, tree->subdir, entry->name, extbuf);
	  adfParentDir (vol);
	} else {
	  fprintf (stderr, "ExtractTree: dir \"%s/%s\" not found.\n", path,entry->name);
	}
      }
    } else if (entry->type == ST_FILE) {
      /* file */
      do_extract_file (vol, entry, path, extbuf);
    }

    tree = tree->next;
  }
}

/* entry function for the recursive extracter */
void
extract_tree (char *filename, char *path, struct Volume *volume)
{
  struct List *cell, *list;
  unsigned char *buf;

  buf = malloc (BUFSIZE);
  if (!buf) {
    error (0, "Can't allocate memory for extraction of '%s': %s", filename, strerror (errno));
    return;
  }

  print_volume_header (filename, volume);

  if (strlen (path) == 0)
    /* create a directory with the same name as the image, modulo extension */
    path = strip_extension (basename (filename));

  /* make sure the dir to extract to exists, else create it */
  if (access (path, F_OK) == -1) {
    /* dir does not exist, let's create it */
    if (mkdir (path, 0755) == -1) {
      error (1, "Can't create '%s': %s", path, strerror (errno));
    } else {
      notify ("Created extraction dir '%s'.\n", path);
    }
  }

  cell = list = adfGetRDirEnt (volume, volume->curDirPtr, 1);
  do_extract_tree (volume, cell, path, buf);

  putchar ('\n');
  adfFreeDirList (list);
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
    printf ("List/extract files from an adf-image.\n\n");
    printf ("\t-e                   \tcreates a directory and extracts all contents\n");
    printf ("\t                     \tof the image into there (default)\n");
    printf ("\t    --extract=DIR    \tsame as -e, but extracts to DIR instead\n");
    printf ("\t-l, --list           \tlists root directory contents\n");
    printf ("\t-r, --tree           \tlists directory tree contents\n");
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
  char *extract_dir = "";
  int c, i;
  int n_files;
  struct Device *device;
  struct Volume *volume;

  init_adflib();

  /* parse the options */
  while ((c = getopt_long (argc, argv, "ehV", long_options, NULL)) != -1) {
    switch (c) {
      case 0:
	break;

      case EXTRACT_OPTION:
	extract_dir = optarg;
	break;

      case 'e':
	extract_dir = "";
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
  if (optind < argc) {
    /* initiate the array for timestamps */
    file_timestamps = malloc (sizeof(struct fts) * MAX_FTS);

    while (optind < argc) {
      char *filename = argv[optind++];

      /* lazy way to mount both the device and the volume (if possible) */
      if (!mount_adf (filename, &device, &volume, READ_ONLY))
	continue;

      extract_tree (filename, extract_dir, volume);

      printf ("Updating timestamps...\n");
      for (i = 0; i < n_fts; i++) {
        char *filename = file_timestamps[i].filename;

        utime(filename, &file_timestamps[i].utime_buf);
        if (filename)
          free(filename);
      }
    }
  }

  printf ("All Done.\n");

  cleanup_adflib();
  return 1;
}
