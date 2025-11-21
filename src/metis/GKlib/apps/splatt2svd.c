/*!
\file 
\brief A simple program to convert a tensor in coordinate format into an unfolded 
       matrix

\author George
*/

#include <GKlib.h>


int main(int argc, char *argv[])
{
  size_t nnz, i, j, k, nI, nJ, nK, nrows, ncols;
  int32_t *I, *J, *K, *rowind, *colind;
  ssize_t *rowptr, *colptr;
  float *V, *rowval, *colval;

  if (argc != 2) 
    errexit("Usage %s <infile> [%d]\n", argv[0], argc);

  if (!gk_fexists(argv[1]))
    errexit("File %s does not exist.\n", argv[1]);

  gk_getfilestats(argv[1], &nnz, NULL, NULL, NULL);
  I = gk_i32malloc(nnz, "I");
  J = gk_i32malloc(nnz, "J");
  K = gk_i32malloc(nnz, "K");
  V = gk_fmalloc(nnz, "V");

  fprintf(stderr, "Input nnz: %zd\n", nnz);

  FILE *fpin = gk_fopen(argv[1], "r", "infile");
  for (i=0; i<nnz; i++) {
    if (4 != fscanf(fpin, "%d %d %d %f", K+i, I+i, J+i, V+i))
      errexit("Failed to read 4 values in line %zd\n", i);
    K[i]--; I[i]--; J[i]--;
  }
  gk_fclose(fpin);

  nI = gk_i32max(nnz, I, 1)+1;
  nJ = gk_i32max(nnz, J, 1)+1;
  nK = gk_i32max(nnz, K, 1)+1;

  fprintf(stderr, "nI: %zd, nJ: %zd, nK: %zd\n", nI, nJ, nK);

  nrows = nK*nI;
  ncols = nJ;
  rowptr = gk_zsmalloc(nrows+1, 0, "rowptr");
  for (i=0; i<nnz; i++) 
    rowptr[K[i]*nI+I[i]]++;
  MAKECSR(i, nrows, rowptr);

  rowind = gk_i32malloc(nnz, "rowind");
  rowval = gk_fmalloc(nnz, "rowval");
  for (i=0; i<nnz; i++) {
    rowind[rowptr[K[i]*nI+I[i]]] = J[i];
    rowval[rowptr[K[i]*nI+I[i]]] = V[i];
    rowptr[K[i]*nI+I[i]]++;
  }
  SHIFTCSR(i, nrows, rowptr);

  gk_free((void **)&I, &J, &K, &V, LTERM);

  colptr = gk_zsmalloc(ncols+1, 0, "colptr");
  colind = gk_i32malloc(nnz, "colind");
  colval = gk_fmalloc(nnz, "colval");
  for (i=0; i<nrows; i++) {
    for (j=rowptr[i]; j<rowptr[i+1]; j++)
      colptr[rowind[j]]++;
  }
  MAKECSR(i, ncols, colptr);
  for (i=0; i<nrows; i++) {
    for (j=rowptr[i]; j<rowptr[i+1]; j++) {
      colind[colptr[rowind[j]]] = i;
      colval[colptr[rowind[j]]] = rowval[j];
      colptr[rowind[j]]++;
    }
  }
  SHIFTCSR(i, ncols, colptr);

  /* sanity check */
  for (i=0; i<ncols; i++) {
    for (j=colptr[i]+1; j<colptr[i+1]; j++) {
      if (colind[j-1] == colind[j])
        fprintf(stderr, "Duplicate row indices: %d %d %d\n", (int)i, colind[j], colind[j-1]);
    }
  }

  printf("%zd %zd %zd\n", nrows, ncols, nnz);
  for (i=0; i<ncols; i++) {
    printf("%zd\n", colptr[i+1]-colptr[i]);
    for (j=colptr[i]; j<colptr[i+1]; j++)
      printf("%d %.3f\n", colind[j], colval[j]);
  }

}

