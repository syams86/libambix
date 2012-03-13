/* matrix.c -  Matrix handling              -*- c -*-

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

#include "private.h"

#ifdef HAVE_STDLIB_H
# include <stdlib.h>
#endif /* HAVE_STDLIB_H */

ambixmatrix_t*
ambix_matrix_create(void) {
  return ambix_matrix_init(0, 0, NULL);
}
void
ambix_matrix_destroy(ambixmatrix_t*mtx) {
  ambix_matrix_deinit(mtx);
  free(mtx);
  mtx=NULL;
}
void
ambix_matrix_deinit(ambixmatrix_t*mtx) {
  uint32_t r;
  if(mtx->data) {
    for(r=0; r<mtx->rows; r++) {
      if(mtx->data[r])
        free(mtx->data[r]);
      mtx->data[r]=NULL;
    }
  }
  free(mtx->data);
  mtx->data=NULL;
  mtx->rows=0;
  mtx->cols=0;
}
ambixmatrix_t*
ambix_matrix_init(uint32_t rows, uint32_t cols, ambixmatrix_t*orgmtx) {
  ambixmatrix_t*mtx=orgmtx;
  uint32_t r;
  if(!mtx) {
    mtx=(ambixmatrix_t*)calloc(1, sizeof(ambixmatrix_t));
    if(!mtx)
      return NULL;
    mtx->data=NULL;
  }
  ambix_matrix_deinit(mtx);

  mtx->rows=rows;
  mtx->cols=cols;
  if(rows>0 && cols > 0) {
    mtx->data=(float32_t**)calloc(rows, sizeof(float32_t*));
    for(r=0; r<rows; r++) {
      mtx->data[r]=(float32_t*)calloc(cols, sizeof(float32_t));
    }
  }
  
  return mtx;
}
int
ambix_matrix_fill(ambixmatrix_t*mtx, float32_t*data) {
  float32_t**matrix=mtx->data;
  uint32_t rows=mtx->rows;
  uint32_t cols=mtx->cols;
  uint32_t r;
  for(r=0; r<rows; r++) {
    uint32_t c;
    for(c=0; c<cols; c++) {
      matrix[r][c]=*data++;
    }
  }
  return AMBIX_ERR_SUCCESS;
}

int
ambix_matrix_fill_swapped(ambixmatrix_t*mtx, number32_t*data) {
  float32_t**matrix=mtx->data;
  uint32_t rows=mtx->rows;
  uint32_t cols=mtx->cols;
  uint32_t r;
  for(r=0; r<rows; r++) {
    uint32_t c;
    for(c=0; c<cols; c++) {
      number32_t v;
      number32_t d = *data++;
      v.i=swap4(d.i);
      matrix[r][c]=v.f;
    }
  }
  return AMBIX_ERR_SUCCESS;
}



ambixmatrix_t*
ambix_matrix_copy(const ambixmatrix_t*src, ambixmatrix_t*dest) {
  if(!src)
    return NULL;
  if(!dest)
    dest=(ambixmatrix_t*)calloc(1, sizeof(ambixmatrix_t));

  if((dest->rows != src->rows) || (dest->cols != src->cols))
    ambix_matrix_init(src->rows, src->cols, dest);

  do {
    uint32_t r, c;
    float32_t**s=src->data;
    float32_t**d=dest->data;
    for(r=0; r<src->rows; r++) {
      for(c=0; c<src->cols; c++) {
        d[r][c]=s[r][c];
      }
    }
  } while(0);

  return dest;
}


ambixmatrix_t*
ambix_matrix_multiply(const ambixmatrix_t*left, const ambixmatrix_t*right, ambixmatrix_t*dest) {
  uint32_t r, c, rows, cols, common;
  float32_t**ldat,**rdat,**ddat;
  if(!left || !right)
    return NULL;

  if(left->cols != right->rows)
    return NULL;

  if(!dest)
    dest=(ambixmatrix_t*)calloc(1, sizeof(ambixmatrix_t));

  if((dest->rows != left->rows) || (dest->cols != right->cols))
    ambix_matrix_init(left->rows, right->cols, dest);

  rows=dest->rows;
  cols=dest->cols;
  common=left->cols;

  ldat=left->data;
  rdat=right->data;
  ddat=dest->data;

  for(r=0; r<rows; r++)
    for(c=0; c<cols; c++) {
      float32_t sum=0.;
      uint32_t i;
      for(i=0; i<common; i++)
        sum+=ldat[r][i]*rdat[i][c];
      ddat[r][c]=sum;
    }

  return dest;
}


ambixmatrix_t*
ambix_matrix_eye(ambixmatrix_t*matrix) {
  int32_t rows=matrix->rows;
  int32_t cols=matrix->cols;
  float32_t**mtx=matrix->data;

  int32_t r, c;
  for(r=0; r<rows; r++)
    for(c=0; c<cols; c++)
      mtx[r][c]=(r==c)?1.:0.;

  return matrix;
}


ambix_err_t ambix_matrix_multiply_float32(float32_t*dest, const ambixmatrix_t*matrix, const float32_t*source, int64_t frames) {
  float32_t**mtx=matrix->data;
  const uint32_t rows=matrix->rows;
  const uint32_t cols=matrix->cols;
  int64_t f;
  for(f=0; f<frames; f++) {
    uint32_t chan;
    for(chan=0; chan<rows; chan++) {
      const float32_t*src=source;
      float32_t sum=0.;
      uint32_t c;
      for(c=0; c<cols; c++) {
        sum+=mtx[chan][c] * src[c];
      }
      *dest++=sum;
      source+=cols;
    }
  }
  return AMBIX_ERR_SUCCESS;
}


#define MTXMULTIPLY_DATA_INT(typ)                                       \
  ambix_err_t ambix_matrix_multiply_##typ(typ##_t*dest, const ambixmatrix_t*matrix, const typ##_t*source, int64_t frames) { \
    float32_t**mtx=matrix->data;                                        \
    const uint32_t rows=matrix->rows;                                   \
    const uint32_t cols=matrix->cols;                                   \
    int64_t f;                                                          \
    for(f=0; f<frames; f++) {                                           \
      uint32_t chan;                                                    \
      for(chan=0; chan<rows; chan++) {                                  \
        const typ##_t*src=source;                                       \
        float32_t sum=0.;                                               \
        uint32_t c;                                                     \
        for(c=0; c<cols; c++) {                                         \
          sum+=mtx[chan][c] * src[c];                                   \
        }                                                               \
        *dest++=sum;                                                    \
        source+=cols;                                                   \
      }                                                                 \
    }                                                                   \
    return AMBIX_ERR_SUCCESS;                                           \
  }


MTXMULTIPLY_DATA_INT(int16);
MTXMULTIPLY_DATA_INT(int32);
