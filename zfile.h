 /*
  * UAE - The Un*x Amiga Emulator
  *
  * routines to handle compressed file automatically
  *
  * (c) 1996 Samuel Devulder
  */

extern struct zfile *zfile_open(const char *, const char *, unsigned short re_compress);
extern FILE *f_zfile_open(const char *, const char *, unsigned short re_compress);
extern char *n_zfile_open(const char *, const char *, unsigned short re_compress);
extern int zfile_close(FILE *);
extern void zfile_exit(void);
