#ifndef ADFTOOLS_MISC_H
#define ADFTOOLS_MISC_H 1

#include <adflib.h>

#define BUFSIZE 1024

#define BOOTBLOCK_SIZE LOGICAL_BLOCK_SIZE*2
#define TRACKS 80
#define SECTORS 11
#define HEADS 2

#ifdef WIN32
#define DIRSEP '\\'
#define MKDIR "md"
#else
#define DIRSEP '/'
#define MKDIR "mkdir"
#endif

#define ADF_FTW_F   1 /* anything but a directory */
#define ADF_FTW_D   2 /* directory */
#define ADF_FTW_DNR 3 /* unreadable directory */
#define ADF_FTW_NS  4 /* file that we can't stat */

#define READ_ONLY 1
#define READ_WRITE 0

/* if PATH_MAX is indeterminatable, take a guess */
#define PATH_MAX_GUESS 1024

/* needed for the callback-handler by libadf */
ENV_DECLARATION;

int isdigits (char *str);
char *strip_extension (char *str);
char *basename (char *filename);
char *dirname (char *filename);
char *strip_trailing_slashes (char *path);
int is_adf_file (unsigned char *buf);
char *get_adf_dostype (char dostype);
int mount_adf (char *filename, struct Device **dev, struct Volume **vol, int rw);
void print_volume_header (char *filename, struct Volume *volume);
void init_adflib (void);
void cleanup_adflib (void);
char *access2str (long access);
void change_to_root_dir (struct Volume *volume);
char *splitc (char *first, char *rest);
unsigned char *allocate_bootblock_buf (void);
unsigned char *read_bootblock (char *filename);

#endif /* ADFTOOLS_MISC_H */
