/*!
\file  
\brief It takes as input two CSR matrices A and B and computes how
       similar AA' and A'A are to BB' and B'B, respectively in terms
       of the cosine similarity of the corresponding rows.

\date 11/09/2015
\author George
\version \verbatim $Id: m2mnbrs.c 17699 2014-09-27 18:05:31Z karypis $ \endverbatim
*/

#include <GKlib.h>

/*************************************************************************/
/*! Data structures for the code */
/*************************************************************************/
typedef struct {
  int simtype;             /*!< The similarity type to use */
  int verbosity;           /*!< The reporting verbosity level */

  char *afile;             /*!< The file storing the query documents */
  char *bfile;             /*!< The file storing the collection documents */

  /* timers */
  double timer_global;
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
#define CMD_VERBOSITY       70
#define CMD_HELP            100

/* The text labels for the different simtypes */
static char simtypenames[][10] = {"", "dotp", "cos", "jac", ""};


/*************************************************************************/
/*! Local variables */
/*************************************************************************/
static struct gk_option long_options[] = {
  {"simtype",           1,      0,      CMD_SIMTYPE},
  {"verbosity",         1,      0,      CMD_VERBOSITY},

  {"help",              0,      0,      CMD_HELP},
  {0,                   0,      0,      0}
};

static gk_StringMap_t simtype_options[] = {
  {"dotp",               GK_CSR_DOTP},
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
"Usage: cmpnbrs [options] afile bfile",
" ",
" Options",
"  -simtype=string",
"     Specifies the type of similarity to use. Possible values are:",
"       dotp  - Dot-product similarity [default]",
"       cos   - Cosine similarity",
"       jac   - Jacquard similarity", 
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
double ComputeNeighborhoodSimilarity(params_t *params, gk_csr_t *amat, gk_csr_t *bmat);


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
  params->simtype   = GK_CSR_DOTP;
  params->verbosity = -1;
  params->afile     = NULL;
  params->bfile     = NULL;


  /* Parse the command line arguments  */
  while ((c = gk_getopt_long_only(argc, argv, "", long_options, &option_index)) != -1) {
    switch (c) {
      case CMD_SIMTYPE:
        if (gk_optarg) {
          if ((params->simtype = gk_GetStringID(simtype_options, gk_optarg)) == -1)
            errexit("Invalid simtype of %s.\n", gk_optarg);
        }
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
  if (argc-gk_optind != 2) {
    printf("Missing input file info.\n  Use %s -help for a summary of the options.\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  params->afile = gk_strdup(argv[gk_optind++]);
  params->bfile = gk_strdup(argv[gk_optind++]);

  if (!gk_fexists(params->afile))
    errexit("input file %s does not exist.\n", params->afile);
  if (!gk_fexists(params->bfile))
    errexit("input file %s does not exist.\n", params->bfile);

  return params;
}


/*************************************************************************/
/*! This is the entry point of the program */
/**************************************************************************/
int main(int argc, char *argv[])
{
  params_t *params;
  gk_csr_t *amat, *bmat, *amatt, *bmatt;
  int rc = EXIT_SUCCESS;

  params = parse_cmdline(argc, argv);

  amat = gk_csr_Read(params->afile, GK_CSR_FMT_CSR, 1, 0);
  bmat = gk_csr_Read(params->bfile, GK_CSR_FMT_CSR, 1, 0);

  /* make the matrices of similar dimensions (if neccessary) */
  GKASSERT(amat->nrows == bmat->nrows);
  amat->ncols = gk_max(amat->ncols, bmat->ncols);
  bmat->ncols = amat->ncols;

  /* create the transpose matrices */
  amatt = gk_csr_Transpose(amat);
  bmatt = gk_csr_Transpose(bmat);

  printf("********************************************************************************\n");
  printf("cmpnbrs (%d.%d.%d) Copyright 2015, GK.\n", VER_MAJOR, VER_MINOR, VER_SUBMINOR);
  printf("  simtype=%s\n",
      simtypenames[params->simtype]);
  printf("  afile=%s, nrows=%d, ncols=%d, nnz=%zd\n",
      params->afile, amat->nrows, amat->ncols, amat->rowptr[amat->nrows]);
  printf("  bfile=%s, nrows=%d, ncols=%d, nnz=%zd\n",
      params->bfile, bmat->nrows, bmat->ncols, bmat->rowptr[bmat->nrows]);

  gk_clearwctimer(params->timer_global);
  gk_startwctimer(params->timer_global);

  printf("SIM(AA', BB'): %.5lf\t", ComputeNeighborhoodSimilarity(params, amat, bmat));
  printf("SIM(A'A, B'B): %.5lf\n", ComputeNeighborhoodSimilarity(params, amatt, bmatt));

  gk_stopwctimer(params->timer_global);

  printf("    wclock: %.2lfs\n", gk_getwctimer(params->timer_global));
  printf("********************************************************************************\n");

  gk_csr_Free(&amat);
  gk_csr_Free(&bmat);
  gk_csr_Free(&amatt);
  gk_csr_Free(&bmatt);

  exit(rc);
}


/*************************************************************************/
/*! Compares the neighbors of AA' vs BB' */
/**************************************************************************/
double ComputeNeighborhoodSimilarity(params_t *params, gk_csr_t *amat, 
           gk_csr_t *bmat)
{
  int iR, iH, nahits, nbhits, ncmps;
  int32_t *marker;
  gk_fkv_t *ahits, *bhits, *cand;
  double tabsim, abdot, anorm2, bnorm2, *avec, *bvec;

  /* if cosine, make rows unit length */
  if (params->simtype == GK_CSR_COS) {
    gk_csr_Normalize(amat, GK_CSR_ROW, 2);
    gk_csr_Normalize(bmat, GK_CSR_ROW, 2);
  }

  /* create the inverted index */
  gk_csr_CreateIndex(amat, GK_CSR_COL);
  gk_csr_CreateIndex(bmat, GK_CSR_COL);

  /* compute the row squared norms */
  gk_csr_ComputeSquaredNorms(amat, GK_CSR_ROW);
  gk_csr_ComputeSquaredNorms(bmat, GK_CSR_ROW);


  /* allocate memory for the necessary working arrays */
  ahits  = gk_fkvmalloc(amat->nrows, "ComputeNeighborhoodSimilarity: ahits");
  bhits  = gk_fkvmalloc(bmat->nrows, "ComputeNeighborhoodSimilarity: bhits");
  marker = gk_i32smalloc(amat->nrows, -1, "ComputeNeighborhoodSimilarity: marker");
  cand   = gk_fkvmalloc(amat->nrows, "ComputeNeighborhoodSimilarity: cand");
  avec   = gk_dsmalloc(amat->nrows, 0.0, "ComputeNeighborhoodSimilarity: avec");
  bvec   = gk_dsmalloc(bmat->nrows, 0.0, "ComputeNeighborhoodSimilarity: bvec");


  /* find the best neighbors for each row in the two matrices and compute 
     the cosine similarity between them. */
  tabsim = 0.0;
  ncmps  = 0;
  for (iR=0; iR<amat->nrows; iR++) {
    if (params->verbosity > 1)
      printf("Working on row %7d\n", iR);

    if (amat->rowptr[iR+1]-amat->rowptr[iR] == 0 ||
        bmat->rowptr[iR+1]-bmat->rowptr[iR] == 0)
      continue;

    nahits = gk_csr_GetSimilarRows(amat, 
                 amat->rowptr[iR+1]-amat->rowptr[iR], 
                 amat->rowind+amat->rowptr[iR], 
                 amat->rowval+amat->rowptr[iR], 
                 params->simtype, amat->nrows, 0.0,
                 ahits, marker, cand);

    nbhits = gk_csr_GetSimilarRows(bmat, 
                 bmat->rowptr[iR+1]-bmat->rowptr[iR], 
                 bmat->rowind+bmat->rowptr[iR], 
                 bmat->rowval+bmat->rowptr[iR], 
                 params->simtype, bmat->nrows, 0.0,
                 bhits, marker, cand);

    if (params->verbosity > 0)
      printf("Row %7d %7d %7d %8zd %8zd\n", iR, nahits, nbhits, 
          amat->rowptr[iR+1]-amat->rowptr[iR], bmat->rowptr[iR+1]-bmat->rowptr[iR]);

    for (iH=0; iH<nahits; iH++) 
      avec[ahits[iH].val] = ahits[iH].key;
    for (iH=0; iH<nbhits; iH++) 
      bvec[bhits[iH].val] = bhits[iH].key;

    for (abdot=anorm2=bnorm2=0.0, iH=0; iH<amat->nrows; iH++) {
      abdot  += avec[iH]*bvec[iH];
      anorm2 += avec[iH]*avec[iH];
      bnorm2 += bvec[iH]*bvec[iH];
    }
    tabsim += (abdot > 0 ? abdot/sqrt(anorm2*bnorm2) : 0.0);
    ncmps++;

    for (iH=0; iH<nahits; iH++) 
      avec[ahits[iH].val] = 0.0;
    for (iH=0; iH<nbhits; iH++) 
      bvec[bhits[iH].val] = 0.0;
  }

  gk_free((void **)&ahits, &bhits, &marker, &cand, &avec, &bvec, LTERM);

  return tabsim/ncmps;
}

