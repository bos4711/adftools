/* adfcopy.c - Copy file(s) to an adf-image
 *
 * adftools - A complete package for maintaining image-files for the best
 * Amiga-emulator out there: UAE - http://www.freiburg.linux.de/~uae/
 *
 * Copyright (C)2002-2015 Rikard Bosnjakovic <bos@hack.org>
 */
#include <adflib.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "error.h"
#include "misc.h"
#include "version.h"

/* the name of this program */
char *program_name = ADFCOPY;

/* we need to determine the maximum size of a path, using PATH_MAX */
#ifdef PATH_MAX
static int pathmax = PATH_MAX;
#else
static int pathmax = 0;
#endif

/* function type that's called for each filename in the tree walker */
typedef int adf_ftw_t (char *, const struct stat *, int);

/* some global counters for the files */
static long n_files, n_dirs;

/* contains full path for every file in the tree walker */
static char *fullpath;

/* contains path relative to the starting path */
static char *adf_path;

/* sector value for the current directory */
static SECTNUM sector;

/* should directories be created when copying files the non-recursively way? */
static int opt_force;

/* the structures for the adf-image */
static struct Device *device;
static struct Volume *volume;

/* options */
static struct option long_options[] =
{
  {"force",     no_argument,            0, 'f'},
  {"help",	no_argument,		0, 'h'},
  {"version",	no_argument,		0, 'V'},

  /* end of options */
  {NULL, 0, NULL, 0}
};

/********************************************************************/
/*                        disk file-functions                       */
/********************************************************************/
/* copies file time from the source file to the adf-file */
void
adf_copy_file_time (struct Volume *vol, char *filename) {
  struct File *file;
  /* SECTNUM nSect; */
  struct bEntryBlock entry, parent;
  struct DateTime dt;
  struct stat st_buf;
  struct tm *local;
  /* char *adf_filename = filename; //strdup(filename); */

  /* when copying a whole directory, the filename will be something
     like foo/devs/bar, where "foo" will not be an actual element in
     the adf-file so we'll strip it off */
/*   printf("APFIS?\n"); */
/*   while (*adf_filename) { */
/*     printf ("%c", *adf_filename); */
/*     if (*adf_filename == '/') */
/*       break; */
/*     adf_filename++; */
/*   } */
/*   adf_filename++; */
/*   printf("\nZE FILENAME = %s\n", adf_filename); */

  adfReadEntryBlock (vol, vol->curDirPtr, &parent);
  /* nSect = adfNameToEntryBlk (vol, parent.hashTable, basename (filename), &entry, NULL); */
  adfNameToEntryBlk (vol, parent.hashTable, basename (filename), &entry, NULL);

  file = (struct File*) malloc (sizeof (struct File));
  if (!file) {
    error (0, "Can't allocate memory for timestamp copy #1");
    return;
  }

  file->fileHdr = (struct bFileHeaderBlock*) malloc (sizeof (struct bFileHeaderBlock));
  if (!file->fileHdr) {
    error (0, "Can't allocate memory for timestamp copy #2");
    return;
  }

  file->currentData = malloc (512 * sizeof (unsigned char));
  if (!file->currentData) {
    error (0, "Can't allocate memory for timestamp copy #3");
    return;
  }

  file->volume = vol;
  file->pos = 0;
  file->posInExtBlk = 0;
  file->posInDataBlk = 0;
  file->writeMode = 1;
  file->currentExt = NULL;
  file->nDataBlock = 0;

  memcpy(file->fileHdr, &entry, sizeof(struct bFileHeaderBlock));
  file->eof = TRUE;

  stat (filename, &st_buf);
  local = localtime (&st_buf.st_mtime);
  dt.year = local->tm_year;
  dt.mon  = local->tm_mon + 1;
  dt.day  = local->tm_mday;
  dt.hour = local->tm_hour;
  dt.min  = local->tm_min;
  dt.sec  = local->tm_sec;

  /* printf("filename is %s\n", filename); */
  /* printf("year, month, date is %d %d %d\n", dt.year, dt.mon, dt.day); */

  adfTime2AmigaTime(dt,
                    &(file->fileHdr->days),
                    &(file->fileHdr->mins),
                    &(file->fileHdr->ticks)
    );

  adfWriteFileHdrBlock(file->volume, file->fileHdr->headerKey, file->fileHdr);
  adfUpdateBitmap(file->volume);
}

/* validate and change directory within an adf-file */
/* input might be something like 'Work/Programming/Gfx/' */
int
adf_validate_directory (char *dir)
{
  /* backup the directory name */
  char *directory = strdup (dir);

  /* a single '/' means to copy to the root directory */
  if ((strlen (directory) == 1) &&
      (strcmp (directory, "/") == 0))
    free (directory);
    return 1;

  /* delete leading and trailing slashes (/) in the string (if any) */
  while (directory[strlen (directory) - 1] == '/')
    directory[strlen (directory) - 1] = '\0';
  while (directory[0] == '/')
    directory++;

  /* looks like there were more directories, check them all */
  if (strchr (directory, '/')) {
    char *tmp = strdup (directory);

    /* loop through the directories */
    while (splitc (tmp, directory)) {
      /* check if the adf-dir exists */
      if (adfChangeDir (volume, tmp) != RC_OK) {
        free (directory);
        free (tmp);
        return 0;
      }
    }

    free (tmp);
  }

  /* last element is left in 'directory' */
  if (adfChangeDir (volume, directory) != RC_OK)
    free (directory);
    return 0;

  /* all went fine. update the global sector variable */
  sector = volume->curDirPtr;

  free (directory);
  return 1;
}

/* mostly based on the one in adfmakedir.c */
void
adf_make_dir (const char *path)
{
  char *directory = strdup (path);

  if (!path || !strlen (path)) {
    /* directory is empty*/
    free (directory);
    return;
  }

  /* delete leading and trailing slashes (/) in the string (if any) */
  while (directory[strlen (directory) - 1] == '/')
    directory[strlen (directory) - 1] = '\0';
  while (directory[0] == '/')
    directory++;

  if ((strncmp (directory, "./", 2) == 0) &&
      (strlen (directory) > 2)) {
    /* skip ./ */
    adf_make_dir (directory + 2);
    free (directory);
    return;
  }

  /* we need two versions */
  if (!strchr (directory, '/')) {
    /*************************************************************************************/
    /* CASE 1: no subdirs specified (no "/" in dir name). create directory in root level */
    /*************************************************************************************/
    /* try to change to the dir first, to see if it already exists */
    if (adfChangeDir (volume, directory) == RC_OK) {
      notify ("Directory '%s' exists, skipping.\n", directory);
      sector = volume->curDirPtr;

      free (directory);
      return;
    }

    /* no subdirs, create in the root directory */
    if (adfCreateDir (volume, sector, directory) == RC_OK) {
      /* internally change to the dir we just created */
      if (adfChangeDir (volume, directory) != RC_OK)
        error (0, "Created directory '%s', but couldn't set it as working directory. Weird", directory);
      else
        notify ("Created directory '%s'.\n", directory);

      sector = volume->curDirPtr;
    } else
      error (0, "Could not create directory '%s', no idea why");
  } else {
    /***********************************************************/
    /* CASE 2: subdirs specified. strip the / and go recursive */
    /***********************************************************/
    char *tmp  = strdup (directory);
    char *tmp2 = strdup (directory);

    /* repeat as long as there is a / in the directory to be created */
    while (splitc (tmp, tmp2))
      continue;

    /* last element is left in 'directory' */
    adf_make_dir (tmp2);

    free (tmp);
    free (tmp2);
  }

  free (directory);
}

int
copy_file_to_adf (char *filename)
{
  struct File* file;
  FILE* in;
  long n, len;
  unsigned char buf[BUFSIZE];

  in = fopen (filename, "rb");
  if (!in) {
    /* adfCloseFile (file); */  // Is this needed?  /bos 2013-11-22
    /* add more error handling */
    error (0, "Can't open '%s' for reading: %s", filename, strerror (errno));
    return 0;
  };

  file = adfOpenFile (volume, basename (filename), "w");
  if (!file) {
    /* add more error handling */
    error (0, "Can't open '%s' for writing. No idea why, perhaps the file exists", filename);
    return 0;
  };

  len = BUFSIZE;
  n = fread (buf, sizeof (unsigned char), len, in);
  while (!feof (in)) {
    /* WARNING - adfWriteFile() DOES NOT REPORT ANY ERRORS IF THE DISK IS FULL! */
    adfWriteFile (file, n, buf);
    n = fread (buf, sizeof (unsigned char), len, in);
  }

  if (n > 0)
    adfWriteFile (file, n, buf);

  fclose (in);
  adfCloseFile (file);

  // adfUpdateBitmap() breaks!
  adf_copy_file_time (volume, filename);
  // adfUpdateBitmap() breaks!

  /* notify user what file got copied */
  notify ("%s -> %s:%s\n", filename, adf_path, filename);

  return 1;
}

/* checks in the directory really is a directory */
int
check_destination_dir (const char *directory)
{
  struct stat statbuf;

  if (lstat (directory, &statbuf) < 0) {
    error (0, "Could not stat destination directory: %s", strerror (errno));
    return 0;
  }

  if (!S_ISDIR (statbuf.st_mode))
    /* not a dir */
    return 0;
  else
    return 1;
}

/* allocate a buffer big enough to contain the biggest pathname possible */
char *
allocate_path ()
{
  char *ptr;

#ifndef PATH_MAX
  if (pathmax == 0) {
    /* first time */
    errno = 0;
    if ((pathmax = pathconf ("/", _PC_PATH_MAX)) < 0) {
      if (errno == 0) {
        /* no PATH_MAX available */
        pathmax = PATH_MAX_GUESS;
      } else {
        error (1, "Error while fetching PATH_MAX: %s", strerror (errno));
      }
    } else
      /* need to add one for the slash (PATH_MAX does not account for it) */
      pathmax++;
  }
#endif

  ptr = malloc (pathmax + 1);

  /* could be NULL */
  return ptr;
}

/* this function handles all entries, one by one */
static int
adf_ftw_callback (char *pathname, const struct stat *statptr, int type)
{
  switch (type) {
    /* not a directory */
    case ADF_FTW_F:
      switch (statptr->st_mode & S_IFMT) {
        case S_IFREG:
          /* regular file */
          copy_file_to_adf (pathname);
          n_files++;
          break;

        case S_IFDIR:
          /* directory */
          notify ("This should *not* happen. Blame Canada, then do a bug-report.\n");

        default:
          /* block special, links, sockets etc are ignored */
          notify ("Ignoring '%s', the file is not a regular file or directory.\n", pathname);
          break;
      }
      break;

    case ADF_FTW_D:
      /* directory */
      adf_make_dir (pathname);
      n_dirs++;
      break;

    case ADF_FTW_DNR:
      /* unreadable directory */
      error (0, "Can't read directory '%s': %s", pathname, strerror (errno));
      break;

    case ADF_FTW_NS:
      /* nonstatable file */
      error (0, "Can't stat '%s': %s", pathname, strerror (errno));
      break;

    default:
      /* error */
      error (0, "Unknown type %d for '%s'", type, pathname);
  }

  return 1;
}

/* descend through the hierarchy, starting at "fullpath". if "fullpath"
   is anything other than a directory, we lstat() it, call func() and
   return. for a directory, we call ourself recursively for each name
   in the directory */
static int
process_path (adf_ftw_t *func)
{
  char *ptr;
  DIR *dp;
  int ret;
  struct dirent *dirp;
  struct stat statbuf;

  if (lstat (fullpath, &statbuf) < 0)
    /* stat error */
    return (func (fullpath, &statbuf, ADF_FTW_NS));

  if (!S_ISDIR (statbuf.st_mode))
    /* not a directory */
    return (func (fullpath, &statbuf, ADF_FTW_F));

  /* it's a directory. first call func() for the directory, then process
     each filename in the directory */
  ret = func (fullpath, &statbuf, ADF_FTW_D);
  if (!ret)
    return ret;

  /* point to end of fullpath */
  ptr = fullpath + strlen (fullpath);
  *ptr++ = '/';
  *ptr = 0;

  if ((dp = opendir (fullpath)) == NULL)
    /* can't read directory */
    return (func (fullpath, &statbuf, ADF_FTW_DNR));

  /* process all entries in the directory */
  while ((dirp = readdir (dp)) != NULL) {
    /* ignore dot and dot-dot */
    if ((strcmp (dirp->d_name, ".")  == 0) ||
        (strcmp (dirp->d_name, "..") == 0))
      continue;

    /* append name after slash */
    strcpy (ptr, dirp->d_name);

    /* recursive */
    ret = process_path (func);
    if (!ret)
      /* something went wrong, time to quit */
      break;
  }

  /* erase everything from the slash onwards */
  ptr[-1] = 0;

  if (closedir (dp) < 0)
    error (0, "Can't close directory '%s': %s", fullpath, strerror (errno));

  /* we're back from the directory. go to the parent directory in the adf
     and update the sector counter */
  adfParentDir (volume);
  sector = volume->curDirPtr;

  return ret;
}

/* entry function */
static int
adf_ftw (char *pathname, adf_ftw_t *func)
{
  /* allocate PATH_MAX+1 bytes */
  fullpath = allocate_path ();
  if (!fullpath)
    error (1, "Can't allocate memory for pathname: %s", strerror (errno));

  strcpy (fullpath, pathname);

  /* we return whatever func() returns */
  return (process_path (func));
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
    printf ("Usage: %s ADF-FILE FILE(s) DIR(s) ADF-DIRECTORY\n", program_name);
    printf ("Copy file(s) to an adf-image.\n\n");
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
  char *adf_image;
  int c;
  int n_args;

  init_adflib();

  /* parse the options */
  while ((c = getopt_long (argc, argv, "hfV", long_options, NULL)) != -1) {
    switch (c) {
      case 0:
        break;

      case 'h':
        print_usage (1);
        break;

      case 'f':
        opt_force = 1;
        break;

      case 'V':
        print_version ();
        exit (0);

      default:
        print_usage (0);
    }
  }

  /* first argument is (should be) the adf-file */
  adf_image = argv[optind++];

  n_args = argc - optind;
  if (n_args < 0) {
    /* no args at all */
    error (0, "No adf-file specified");
    print_usage (0);
  } else if (n_args < 2) {
    /* adf-file exists, but not at least one file and no destination */
    error (0, "Too few arguments");
    print_usage (0);
  }

  /* mount the adf-file */
  if (!mount_adf (adf_image, &device, &volume, READ_WRITE))
    exit(1);

  adf_path = allocate_path ();
  if (!adf_path)
    error (1, "Can't allocate memory: %s", strerror (errno));

  strcpy (adf_path, basename (adf_image));

  if (optind < argc) {
    char *adf_destination = argv[--argc];
    int ret;

    /* make sure the selected destination in the image exists */
    ret = adf_validate_directory (adf_destination);
    if (!ret && !opt_force)
      error (1, "No such directory in the adf-file: '%s'", adf_destination);

    sector = volume->curDirPtr;
    while (optind < argc) {
      char *file = argv[optind++];

      if (check_destination_dir (file)) {
        /* file is a dir, go recursive */
        ret = adf_ftw (file, adf_ftw_callback);
      } else {
        /* file is a file */
        copy_file_to_adf (file);
      }
    }
  } else {
    error (1, "Something's wrong");
  }

  printf ("All Done.\n");

  cleanup_adflib();
  return 1;
}
