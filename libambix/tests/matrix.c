/* matrix - test matrix functionality

   Copyright © 2012 IOhannes m zmölnig <zmoelnig@iem.at>.
         Institute of Electronic Music and Acoustics (IEM),
         University of Music and Dramatic Arts, Graz

   This file is part of libambix

   libambix is free software; you can redistribute it and/or modify
   it under the terms of the GNU Lesser General Public License as
   published by the Free Software Foundation; either version 2.1 of
   the License, or (at your option) any later version.

   libambix is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this program; if not, see <http://www.gnu.org/licenses/>.

*/

#include "common.h"
#include <string.h>

static float32_t leftdata_4_3[]= {
   0.19, 0.06, 0.14,
   0.05, 0.08, 0.44,
   0.25, 0.90, 0.77,
   0.83, 0.51, 0.58,
};
static float32_t leftdata_3_4[]= {
  /* just the transposition of leftdata[4,3] */
  0.19, 0.05, 0.25, 0.83,
  0.06, 0.08, 0.90, 0.51,
  0.14, 0.44, 0.77, 0.58,
};
static float32_t leftdata_4_4[]= {
  0.7131185686247925, 0.1054799265939327, 0.1882023608287114, 0.1496964665104298,
  0.9035382689904633, 0.0958506183093942, 0.1490156537909140, 0.6730762573692578,
  0.7110257215280688, 0.4278857180785819, 0.5050723092090162, 0.2342525090113509,
  0.1917073427152419, 0.3837280931544647, 0.0397484032568303, 0.5895499716980565,
};
static float32_t rightdata_3_2[]= {
   0.22, 0.46,
   0.36, 0.53,
   0.77, 0.85,
};
static float32_t resultdata_4_2[]= {
  /* leftdata[4,3] * rightdata[3,2] */
   0.1712, 0.2382,
   0.3786, 0.4394,
   0.9719, 1.2465,
   0.8128, 1.1451,
};
static float32_t resultpinv_4_3[] = {
  /* (leftdata[4,3])^-1 */
 0.320059925923633, -0.616572833442599, -0.758203952544301,  1.397070173352668,
-0.475395139478048, -2.112396458091558,  1.443108803981482, -0.198593134445739,
 0.349719602203337,  2.683861685335670, -0.150602340058839, -0.196372558639406,
};
static float32_t resultpinv_3_4[] = {
  /* (leftdata[3,4])^-1 */
   0.320059925923633, -0.475395139478048,  0.349719602203337,
  -0.616572833442600, -2.112396458091559,  2.683861685335671,
  -0.758203952544302,  1.443108803981483, -0.150602340058840,
   1.397070173352670, -0.198593134445740, -0.196372558639407,
};
static float32_t resultpinv_4_4[] = {
  /* (leftdata[4,4])^-1 */
  3.260687761423661,-0.750604578051301,-1.027983295100203, 0.437466478433052,
  4.478583197847279,-3.527343133136301,-0.883177052786819, 3.240826676373624,
 -6.751824651655014, 2.959470009112655, 3.874801812795508,-3.203980734335171,
 -3.520111656963465, 2.340434097800232, 0.647874863173282,-0.339426118611342,
};
static void mtxinverse_test(ambix_matrix_t *mtx, ambix_matrix_t *result, float32_t eps) {
  ambix_matrix_t *pinv = 0;
  ambix_matrix_t *mul=0;
  ambix_matrix_t *eye=0;
  float32_t errf;
  int min_rowcol=(mtx->cols<mtx->rows)?mtx->cols:mtx->rows;

  fail_if((NULL==mtx), __LINE__, "cannot invert NULL-matrix");
  fail_if((NULL==result), __LINE__, "cannot invert to NULL-matrix");
  eye=ambix_matrix_init(min_rowcol, min_rowcol, eye);
  eye=ambix_matrix_fill(eye, AMBIX_MATRIX_IDENTITY);
  fail_if((NULL==eye), __LINE__, "cannot create eye-matrix for pinv-verification");

  pinv=ambix_matrix_pinv(mtx, pinv);
  fail_if((NULL==pinv), __LINE__, "could not invert matrix");

  if(mtx->cols < mtx->rows)
    mul=ambix_matrix_multiply(pinv, mtx, 0);
  else
    mul=ambix_matrix_multiply(mtx, pinv, 0);

#if 0
  matrix_print(mtx);
  matrix_print(pinv);
  matrix_print(mul);
  //matrix_print(result);
  printf("------------\n");
#endif

  errf=matrix_diff(__LINE__, pinv, result, eps);
  fail_if((errf>eps), __LINE__, "diffing (pseudo)inverse returned %g (>%g)", errf, eps);

  errf=matrix_diff(__LINE__, mul, eye, eps);
  fail_if((errf>eps), __LINE__, "diffing mtx*pinv(mtx) returned %g (>%g)", errf, eps);

  ambix_matrix_destroy(pinv);
  ambix_matrix_destroy(mul);
  ambix_matrix_destroy(eye);
}
void mtxinverse_tests(float32_t eps) {
  float32_t errf;
  ambix_matrix_t *mtx, *testresult;
  STARTTEST("");

  /* fill in some test data 4x4 */
  mtx=ambix_matrix_init(4, 4, mtx);
  ambix_matrix_fill_data(mtx, leftdata_4_4);
  testresult=ambix_matrix_init(4, 4, testresult);
  ambix_matrix_fill_data(testresult, resultpinv_4_4);
  mtxinverse_test(mtx, testresult, eps);

  /* fill in some test data 4x3 */
  mtx=ambix_matrix_init(4, 3, mtx);
  ambix_matrix_fill_data(mtx, leftdata_4_3);
  testresult=ambix_matrix_init(3, 4, testresult);
  ambix_matrix_fill_data(testresult, resultpinv_4_3);
  mtxinverse_test(mtx, testresult, eps);

  /* fill in some test data 3x4 */
  mtx=ambix_matrix_init(3, 4, mtx);
  ambix_matrix_fill_data(mtx, leftdata_3_4);
  testresult=ambix_matrix_init(4, 3, testresult);
  ambix_matrix_fill_data(testresult, resultpinv_3_4);
  mtxinverse_test(mtx, testresult, eps);

  ambix_matrix_destroy(mtx);
  ambix_matrix_destroy(testresult);
}
void mtxmul_tests(float32_t eps) {
  float32_t errf;
  ambix_matrix_t *left, *right, *result, *testresult;
  STARTTEST("");

 /* fill in some test data */
  left=ambix_matrix_init(4, 3, NULL);
  ambix_matrix_fill_data(left, leftdata_4_3);
  right=ambix_matrix_init(3, 2, NULL);
  ambix_matrix_fill_data(right, rightdata_3_2);
  testresult=ambix_matrix_init(4, 2, NULL);
  ambix_matrix_fill_data(testresult, resultdata_4_2);

  errf=matrix_diff(__LINE__, left, left, eps);
  fail_if(!(errf<eps), __LINE__, "diffing matrix with itself returned %f (>%f)", errf, eps);

  /* do some matrix multiplication */
  result=ambix_matrix_multiply(left, right, NULL);
  fail_if((NULL==result), __LINE__, "multiply into NULL did not create matrix");

  fail_if((result!=ambix_matrix_multiply(left, right, result)), __LINE__, "multiply into existing matrix returned new matrix");

#if 0
  matrix_print(left);
  matrix_print(right);
  matrix_print(result);
  printf("------------\n");
#endif
  errf=matrix_diff(__LINE__, testresult, result, eps);
  fail_if((errf>eps), __LINE__, "diffing two results of same multiplication returned %f (>%f)", errf, eps);

  ambix_matrix_destroy(left);
  ambix_matrix_destroy(right);
  ambix_matrix_destroy(result);
  ambix_matrix_destroy(testresult);
}
void mtxmul_eye_tests(float32_t eps) {
  float32_t errf;
  ambix_matrix_t *left, *result, *eye;
  STARTTEST("");
  eye=ambix_matrix_init(4, 4, NULL);
  fail_if((eye!=ambix_matrix_fill(eye, AMBIX_MATRIX_IDENTITY)), __LINE__, "filling unity matrix %p did not return original matrix %p", eye);

  left=ambix_matrix_init(4, 2, NULL);
  fail_if(AMBIX_ERR_SUCCESS!=ambix_matrix_fill_data(left, resultdata_4_2), __LINE__,
          "filling left data failed");

  result=ambix_matrix_init(4, 2, NULL);
  fail_if(AMBIX_ERR_SUCCESS!=ambix_matrix_fill_data(result, resultdata_4_2), __LINE__,
          "filling result data failed");

  fail_if((result!=ambix_matrix_multiply(eye, left, result)), __LINE__, "multiplication into matrix did not return original matrix");
#if 0
  matrix_print(eye);
  matrix_print(result);
  matrix_print(left);
#endif
  errf=matrix_diff(__LINE__, left, result, eps);
  fail_if((errf>eps), __LINE__, "diffing matrix M with E*M returned %f (>%f)", errf, eps);

  ambix_matrix_destroy(left);
  ambix_matrix_destroy(result);
  ambix_matrix_destroy(eye);
}
void datamul_tests(float32_t eps) {
  float32_t errf;
  float32_t*resultdata  = (float32_t*)calloc(2*4, sizeof(float32_t));
  float32_t*resultdataT = (float32_t*)calloc(4*2, sizeof(float32_t));
  float32_t*inputdata   = (float32_t*)calloc(2*3, sizeof(float32_t));

  fail_if((NULL==resultdata), __LINE__, "couldn't callocate resultdata");
  fail_if((NULL==resultdataT), __LINE__, "couldn't callocate resultdataT");
  fail_if((NULL==inputdata), __LINE__, "couldn't callocate inputdata");

  ambix_matrix_t*mtx=NULL;
  STARTTEST("");

  mtx=ambix_matrix_init(4, 3, NULL);
  ambix_matrix_fill_data(mtx, leftdata_4_3);

  data_transpose(inputdata, rightdata_3_2, 3, 2);

  fail_if(AMBIX_ERR_SUCCESS!=ambix_matrix_multiply_float32(resultdata, mtx, inputdata, 2), __LINE__,
          "data multiplication failed");

  data_transpose(resultdataT, resultdata, 2, 4);

  errf=data_diff(__LINE__, resultdataT, resultdata_4_2, 4*2, eps);
  if(errf>eps) {
    printf("matrix:\n");
    matrix_print(mtx);
    printf("input:\n");
    data_print(inputdata, 3*2);

    printf("expected:\n");
    data_print(resultdata_4_2, 4*2);
    printf("calculated:\n");
    data_print(resultdataT   , 4*2);

  }
  fail_if(!(errf<eps), __LINE__, "diffing data multiplication returned %f (>%f)", errf, eps);

#if 0
  printf("matrix:\n");matrix_print(mtx);
  printf("input :\n");  data_print(rightdata_3_2, 3*2);
  printf("output:\n");  data_print(resultdata, 4*2);

  printf("target:\n");  data_print(resultdata_4_2, 4*2);
#endif


  if(mtx)ambix_matrix_destroy(mtx);
  free(resultdata);
  free(resultdataT);
  free(inputdata);
}

void datamul_eye_tests(float32_t eps) {
  float32_t errf;
  uint64_t frames=4096;
  uint32_t channels=16;
  float32_t*inputdata;
  float32_t*outputdata;
  float32_t freq=500;
  ambix_matrix_t eye = {0, 0, NULL};
  STARTTEST("");


  inputdata =data_sine(frames, channels, freq);
  outputdata=(float32_t*)malloc(sizeof(float32_t)*frames*channels);
  fail_if((NULL==outputdata), __LINE__, "couldn't mallocate outputdata");

  ambix_matrix_init(channels, channels, &eye);
  ambix_matrix_fill(&eye, AMBIX_MATRIX_IDENTITY);

  fail_if(AMBIX_ERR_SUCCESS!=ambix_matrix_multiply_float32(outputdata, &eye, inputdata, frames),
          __LINE__, "data multilplication failed");

  errf=data_diff(__LINE__, inputdata, outputdata, frames*channels, eps);
  fail_if(!(errf<eps), __LINE__, "diffing data multiplication returned %f (>%f)", errf, eps);

#if 0
  printf("matrix:\n");  matrix_print(&eye);
  printf("input :\n");  data_print(inputdata, frames*channels);
  printf("output:\n");  data_print(outputdata,frames*channels);
#endif

  free(inputdata);
  free(outputdata);
  ambix_matrix_deinit(&eye);
}

void datamul_4_2_tests(uint32_t chunksize, float32_t eps) {
  uint32_t r, c, rows, cols;
  float32_t errf;
  uint64_t frames=8;
  uint32_t rawchannels=2;
  uint32_t cokchannels=4;

  float32_t*inputdata;
  float32_t*outputdata;
  float32_t*targetdata;

  float32_t freq=500;

  ambix_matrix_t eye = {0, 0, NULL};
  STARTTEST("");



  inputdata =data_sine(frames, rawchannels, freq);
  targetdata=data_sine(frames, cokchannels, freq);

  outputdata=(float32_t*)malloc(sizeof(float32_t)*frames*cokchannels);
  fail_if((NULL==outputdata), __LINE__, "couldn't allocate outputdata");

  ambix_matrix_init(cokchannels, rawchannels, &eye);
  rows=eye.rows;
  cols=eye.cols;

  for(r=0; r<rows; r++) {
    for(c=0; c<cols; c++) {
      eye.data[r][c]=(1+r+c)%2;
    }
  }

#if 0
  matrix_print(&eye);
  printf("input\n");
  data_print(inputdata, rawchannels*frames);
#endif

  fail_if(AMBIX_ERR_SUCCESS!=ambix_matrix_multiply_float32(outputdata, &eye, inputdata, frames),
          __LINE__, "data multilplication failed");

#if 0
  printf("output\n");
  data_print(outputdata, cokchannels*frames);

  printf("target\n");
  data_print(targetdata, cokchannels*frames);
#endif



  errf=data_diff(__LINE__, targetdata, outputdata, frames*cokchannels, eps);
  fail_if(!(errf<eps), __LINE__, "diffing data multiplication returned %f (>%f)", errf, eps);

#if 0
  printf("matrix:\n");  matrix_print(&eye);
  printf("input :\n");  data_print(inputdata, frames*channels);
  printf("output:\n");  data_print(outputdata,frames*channels);
#endif

  ambix_matrix_deinit(&eye);

  free(inputdata);
  free(outputdata);
  free(targetdata);
}







void create_tests(float32_t eps) {
  int rows=4;
  int cols=3;
  int cols2=2;
  ambix_matrix_t matrix, *left, *right;
  STARTTEST("");

  memset(&matrix, 0, sizeof(matrix));

  left=ambix_matrix_create();
  fail_if((left==NULL), __LINE__, "failed to create left matrix");
  fail_if((left->rows || left->cols), __LINE__, "created empty matrix has non-zero size");
  fail_if((left!=ambix_matrix_init(rows, cols, left)), __LINE__, "initializing existing matrix* returned new matrix");
  fail_if((left->rows!=rows || left->cols!=cols), __LINE__, "created matrix [%dx%d] does not match [%dx%d]", left->rows, left->cols, cols, cols2);

  right=ambix_matrix_init(cols, cols2, NULL);
  fail_if((right==NULL), __LINE__, "failed to create right matrix");
  fail_if((right->rows!=cols || right->cols!=cols2), __LINE__, "created matrix [%dx%d] does not match [%dx%d]", right->rows, right->cols, cols, cols2);

  fail_if((&matrix!=ambix_matrix_init(rows, cols2, &matrix)), __LINE__, "initializing existing matrix returned new matrix");
  fail_if((matrix.rows!=rows || matrix.cols!=cols2), __LINE__, "initialized matrix [%dx%d] does not match [%dx%d]", matrix.rows, matrix.cols, rows, cols2);


  ambix_matrix_deinit(&matrix);
  fail_if((matrix.rows || matrix.cols), __LINE__, "deinitialized matrix is non-zero");

  ambix_matrix_deinit(left);
  fail_if((left->rows || left->cols), __LINE__, "deinitialized matrix is non-zero");

  ambix_matrix_destroy(left);
  ambix_matrix_destroy(right);
}



int main(int argc, char**argv) {
#if 1
  create_tests(1e-7);
  mtxmul_tests(1e-7);
  mtxmul_eye_tests(1e-7);
  datamul_tests(1e-7);
  datamul_eye_tests(1e-7);
#endif
  datamul_4_2_tests(1024, 1e-7);
  mtxinverse_tests(1e-5);

  pass();
  return 0;
}
