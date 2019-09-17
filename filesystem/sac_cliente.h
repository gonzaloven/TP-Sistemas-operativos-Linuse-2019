#ifndef SAC_CLIENT_H
#define SAC_CLIENT_H

#include<stdio.h>

// Definition of functions to be implemented
// Todavia no se si los parametros van a quedar asi o nos va a convenir
// definirlos diferente, esto es un copy paste de las funciones posta

// Reading functions
ssize_t linuse_read(int fd, void *buf, size_t count); // read bits of a file

// Writing functions
int linuse_mkdir(const char *pathname, mode_t mode); // make a directory
int linuse_mknod (const char *pathname, mode_t mode, dev_t dev); // make a file
int linuse_rmdir(const char *pathname); // delete a directory
int linuse_unlink(const char *pathname); // delete a file
ssize_t linuse_write(int fd, const void *buf, size_t count); // write bits of a file



#endif
