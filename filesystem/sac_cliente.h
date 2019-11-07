#ifndef SAC_CLIENT_H
#define SAC_CLIENT_H

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <fuse.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <commons/log.h>
#include "message.h"
#include "network.h"


//variables globales
t_log *logger;
int master_socket;

//functions

int send_call(Function *f);
int send_path(FuncType func_type, const char *path);

// Definition of functions to be implemented //

// Reading functions //
/*
 * @DESC
 *  This function will be called when the FUSE library receives an order to obtain
 *   the metadata of a file / directory. This can be size, type, permits, owner, etc ..
 *
 * @PARAMETERS
 * 		path - The path is relative to the mount point and is the way by which we must
 * 		 find the file or directory that we are requested
 * 		stbuf - This structure is what we must complete
 *
 * 	@RETURN
 * 		O file/directory found. -ENOENT file/directory found not found
 *
 * 	@PERMISSIONS
 * 		If it is a directory and must has permissions:
 * 			stbuf->st_mode = S_IFDIR | 0777;
 * 			stbuf->st_nlink = 2;
 * 		If it is a file:
 * 			stbuf->st_mode = S_IFREG | 0777;
 * 			stbuf->st_nlink = 1;
 * 			stbuf->st_size = [SIZE];
 *
 */
int sac_cliente_getattr(const char *path, struct stat *stbuf);

/*
 * @DESC
 *  This function will be called when the FUSE library takes you a request to
 *  obtain the list of files or directories that are within a directory
 *
 * @PARAMETERS
 * 		path - The path is relative to the mount point and is the way by which we must
 * 		 find the file or directory that we are requested
 * 		buf - This is a buffer where the names of the files and directories that are inside
 * 		 the directory indicated by the path will be placed
 * 		filler - This is a pointer to a function, which knows how to save a string inside the buf field
 *
 * 	@RETURN
 * 		O directory found. -ENOENT directory not found
 */
int sac_cliente_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi) //

/*
 * @DESC
 *  This function will be called when the FUSE library takes you a request to obtain the
 *   contents of a file
 *
 * @PARAMETERS
 * 		path - The path is relative to the mount point and is the way by which we must
 * 		 find the file or directory that we are requested
 * 		buf - This is the buffer where the requested content will be saved
 * 		size - How much bytes we have to read
 * 		offset - Indicates from what position of the file we have to read
 *
 * 	@RETURN
 * 		If the direct_io parameter is used, the return values ​​are 0 if the file was found
 * 		or -ENOENT if an error occurred. If the direct_io parameter is not present, the return values
 * 		​​are the number of bytes read if everithing ok or -ENOENT if an error occurred.
 */
int sac_cliente_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi);

/* * @DESC
 *  Esta función va a ser llamada cuando a la biblioteca de FUSE le llege un pedido
 * para tratar de abrir un archivo
 *
 * @PARAMETROS
 * 		path - El path es relativo al punto de montaje y es la forma mediante la cual debemos
 * 		       encontrar el archivo o directorio que nos solicitan
 * 		fi - es una estructura que contiene la metadata del archivo indicado en el path
 *
 * 	@RETURN
 * 		O archivo fue encontrado. -EACCES archivo no es accesible*/

int sac_cliente_open(const char *path, struct fuse_file_info *fi);

// Writing functions //

/*
 *  @ DESC
 * 		This function will create a directory in the filesystem
 *
 * 	@ PARAM
 *
 * 		-path: The path where the directory will be created
 *
 * 		-mode: It contains the permissions that the directory must have and other metadata
 *
 * 	@ RET
 * 		It returns 0 on success, or -1 if an error ocurred
 */
int sac_cliente_mkdir(const char *pathname, mode_t mode);

/*
 *  @DESC
 *  	This function will create a file in the filesystem
 *
 *  @PARAM
 *  	path - The path where the directory will be created
 *  	mode - File's options
 *  	dev - Parameter not used
 *
 *  @RET
 *  	It returns 0 on success, or -1 if an error ocurred
 */
int sac_cliente_mknod (const char *pathname, mode_t mode, dev_t dev);

/*
 *	@DESC
 *		This function will remove a directory of the filesystem
 *
 *	@PARAM
 *		Path - The path of the directory to be removed
 *
 *	@RET
 *		It returns 0 on success, or -ENOENT if an error ocurred
 *
 */
int sac_cliente_rmdir(const char *pathname);

/*
 *  @DESC
 *  	This function will remove a file of the filesystem
 *
 *  @PARAM
 *  	path - The path of the directory to be removed
 *
 *  @RET
 *  	It returns 0 on success, or -ENOENT if an error ocurred
 */
int sac_cliente_unlink(const char *pathname);

/*
 * 	@DESC
 * 		Function to write files
 *
 * 	@PARAM
 * 		path - The path of the file
 * 		buf - Buffer that contains the data to be copied
 * 		size - Size of the data to be copied
 * 		offset - Indicates from what position of the file we have to write
 * 		fi - File info. Contains flags and other things not used
 *
 * 	@RET
 * 		It returns the size of bytes writeen on success, or a negative number
 * 		if an error ocurred, and errno is set appropriately.
 */
int sac_cliente_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi);

#endif
