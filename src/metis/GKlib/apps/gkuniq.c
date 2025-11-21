/*!
\file  
\brief A program to test various implementations for unique.

\date 10/8/2020
\author George
*/

#include <GKlib.h>

/*************************************************************************/
/*! Data structures for the code */
/*************************************************************************/
typedef struct {
  ssize_t length, dupfactor;
} params_t;

/*************************************************************************/
/*! Constants */
/*************************************************************************/
#define CMD_HELP        10


/*************************************************************************/
/*! Local variables */
/*************************************************************************/
static struct gk_option long_options[] = {
  {"help",          0,      0,      CMD_HELP},
  {0,               0,      0,      0}
};


/*-------------------------------------------------------------------*/
/* Mini help  */
/*-------------------------------------------------------------------*/
static char helpstr[][100] = {
" ",
"Usage: gkuniq length dupfactor",
" ",
" Required parameters",
"  length",
"     The length of the base array.",
" ",
"  dupfactor",
"     The number of times the initial array is replicated.",
" ",
" Optional parameters",
"  -help",
"     Prints this message.",
""
};



/*************************************************************************/
/*! Function prototypes */
/*************************************************************************/
params_t *parse_cmdline(int argc, char *argv[]);
int unique_v1(int n, int *input, int *output);
int unique_v2(int n, int *input, int *output);
int unique_v3(int n, int *input, int *output, int *r_maxsize, int **r_hmap);
void mem_flush(const void *p, unsigned int allocation_size);

/*************************************************************************/
/*! A function to flush the cache associated with an array */
/**************************************************************************/
void mem_flush(const void *p, unsigned int allocation_size)
{
#ifndef NO_X86 
  const size_t cache_line = 64;
  const char *cp = (const char *)p;
  size_t i = 0;

  if (p == NULL || allocation_size <= 0)
    return;

  for (i = 0; i < allocation_size; i += cache_line) {
    __asm__ volatile("clflush (%0)\n\t"
                 :
                 : "r"(&cp[i])
                 : "memory");
  }

  __asm__ volatile("sfence\n\t"
                :
                :
                : "memory");
#endif
}

/*************************************************************************/
/*! the entry point */
/**************************************************************************/
int main(int argc, char *argv[])
{
  int i, j, k;
  params_t *params;
  double tmr;
  int n, nunique, *input, *output;
  int maxsize=0, *hmap=NULL; 
 
  params = parse_cmdline(argc, argv);

  /* create the input data */
  n = params->length*params->dupfactor;
  input  = gk_imalloc(n, "input");
  output = gk_imalloc(n, "output");
  for (i=0; i<params->length; i++) {
    k = RandomInRange(n);
    for (j=0; j<params->dupfactor; j++)
      input[j*params->length+i] = k;
  }

  gk_clearwctimer(tmr);
  gk_startwctimer(tmr);
  mem_flush(input, n*sizeof(int));
  mem_flush(output, n*sizeof(int));
  nunique = unique_v1(n, input, output);
  gk_stopwctimer(tmr);
  printf(" V1: nunique: %d, timer: %.5lf\n", nunique, gk_getwctimer(tmr));

  gk_clearwctimer(tmr);
  gk_startwctimer(tmr);
  mem_flush(input, n*sizeof(int));
  mem_flush(output, n*sizeof(int));
  nunique = unique_v2(n, input, output);
  gk_stopwctimer(tmr);
  printf(" V2: nunique: %d, timer: %.5lf\n", nunique, gk_getwctimer(tmr));

  gk_clearwctimer(tmr);
  gk_startwctimer(tmr);
  mem_flush(input, n*sizeof(int));
  mem_flush(output, n*sizeof(int));
  nunique = unique_v3(n, input, output, &maxsize, &hmap);
  gk_stopwctimer(tmr);
  printf("V3c: nunique: %d, timer: %.5lf\n", nunique, gk_getwctimer(tmr));

  gk_clearwctimer(tmr);
  gk_startwctimer(tmr);
  mem_flush(input, n*sizeof(int));
  mem_flush(output, n*sizeof(int));
  nunique = unique_v3(n, input, output, &maxsize, &hmap);
  gk_stopwctimer(tmr);
  printf("V3w: nunique: %d, timer: %.5lf\n", nunique, gk_getwctimer(tmr));

  gk_free((void **)&input, &output, &hmap, LTERM);

}



/*************************************************************************/
/*! This is the entry point of the command-line argument parser */
/*************************************************************************/
params_t *parse_cmdline(int argc, char *argv[])
{
  int i;
  int c, option_index;
  params_t *params;

  params = (params_t *)gk_malloc(sizeof(params_t), "parse_cmdline: params");

  /* Parse the command line arguments  */
  while ((c = gk_getopt_long_only(argc, argv, "", long_options, &option_index)) != -1) {
    switch (c) {
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

  if (argc-gk_optind != 2) {
    printf("Unrecognized parameters.");
    for (i=0; strlen(helpstr[i]) > 0; i++)
      printf("%s\n", helpstr[i]);
    exit(0);
  }

  params->length    = atoi(argv[gk_optind++]);
  params->dupfactor = atoi(argv[gk_optind++]);

  return params;
}


/*************************************************************************/
/*! gklib-sort based approach */
/*************************************************************************/
int unique_v1(int n, int *input, int *output)
{
  int i, j;

  gk_isorti(n, input);

  output[0] = input[0];
  for (j=0, i=1; i<n; i++) {
    if (output[j] != input[i]) 
      output[++j] = input[i];
  }
  return j+1;
}


/*************************************************************************/
/*! hash-table based approach */
/*************************************************************************/
int unique_v2(int n, int *input, int *output)
{
  int i, j, k, nuniq, size, mask;
  int *hmap;

  for (size=1; size<2*n; size*=2);
  mask = size-1;
  //printf("size: %d, mask: %x\n", size, mask);
  hmap = gk_ismalloc(size, -1, "hmap");

  for (nuniq=0, i=0; i<n; i++) {
    k = input[i];
    for (j=(k&mask); hmap[j]!=-1 && hmap[j]!=k; j=((j+1)&mask));
    if (hmap[j] == -1) {
      hmap[j] = k;
      output[nuniq++] = k;
    }
  }

  gk_free((void **)&hmap, LTERM);
  return nuniq;
}


/*************************************************************************/
/*! hash-table based approach, where the htable is most likely pre-allocated */
/*************************************************************************/
int unique_v3(int n, int *input, int *output, int *r_maxsize, int **r_hmap)
{
  int i, j, k, nuniq, size, mask;
  int *hmap;

  for (size=1; size<2*n; size*=2);
  mask = size-1;
  //printf("size: %d, mask: %x\n", size, mask);
  if (size > *r_maxsize) {
    gk_free((void **)r_hmap, LTERM);
    hmap = *r_hmap = gk_ismalloc(size, -1, "hmap");
    *r_maxsize = size;
  }
  else {
    hmap = *r_hmap;
    gk_iset(size, -1, hmap);
  }

  for (nuniq=0, i=0; i<n; i++) {
    k = input[i];
    for (j=(k&mask); hmap[j]!=-1 && hmap[j]!=k; j=((j+1)&mask));
    if (hmap[j] == -1) {
      hmap[j] = k;
      output[nuniq++] = k;
    }
  }

  return nuniq;
}
