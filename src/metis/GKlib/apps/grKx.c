/*!
\file  
\brief A simple program to create multiple copies of an input matrix.

\date 5/30/2013
\author George
\version \verbatim $Id: grKx.c 17699 2014-09-27 18:05:31Z karypis $ \endverbatim
*/

#include <GKlib.h>

/*************************************************************************/
/*! Data structures for the code */
/*************************************************************************/
typedef struct {
  int inf, outf;
  int numbering;    /* input numbering (output when applicable) */
  int readvals;     /* input values (output when applicable) */
  int writevals;    /* output values */
  int rshuf, cshuf; /* random shuffle of rows/columns */
  int symmetric;    /* a symmetric shuffle */
  int ncopies;      /* the copies of the graph to create */
  char *infile;     /* input file */
  char *outfile;    /* output file */
} params_t;


/*************************************************************************/
/*! Constants */
/*************************************************************************/
#define CMD_NUMONE        1
#define CMD_NOREADVALS    2
#define CMD_NOWRITEVALS   3
#define CMD_RSHUF         4
#define CMD_CSHUF         5
#define CMD_SYMMETRIC     6
#define CMD_HELP          100


/*************************************************************************/
/*! Local variables */
/*************************************************************************/
static struct gk_option long_options[] = {
  {"numone",      0,      0,      CMD_NUMONE},
  {"noreadvals",  0,      0,      CMD_NOREADVALS},
  {"nowritevals", 0,      0,      CMD_NOWRITEVALS},
  {"rshuf",       0,      0,      CMD_RSHUF},
  {"cshuf",       0,      0,      CMD_CSHUF},
  {"symmetric",   0,      0,      CMD_SYMMETRIC},
  {"help",        0,      0,      CMD_HELP},
  {0,             0,      0,      0}
};


/*-------------------------------------------------------------------*/
/* Mini help  */
/*-------------------------------------------------------------------*/
static char helpstr[][100] = {
" ",
"Usage: grKx [options] <infile> <inf> <outfile> <outf> <ncopies>",
" ",
" Required parameters",
"  infile, outfile",
"     The name of the input/output CSR file.",
" ",
"  inf/outf",
"     The format of the input/output file.",
"     Supported values are:",
"        1  GK_CSR_FMT_CLUTO",
"        2  GK_CSR_FMT_CSR",
"        3  GK_CSR_FMT_METIS",
"        4  GK_CSR_FMT_BINROW",
"        6  GK_CSR_FMT_IJV",
"        7  GK_CSR_FMT_BIJV",
" ",
" Optional parameters",
"  -numone",
"     Specifies that the numbering of the input file starts from 1. ",
"     It only applies to CSR/IJV formats.",
" ",
"  -nowritevals",
"     Specifies that no values will be output.",
" ",
"  -noreadvals",
"     Specifies that the values will not be read when applicable.",
" ",
"  -rshuf",
"     Specifies that the rows will be randmly shuffled prior to output.",
" ",
"  -cshuf",
"     Specifies that the columns will be randmly shuffled prior to output.",
" ",
"  -symmetric",
"     Specifies that the row+column shuffling will be symmetric.",
" ",
"  -help",
"     Prints this message.",
""
};

static char shorthelpstr[][100] = {
" ",
"   Usage: grKx [options] <infile> <inf> <outfile> <outf> <ncopies>",
"          use 'csrconv -help' for a summary of the options.",
""
};
 

/*************************************************************************/
/*! This is the entry point of the command-line argument parser */
/*************************************************************************/
params_t *parse_cmdline(int argc, char *argv[])
{
  int i;
  int c, option_index;
  params_t *params;

  params = (params_t *)gk_malloc(sizeof(params_t), "parse_cmdline: params");

  /* initialize the params data structure */
  params->numbering = 0;
  params->readvals  = 1;
  params->writevals = 1;
  params->rshuf     = 0;
  params->cshuf     = 0;
  params->symmetric = 0;

  params->inf       = -1;
  params->outf      = -1;
  params->infile    = NULL;
  params->outfile   = NULL;


  /* Parse the command line arguments  */
  while ((c = gk_getopt_long_only(argc, argv, "", long_options, &option_index)) != -1) {
    switch (c) {
      case CMD_NUMONE:
        params->numbering = 1;
        break;
      case CMD_NOREADVALS:
        params->readvals = 0;
        break;
      case CMD_NOWRITEVALS:
        params->writevals = 0;
        break;
      case CMD_RSHUF:
        params->rshuf = 1;
        break;
      case CMD_CSHUF:
        params->cshuf = 1;
        break;
      case CMD_SYMMETRIC:
        params->symmetric = 1;
        break;

      case CMD_HELP:
        for (i=0; strlen(helpstr[i]) > 0; i++)
          printf("%s\n", helpstr[i]);
        exit(0);
        break;
      case '?':
      default:
        printf("Illegal command-line option(s)\nUse %s -help for a summary of the options.\n", argv[0]);
        exit(0);
    }
  }

  if (argc-gk_optind != 5) {
    printf("Unrecognized parameters.");
    for (i=0; strlen(shorthelpstr[i]) > 0; i++)
      printf("%s\n", shorthelpstr[i]);
    exit(0);
  }

  params->infile  = gk_strdup(argv[gk_optind++]);
  params->inf     = atoi(argv[gk_optind++]);
  params->outfile = gk_strdup(argv[gk_optind++]);
  params->outf    = atoi(argv[gk_optind++]);
  params->ncopies = atoi(argv[gk_optind++]);

  if (!gk_fexists(params->infile))
    errexit("input file %s does not exist.\n", params->infile);

  return params;
}


/*************************************************************************/
/*! the entry point */
/**************************************************************************/
int main(int argc, char *argv[])
{
  ssize_t i, j, k, knnz, nrows, ncols, ncopies;
  int what;
  params_t *params;
  gk_csr_t *mat, *kmat, *smat;
 
  /* get command-line options */
  params = parse_cmdline(argc, argv);

  /* read the data */
  mat = gk_csr_Read(params->infile, params->inf, params->readvals, params->numbering);

  /* create the copies */
  ncopies = params->ncopies;

  nrows = mat->nrows;
  ncols = mat->ncols;
  knnz  = mat->rowptr[nrows]*ncopies;

  kmat         = gk_csr_Create();
  kmat->nrows  = nrows*ncopies;
  kmat->ncols  = ncols*ncopies;
  kmat->rowptr = gk_zmalloc(kmat->nrows+1, "rowptr");
  kmat->rowind = gk_imalloc(knnz, "rowind");
  if (mat->rowval)
    kmat->rowval = gk_fmalloc(knnz, "rowval");

  kmat->rowptr[0] = knnz = 0;
  for (k=0; k<ncopies; k++) {
    for (i=0; i<nrows; i++) {
      for (j=mat->rowptr[i]; j<mat->rowptr[i+1]; j++, knnz++) {
        kmat->rowind[knnz] = mat->rowind[j] + k*ncols;
        if (mat->rowval)
          kmat->rowval[knnz] = mat->rowval[j];
      }
      kmat->rowptr[k*nrows+i+1] = knnz;
    }
  }

  gk_csr_Free(&mat);
  mat = kmat;


  if (params->rshuf || params->cshuf) {
    if (params->rshuf && params->cshuf)
      what = GK_CSR_ROWCOL;
    else if (params->rshuf)
      what = GK_CSR_ROW;
    else
      what = GK_CSR_COL;

    smat = gk_csr_Shuffle(mat, what, params->symmetric);
    gk_csr_Free(&mat);
    mat = smat;
  }

  if (params->writevals && mat->rowval == NULL) 
    mat->rowval = gk_fsmalloc(mat->rowptr[mat->nrows], 1.0, "mat->rowval");

  gk_csr_Write(mat, params->outfile, params->outf, params->writevals, 0);

  gk_csr_Free(&mat);

}

