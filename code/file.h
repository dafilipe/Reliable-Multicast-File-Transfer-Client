/*****************************************************************************\
 * Redes Integradas de Telecomunicacoes
 * MIEEC/MEEC/MERSIM - FCT NOVA  2025/2026
 *
 * file.h
 *
 * Header file of functions that handle the shared file list GUI and file management
 *
 * @author  Luis Bernardo
\*****************************************************************************/
#ifndef FILE_H_
#define FILE_H_

// Creates a directory and sets permissions that allow creation of new files
gboolean make_directory(const char *dirname);

// Extracts the directory name from a full path name
const char *get_trunc_filename(const char *FileName);

// Returns the file length
unsigned long long get_filesize(const char *FileName);

// Returns a XOR HASH value for the contents of a file
unsigned long fhash(FILE *f);

// Returns a XOR HASH value for the contents of a file
unsigned long fhash_filename(const char *FileName);

#endif
