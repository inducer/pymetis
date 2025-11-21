/*!
\file  
\brief It takes as input two CSR matrices and finds for each row of the 
       first matrix the most similar rows in the second matrix.

\date 9/27/2014
\author George
\version \verbatim $Id: m2mnbrs.c 17699 2014-09-27 18:05:31Z karypis $ \endverbatim
*/

#include <GKlib.h>

/*************************************************************************/
/*! Data structures for the code */
/*************************************************************************/
typedef struct {
  int simtype;             /*!< The similarity type to use */
  int nnbrs;               /*!< The maximum number of nearest neighbots to output */
  float minsim;            /*!< The minimum similarity to use for keeping neighbors */

  int verbosity;           /*!< The reporting verbosity level */

  char *qfile;             /*!< The file storing the query documents */
  char *cfile;             /*!< The file storing the collection documents */
  char *outfile;           /*!< The file where the output will be stored */

  /* timers */
  double timer_global;
  double timer_1;
  double timer_2;
  double timer_3;
  double timer_4;
} params_t;


/*************************************************************************/
/*! Constants */
/*************************************************************************/
/* Versions */
#define VER_MAJOR           0
#define VER_MINOR           1
#define VER_SUBMINOR        0

/* Command-line option codes */
#define CMD_SIMTYPE         10
#define CMD_NNBRS           20
#define CMD_MINSIM          22
#define CMD_VERBOSITY       70
#define CMD_HELP            100

/* The text labels for the different simtypes */
static char simtypenames[][10] = {"", "dotp", "cos", "jac", ""};



/*************************************************************************/
/*! Local variables */
/*************************************************************************/
static struct gk_option long_options[] = {
  {"simtype",           1,      0,      CMD_SIMTYPE},
  {"nnbrs",             1,      0,      CMD_NNBRS},
  {"minsim",            1,      0,      CMD_MINSIM},
  {"verbosity",         1,      0,      CMD_VERBOSITY},

  {"help",              0,      0,      CMD_HELP},
  {0,                   0,      0,      0}
};

static gk_StringMap_t simtype_options[] = {
  {"cos",                GK_CSR_COS},
  {"jac",                GK_CSR_JAC},
  {NULL,                 0}
};


/*-------------------------------------------------------------------
 * Mini help
 *-------------------------------------------------------------------*/
static char helpstr[][100] =
{
" ",
"Usage: m2mnbrs [options] qfile cfile [outfile]",
" ",
" Options",
"  -simtype=string",
"     Specifies the type of similarity to use. Possible values are:",
"       cos   - Cosine similarity",
"       jac   - Jacquard similarity [default]", 
" ",
"  -nnbrs=int",
"     Specifies the maximum number of nearest neighbors.",
"     A value of -1 indicates that all neighbors will be considered.",
"     Default value is 100.",
" ",
"  -minsim=float",
"     The minimum allowed similarity between neighbors. ",
"     Default value is .25.",
" ",
"  -verbosity=int",
"     Specifies the level of debugging information to be displayed.",
"     Default value is 0.",
" ",
"  -help",
"     Prints this message.",
""
};



/*************************************************************************/
/*! Function prototypes */
/*************************************************************************/
params_t *parse_cmdline(int argc, char *argv[]);
void FindNeighbors(params_t *params, gk_csr_t *qmat, gk_csr_t *cmat);


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
  params->simtype   = GK_CSR_JAC;
  params->nnbrs     = 100;
  params->minsim    = .25;
  params->verbosity = -1;
  params->qfile     = NULL;
  params->cfile     = NULL;
  params->outfile   = NULL;


  /* Parse the command line arguments  */
  while ((c = gk_getopt_long_only(argc, argv, "", long_options, &option_index)) != -1) {
    switch (c) {
      case CMD_SIMTYPE:
        if (gk_optarg) {
          if ((params->simtype = gk_GetStringID(simtype_options, gk_optarg)) == -1)
            errexit("Invalid simtype of %s.\n", gk_optarg);
        }
        break;

      case CMD_NNBRS:
        if (gk_optarg) params->nnbrs = atoi(gk_optarg);
        break;

      case CMD_MINSIM:
        if (gk_optarg) params->minsim = atof(gk_optarg);
        break;

      case CMD_VERBOSITY:
        if (gk_optarg) params->verbosity = atoi(gk_optarg);
        break;

      case CMD_HELP:
        for (i=0; strlen(helpstr[i]) > 0; i++)
          printf("%s\n", helpstr[i]);
        exit(EXIT_SUCCESS);
        break;

      case '?':
      default:
        printf("Illegal command-line option(s)\nUse %s -help for a summary of the options.\n", argv[0]);
        exit(EXIT_FAILURE);
    }
  }

  /* Get the input/output file info */
  if (argc-gk_optind < 1) {
    printf("Missing input/output file info.\n  Use %s -help for a summary of the options.\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  params->qfile   = gk_strdup(argv[gk_optind++]);
  params->cfile   = gk_strdup(argv[gk_optind++]);
  params->outfile = (gk_optind < argc ? gk_strdup(argv[gk_optind++]) : NULL);

  if (!gk_fexists(params->qfile))
    errexit("input file %s does not exist.\n", params->qfile);
  if (!gk_fexists(params->cfile))
    errexit("input file %s does not exist.\n", params->cfile);

  return params;
}


/*************************************************************************/
/*! This is the entry point of the program */
/**************************************************************************/
int main(int argc, char *argv[])
{
  params_t *params;
  gk_csr_t *qmat, *cmat;
  int rc = EXIT_SUCCESS;

  params = parse_cmdline(argc, argv);

  qmat = gk_csr_Read(params->qfile, GK_CSR_FMT_CSR, 1, 0);
  cmat = gk_csr_Read(params->cfile, GK_CSR_FMT_CSR, 1, 0);


  printf("********************************************************************************\n");
  printf("sd (%d.%d.%d) Copyright 2014, GK.\n", VER_MAJOR, VER_MINOR, VER_SUBMINOR);
  printf("  simtype=%s, nnbrs=%d, minsim=%.2f\n",
      simtypenames[params->simtype], params->nnbrs, params->minsim);
  printf("  qfile=%s, nrows=%d, ncols=%d, nnz=%zd\n",
      params->qfile, qmat->nrows, qmat->ncols, qmat->rowptr[qmat->nrows]);
  printf("  cfile=%s, nrows=%d, ncols=%d, nnz=%zd\n",
      params->cfile, cmat->nrows, cmat->ncols, cmat->rowptr[cmat->nrows]);

  gk_clearwctimer(params->timer_global);
  gk_clearwctimer(params->timer_1);
  gk_clearwctimer(params->timer_2);
  gk_clearwctimer(params->timer_3);
  gk_clearwctimer(params->timer_4);

  gk_startwctimer(params->timer_global);

  FindNeighbors(params, qmat, cmat);

  gk_stopwctimer(params->timer_global);

  printf("    wclock: %.2lfs\n", gk_getwctimer(params->timer_global));
  printf("    timer1: %.2lfs\n", gk_getwctimer(params->timer_1));
  printf("    timer2: %.2lfs\n", gk_getwctimer(params->timer_2));
  printf("    timer3: %.2lfs\n", gk_getwctimer(params->timer_3));
  printf("    timer4: %.2lfs\n", gk_getwctimer(params->timer_4));
  printf("********************************************************************************\n");

  gk_csr_Free(&qmat);
  gk_csr_Free(&cmat);

  exit(rc);
}


/*************************************************************************/
/*! Reads and computes the neighbors of each query document against the
    collection of documents */
/**************************************************************************/
void FindNeighbors(params_t *params, gk_csr_t *qmat, gk_csr_t *cmat)
{
  int iQ, iH, nhits;
  int32_t *marker;
  gk_fkv_t *hits, *cand;
  FILE *fpout;

  GKASSERT(qmat->ncols <= cmat->ncols);

  /* if cosine, make rows unit length */
  if (params->simtype == GK_CSR_COS) {
    gk_csr_Normalize(qmat, GK_CSR_ROW, 2);
    gk_csr_Normalize(cmat, GK_CSR_ROW, 2);
  }

  /* create the inverted index */
  gk_csr_CreateIndex(cmat, GK_CSR_COL);

  /* compute the row norms */
  gk_csr_ComputeSquaredNorms(cmat, GK_CSR_ROW);

  /* create the output file */
  fpout = (params->outfile ? gk_fopen(params->outfile, "w", "FindNeighbors: fpout") : NULL);

  /* allocate memory for the necessary working arrays */
  hits   = gk_fkvmalloc(cmat->nrows, "FindNeighbors: hits");
  marker = gk_i32smalloc(cmat->nrows, -1, "FindNeighbors: marker");
  cand   = gk_fkvmalloc(cmat->nrows, "FindNeighbors: cand");


  /* find the best neighbors for each query document */
  gk_startwctimer(params->timer_1);
  for (iQ=0; iQ<qmat->nrows; iQ++) {
    if (params->verbosity > 0)
      printf("Working on query %7d\n", iQ);

    /* find the neighbors of the ith document */ 
    nhits = gk_csr_GetSimilarRows(cmat, 
                 qmat->rowptr[iQ+1]-qmat->rowptr[iQ], 
                 qmat->rowind+qmat->rowptr[iQ], 
                 qmat->rowval+qmat->rowptr[iQ], 
                 params->simtype, params->nnbrs, params->minsim, 
                 hits, marker, cand);

    /* write the results in the file */
    if (fpout) {
      for (iH=0; iH<nhits; iH++) 
        fprintf(fpout, "%8d %8zd %.3f\n", iQ, hits[iH].val, hits[iH].key);
    }
  }
  gk_stopwctimer(params->timer_1);


  /* cleanup and exit */
  if (fpout) gk_fclose(fpout);

  gk_free((void **)&hits, &marker, &cand, LTERM);
}

