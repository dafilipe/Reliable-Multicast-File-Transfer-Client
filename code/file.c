/*****************************************************************************\
 * Redes Integradas de Telecomunicacoes
 * MIEEC/MEEC/MERSIM - FCT NOVA  2025/2026
 *
 * file.c
 *
 * Functions that handle the shared file list GUI and file management
 *
 * @author  Luis Bernardo
\*****************************************************************************/

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>
#include <assert.h>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include "gui.h"



// Creates a directory and sets permissions that allow creation of new files
gboolean make_directory(const char *dirname) {
  return !mkdir(dirname, S_IRUSR|S_IWUSR|S_IXUSR|S_IRGRP|S_IWGRP|S_IXGRP|S_IROTH|S_IWOTH|S_IXOTH)
	 || (errno==EEXIST);
}


// Extracts the directory name from a full path name
const char *get_trunc_filename(const char *FileName) {
  char *pt= strrchr(FileName, (int)'/');
  if (pt != NULL)
    return pt+1;
  else
    return FileName;
}


// Returns the file length
unsigned long long get_filesize(const char *FileName)
{
  struct stat file;
  if(!stat(FileName, &file))
  {
    return (unsigned long long)file.st_size;
  }
  return 0;
}


// Returns a XOR HASH value for the contents of a file
unsigned int fhash(FILE *f) {
  assert(f != NULL);
  rewind(f);
  u_int sum= 0;
  int aux= 0;
  while (fread(&aux, 1, sizeof(int), f) > 0)
      sum ^= aux;
  return sum;
}


// Returns a XOR HASH value for the contents of a file
unsigned int fhash_filename(const char *FileName) {
	FILE *f= fopen(FileName, "r");
	if (f == NULL)
		return 0;
	unsigned long result= fhash(f);
	fclose(f);
	return result;
}


