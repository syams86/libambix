/* ambix/ambix.h -  Ambisonics Xchange Library Interface              -*- c -*-

   Copyright © 2012 IOhannes m zmölnig <zmoelnig@iem.at>.
         Institute of Electronic Music and Acoustics (IEM),
         University of Music and Dramatic Arts, Graz

   This file is part of libambix

   libambix is free software; you can redistribute it and/or modify
   it under the terms of the GNU Lesser General Public License as
   published by the Free Software Foundation; either version 2.1 of
   the License, or (at your option) any later version.

   Libgcrypt is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this program; if not, see <http://www.gnu.org/licenses/>.

*/

/**
 * @file	ambix/ambix.h
 * @brief	Ambisonics Xchange Library Interface
 * @details This file is part of libambix
 * @author IOhannes m zmölnig <zmoelnig@iem.at>
 * @date 2012
 * @copyright LGPL-2.1
 */
#ifndef AMBIX_AMBIX_H
#define AMBIX_AMBIX_H

#include "exportdefs.h"
#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif	/* __cplusplus */


/** @brief Initialize a matrix
 *
 * Allocates memory for matrix-data of given dimensions
 *
 * @param rows number of rows in the newly initialized matrix
 * @param cols number of columns in the newly initialized matrix
 * @param mtx pointer to a matrix object; if NULL a new matrix object will be created, else the given matrix object will be re-initialized
 * @return pointer to a newly initialized (and/or allocated) matrix, or NULL on error
 */
AMBIX_API
ambixmatrix_t*ambix_matrix_init(uint32_t rows, uint32_t cols, ambixmatrix_t*mtx);


/** @brief De-initialize a matrix
 *
 * Frees associated ressources and sets rows/columns to 0
 *
 * @param mtx matrix object to deinitialize
 */
AMBIX_API
void ambix_matrix_deinit(ambixmatrix_t*mtx);


/** @brief Fill a matrix with values
 *
 * Fill data into a properly initialized matrix
 *
 * @param mtx initialized matrix object to copy data into
 * @param data pointer to at least (mtx->rows*mtx->cols) values; data is ordered row-by-row with no padding (A[0,0], A[0,1], .., A[0,cols-1],  A[1, 0], .. A[rows-1, cols-1])
 * @return an error code indicating success
 */
AMBIX_API
int ambix_matrix_fill(ambixmatrix_t*mtx, float32_t*data);


/** @brief Fill a matrix with byteswapped values
 *
 * Fill byteswapped data into a properly initialized matrix
 *
 * @param mtx initialized matrix object to copy data into
 * @param data pointer to at least (mtx->rows*mtx->cols) values; data is ordered row-by-row with no padding (A[0,0], A[0,1], .., A[0,cols-1],  A[1, 0], .. A[rows-1, cols-1])
 *           each data item has to be byteswapped
 * @return an error code indicating success
 */
AMBIX_API
int ambix_matrix_fill_swapped(ambixmatrix_t*mtx, number32_t*data);


/** @brief Copy a matrix to another matrix
 *
 * Copy a matrix, possibly resizing or creating the destination
 *
 * @param src the source matrix to copy the data from
 * @param dest the destination matrix (if NULL a new matrix will be created)
 * @return pointer to the destination matrix
 */
AMBIX_API
ambixmatrix_t*ambix_matrix_copy(const ambixmatrix_t*src, ambixmatrix_t*dest);
/** @brief Multiplay two matrices
 *
 * Multiply matrices dest=A*B, possibly resizing or creating the destination matrix
 *
 * @param A left-hand operator
 * @param B right-hand operator
 * @param result pointer to the matrix object that will hold the result or NULL
 * @return pointer to the result matrix, or NULL in case the matrix multiplication did not succeed
 */
AMBIX_API
ambixmatrix_t*ambix_matrix_multiply(const ambixmatrix_t*A, const ambixmatrix_t*B, ambixmatrix_t*result);

/** @brief Calculate the number of channels for a full 3d ambisonics set of a given order
 *
 * @param order the order of the full set
 * @return the number of channels of the full set
 */
AMBIX_API
uint32_t ambix_order2channels(uint32_t order);


/** @brief Calculate the order of a full 3d ambisonics set fora gien number of channels
 *
 * @param channels the number of channels of the full set
 * @return the order of the full set, or -1 if the channels don't form a full set
 */
AMBIX_API


int32_t ambix_channels2order(uint32_t channels);
/** @brief Checks whether the channel can form a full 3 ambisonics set
 *
 * @param channels the number of channels supposed to form a full set
 * @return TRUE if the channels can form full set, FALSE otherwise
 */
AMBIX_API
int ambix_isFullSet(uint32_t channels);




/** @brief Open an ambix file
 *
 * Opens a soundfile for reading/writing
 *
 * @param path filename of the file to open
 * @param mode whether to open the file for reading and/or writing (AMBIX_READ, AMBIX_WRITE, AMBIX_READ | AMBIX_WRITE)
 * @param ambixinfo pointer to a valid ambixinfo_t structure
 * @remark
 *    when opening a file for reading, the structure should be initialized to zero before calling ambix_open():
 *    the fields will be set by the library; if you set the ambixinfo_t.ambixformat field to something else than AMBIX_NONE,
 *    the library will present the data as if the was written in this format (e.g. if you set ambixinfo_t.ambixformat:=AMBIX_SIMPLE
 *    but the file really is AMBIX_EXTENDED, the library will automatically pre-multiply the reconstruction matrix to
 *    give you the full ambisonics set.
 * @remark
 *   when opening a file for writing, the caller must set the fields; if ambixinfo_t.ambixformat is AMBIX_NONE, than ambixinfo_t.ambixchannels must be 0,
 *   else ambixinfo_t.ambichannels must be >0; if ambixinfo_t.ambixformat is AMBIX_SIMPLE, then ambixinfo_t.ambichannels must be (ambiorder+1)^2
 * @return A handle to the opened file (or NULL on failure)
 */
AMBIX_API
ambix_t* 	ambix_open	(const char *path, const ambix_filemode_t mode, ambixinfo_t*ambixinfo) ;

/** @brief Close an ambix handle
 *
 * Closes an ambix handle and cleans up all memory allocations associated with it.
 *
 * @param ambix The handle to an ambix file
 * @return an error code indicating success
 */
AMBIX_API
ambix_err_t	ambix_close	(ambix_t*ambix);

/** @brief Read samples from the ambix file
 *
 * reads samples (as 16bit signed integer values) from an ambix file,
 * possibly expanding a reduced channel set to a full ambisonics set (when reading an 'ambix extended' file as 'ambix simple')
 *
 * @param ambix The handle to an ambix file
 * @param ambidata pointer to user allocated array to retrieve ambisonics channels into;
 *        must be large enough to hold at least (frames*ambix->ambichannels) samples, OR
 *        if you successfully added a pre-multiplication matrix using ambix_setPreMultiplyMatrix()
 *        the array must be large enough to hold at least (frames * premultmatrix.rows) samples
 * @param otherdata pointer to user allocated array to retrieve non-ambisonics channels into
 *        must be large enough to hold at least (frames*ambix->otherchannels) samples
 * @param frames number of sample frames you want to read
 * @return the number of sample frames sucessfully read
 */
AMBIX_API
int64_t ambix_readf_int16   (ambix_t*ambix, int16_t*ambidata, int16_t*otherdata, int64_t frames) ;
/** @brief Read samples from the ambix file
 *
 * reads samples (as 32bit signed integer values) from an ambix file,
 * possibly expanding a reduced channel set to a full ambisonics set (when reading an 'ambix extended' file as 'ambix simple')
 *
 * @param ambix The handle to an ambix file
 * @param ambidata pointer to user allocated array to retrieve ambisonics channels into;
 *        must be large enough to hold at least (frames*ambix->ambichannels) samples, OR
 *        if you successfully added a pre-multiplication matrix using ambix_setPreMultiplyMatrix()
 *        the array must be large enough to hold at least (frames * premultmatrix.rows) samples
 * @param otherdata pointer to user allocated array to retrieve non-ambisonics channels into
 *        must be large enough to hold at least (frames*ambix->otherchannels) samples
 * @param frames number of sample frames you want to read
 * @return the number of sample frames sucessfully read
 */
AMBIX_API
int64_t ambix_readf_int32   (ambix_t*ambix, int32_t *ambidata, int32_t*otherdata, int64_t frames) ;
/** @brief Read samples from the ambix file
 *
 * reads samples (as single precision floating point values) from an ambix file,
 * possibly expanding a reduced channel set to a full ambisonics set (when reading an 'ambix extended' file as 'ambix simple')
 *
 * @param ambix The handle to an ambix file
 * @param ambidata pointer to user allocated array to retrieve ambisonics channels into;
 *        must be large enough to hold at least (frames*ambix->ambichannels) samples, OR
 *        if you successfully added a pre-multiplication matrix using ambix_setPreMultiplyMatrix()
 *        the array must be large enough to hold at least (frames * premultmatrix.rows) samples
 * @param otherdata pointer to user allocated array to retrieve non-ambisonics channels into
 *        must be large enough to hold at least (frames*ambix->otherchannels) samples
 * @param frames number of sample frames you want to read
 * @return the number of sample frames sucessfully read
 */
AMBIX_API
int64_t ambix_readf_float32   (ambix_t*ambix, float32_t *ambidata, float32_t*otherdata, int64_t frames) ;

/** @brief get the libsndfile handle associated with the ambix handle
 *
 * If possible, require an SNDFILE handle if possible;
 * if the ambix handle is not asociated with SNDFILE (e.g. because libambix is compiled without libsndfile support),
 * NULL is returned
 *
 * @param ambix The handle to an ambix file
 * @return A libsndfile handle or NULL
 */
AMBIX_API
SNDFILE*ambix_getSndfile	(ambix_t*ambix);


/** @brief get the adaptor matrix
 *
 * the ambix extended fileformat comes with a adaptor matrix, that can be used
 * to reconstruct a full 3d ambisonics set from the channels stored in ambix file.
 * in the ambix simple format no adaptor matrix is present, the file contains the full set.
 * @remark the adaptor matrix can only be obtained for a ambix extended file; if you have opened
 *          an ambix extended file as "ambix simple", the adaptor will be done by the library;
 *          in this case, you will not be able to fetch the adaptor matrix.
 * @remark if you have opened an ambix simple file as ambix extended, this will return a unity matrix
 *
 * @param ambix The handle to an ambix file
 * @return the adaptor matrix to restore the full ambisonics set from the reduced set, or NULL
 *          if there is no such matrix; the memory is owned by the library and must neither be freed
 *          nor used after calling ambix_close()
 */
AMBIX_API
const ambixmatrix_t*ambix_getAdaptorMatrix	(ambix_t*ambix);


/** @brief set a matrix to be pre-multiplied
 *
 * tells the library to do an (additional) matrix-multiplication when reconstructing the full ambisonics set (read mode!);
 * you can use use this to get the ambisonics channels in a format other than SN3D/ACN (e.g. using an ambix to Furse-Malham adaptor matrix)
 * or getting the loudspeaker feeds directly (by supplying a decoder matrix);
 * the matrix MUST have ambix->ambichannels columns
 *
 * @param ambix The handle to an ambix file
 * @param matrix a matrix that will be pre-multiplied to the reconstruction-matrix;
 *          can be freed after this call
 * @return an errorcode indicating success
 * @remark this is only meaningful for reading files as 'ambix simple'
 */
AMBIX_API
ambix_err_t ambix_setPremultiplyMatrix	(ambix_t*ambix, const ambixmatrix_t*matrix);






#ifdef __cplusplus
}		/* extern "C" */
#endif	/* __cplusplus */
#endif /* AMBIX_AMBIX_H */
