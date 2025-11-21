/*!
\file  
\brief A simple program to try out some graph routines

\date 6/12/2008
\author George
\version \verbatim $Id: gkgraph.c 17700 2014-09-27 18:10:02Z karypis $ \endverbatim
*/

#include <GKlib.h>


/*************************************************************************/
/*! Data structures for the code */
/*************************************************************************/
typedef struct {
  int lnbits;
  int cnbits;
  int type;
  int niter;
  float eps;
  float lamda;
  int nosort;
  int write;

  char *infile;
  char *outfile;
} params_t;

/*************************************************************************/
/*! Constants */
/*************************************************************************/
#define CMD_NITER       1
#define CMD_EPS         2
#define CMD_LAMDA       3
#define CMD_TYPE        4
#define CMD_NOSORT      5
#define CMD_WRITE       6
#define CMD_LNBITS      7
#define CMD_CNBITS      8
#define CMD_HELP        10

#define CLINE32 16
#define CLINE64 8
#define MAXRCLOCKSPAN   (1<<20)

/*************************************************************************/
/*! Local variables */
/*************************************************************************/
static struct gk_option long_options[] = {
  {"lnbits",     1,      0,      CMD_LNBITS},
  {"cnbits",     1,      0,      CMD_CNBITS},
  {"type",       1,      0,      CMD_TYPE},
  {"niter",      1,      0,      CMD_NITER},
  {"lamda",      1,      0,      CMD_LAMDA},
  {"eps",        1,      0,      CMD_EPS},
  {"nosort",     0,      0,      CMD_NOSORT},
  {"write",      0,      0,      CMD_WRITE},
  {"help",       0,      0,      CMD_HELP},
  {0,            0,      0,      0}
};


/*-------------------------------------------------------------------*/
/* Mini help  */
/*-------------------------------------------------------------------*/
static char helpstr[][100] = {
" ",
"Usage: gkgraph [options] <graph-file> [<out-file>]",
" ",
" Required parameters",
"  graph-file",
"     The name of the file storing the graph. The file is in ",
"     Metis' graph format.",
" ",
" Optional parameters",
"  -niter=int",
"     Specifies the maximum number of iterations. [default: 100]",
" ",
"  -lnbits=int",
"     Specifies the number of address bits indexing the cacheline. [default: 6]",
" ",
"  -cnbits=int",
"     Specifies the number of address bits indexing the cache. [default: 13]",
" ",
"  -lamda=float",
"     Specifies the follow-the-adjacent-links probability. [default: 0.80]",
" ",
"  -eps=float",
"     Specifies the error tollerance. [default: 1e-10]",
" ",
"  -nosort",
"     Does not sort the adjacency lists.",
" ",
"  -write",
"     Output the reordered graphs.",
" ",
"  -help",
"     Prints this message.",
""
};

static char shorthelpstr[][100] = {
" ",
"   Usage: gkgraph [options] <graph-file> [<out-file>]",
"          use 'gkgraph -help' for a summary of the options.",
""
};
 


/*************************************************************************/
/*! Function prototypes */
/*************************************************************************/
void test_spmv(params_t *params);
void test_tc(params_t *params);
void sort_adjacencies(params_t *params, gk_graph_t *graph);
double compute_spmvstats(params_t *params, gk_graph_t *graph);
double compute_tcstats(params_t *params, gk_graph_t *graph, int32_t *iperm);
int32_t *reorder_degrees(params_t *params, gk_graph_t *graph);
int32_t *reorder_freqlpn(params_t *params, gk_graph_t *graph);
int32_t *reorder_freqlpn_db(params_t *params, gk_graph_t *graph);
int32_t *reorder_minlpn(params_t *params, gk_graph_t *graph);
int32_t *reorder_minlpn_db(params_t *params, gk_graph_t *graph);
void print_init_info(params_t *params, gk_graph_t *graph);
void print_final_info(params_t *params);
params_t *parse_cmdline(int argc, char *argv[]);


/*************************************************************************/
/*! the entry point */
/**************************************************************************/
int main(int argc, char *argv[])
{
  params_t *params;
 
  /* get command-line options */
  params = parse_cmdline(argc, argv);

  test_tc(params);
}


/*************************************************************************/
/*! various spmv-related tests */
/**************************************************************************/
void test_spmv(params_t *params)
{
  ssize_t i, j, v;
  gk_graph_t *graph, *pgraph;
  int32_t *perm;
 
  /* read the data */
  graph = gk_graph_Read(params->infile, GK_GRAPH_FMT_METIS, -1, -1, 0, 0, 0);

  /* display some basic stats */
  print_init_info(params, graph);

  sort_adjacencies(params, graph);
  if (params->write) gk_graph_Write(graph, "original.ijv", GK_GRAPH_FMT_IJV, 1);
  printf("Input            SPMV HitRate: %.4lf\n", compute_spmvstats(params, graph));


  v = RandomInRange(graph->nvtxs);
  gk_graph_ComputeBFSOrdering(graph, v, &perm, NULL);
  pgraph = gk_graph_Reorder(graph, perm, NULL);
  sort_adjacencies(params, pgraph);
  if (params->write) gk_graph_Write(pgraph, "bfs.ijv", GK_GRAPH_FMT_IJV, 1);
  printf("BFS              SPMV HitRate: %.4lf\n", compute_spmvstats(params, pgraph));
  gk_graph_Free(&pgraph);
  gk_free((void **)&perm, LTERM);


  perm = reorder_degrees(params, graph);
  pgraph = gk_graph_Reorder(graph, perm, NULL);
  sort_adjacencies(params, pgraph);
  if (params->write) gk_graph_Write(pgraph, "degrees.ijv", GK_GRAPH_FMT_IJV, 1);
  printf("Degrees          SPMV HitRate: %.4lf\n", compute_spmvstats(params, pgraph));
  gk_graph_Free(&pgraph);
  gk_free((void **)&perm, LTERM);


  perm = reorder_freqlpn(params, graph);
  pgraph = gk_graph_Reorder(graph, perm, NULL);
  sort_adjacencies(params, pgraph);
  if (params->write) gk_graph_Write(pgraph, "freqlpn.ijv", GK_GRAPH_FMT_IJV, 1);
  printf("FreqLabelPropN   SPMV HitRate: %.4lf\n", compute_spmvstats(params, pgraph));
  gk_graph_Free(&pgraph);
  gk_free((void **)&perm, LTERM);

  perm = reorder_freqlpn_db(params, graph);
  pgraph = gk_graph_Reorder(graph, perm, NULL);
  sort_adjacencies(params, pgraph);
  if (params->write) gk_graph_Write(pgraph, "freqlpn-db.ijv", GK_GRAPH_FMT_IJV, 1);
  printf("DBFreqLabelPropN SPMV HitRate: %.4lf\n", compute_spmvstats(params, pgraph));
  gk_graph_Free(&pgraph);
  gk_free((void **)&perm, LTERM);

  perm = reorder_minlpn(params, graph);
  pgraph = gk_graph_Reorder(graph, perm, NULL);
  sort_adjacencies(params, pgraph);
  if (params->write) gk_graph_Write(pgraph, "minlpn.ijv", GK_GRAPH_FMT_IJV, 1);
  printf("MinLabelPropN    SPMV HitRate: %.4lf\n", compute_spmvstats(params, pgraph));
  gk_graph_Free(&pgraph);
  gk_free((void **)&perm, LTERM);

  perm = reorder_minlpn_db(params, graph);
  pgraph = gk_graph_Reorder(graph, perm, NULL);
  sort_adjacencies(params, pgraph);
  if (params->write) gk_graph_Write(pgraph, "minlpn-db.ijv", GK_GRAPH_FMT_IJV, 1);
  printf("DBMinLabelPropN  SPMV HitRate: %.4lf\n", compute_spmvstats(params, pgraph));
  gk_graph_Free(&pgraph);
  gk_free((void **)&perm, LTERM);

  gk_graph_Free(&graph);

  print_final_info(params);

  return;
}


/*************************************************************************/
/*! various tc-related tests */
/**************************************************************************/
void test_tc(params_t *params)
{
  ssize_t i, j, v;
  gk_graph_t *graph, *pgraph;
  int32_t *perm, *iperm;
 
  /* read the data */
  graph = gk_graph_Read(params->infile, GK_GRAPH_FMT_METIS, -1, -1, 0, 0, 0);

  /* display some basic stats */
  print_init_info(params, graph);

  perm = reorder_degrees(params, graph);
  pgraph = gk_graph_Reorder(graph, perm, NULL);
  gk_free((void **)&perm, LTERM);
  sort_adjacencies(params, pgraph);
  iperm = gk_i32incset(graph->nvtxs, 0, gk_i32malloc(graph->nvtxs, "iperm"));
  printf("Degrees          TC HitRate: %.4lf\n", compute_tcstats(params, pgraph, iperm));


  sort_adjacencies(params, pgraph);
  v = RandomInRange(pgraph->nvtxs);
  gk_graph_ComputeBFSOrdering(pgraph, v, &perm, NULL);
  for (i=0; i<graph->nvtxs; i++) iperm[perm[i]] = i;
  gk_free((void **)&perm, LTERM);
  printf("BFS              TC HitRate: %.4lf\n", compute_tcstats(params, pgraph, iperm));


  sort_adjacencies(params, pgraph);
  perm = reorder_freqlpn(params, pgraph);
  for (i=0; i<graph->nvtxs; i++) iperm[perm[i]] = i;
  gk_free((void **)&perm, LTERM);
  printf("FreqLabelPropN   TC HitRate: %.4lf\n", compute_tcstats(params, pgraph, iperm));

  sort_adjacencies(params, pgraph);
  perm = reorder_freqlpn_db(params, pgraph);
  for (i=0; i<graph->nvtxs; i++) iperm[perm[i]] = i;
  gk_free((void **)&perm, LTERM);
  printf("DBFreqLabelPropN TC HitRate: %.4lf\n", compute_tcstats(params, pgraph, iperm));


#ifdef XXX
  perm = reorder_minlpn(params, graph);
  pgraph = gk_graph_Reorder(graph, perm, NULL);
  sort_adjacencies(params, pgraph);
  if (params->write) gk_graph_Write(pgraph, "minlpn.ijv", GK_GRAPH_FMT_IJV, 1);
  printf("MinLabelPropN    SPMV HitRate: %.4lf\n", compute_spmvstats(params, pgraph));
  gk_graph_Free(&pgraph);
  gk_free((void **)&perm, LTERM);

  perm = reorder_minlpn_db(params, graph);
  pgraph = gk_graph_Reorder(graph, perm, NULL);
  sort_adjacencies(params, pgraph);
  if (params->write) gk_graph_Write(pgraph, "minlpn-db.ijv", GK_GRAPH_FMT_IJV, 1);
  printf("DBMinLabelPropN  SPMV HitRate: %.4lf\n", compute_spmvstats(params, pgraph));
  gk_graph_Free(&pgraph);
  gk_free((void **)&perm, LTERM);
#endif

  gk_free((void **)&iperm, LTERM);
  gk_graph_Free(&graph);

  print_final_info(params);

  return;
}


/*************************************************************************/
/*! This function sorts the adjacency lists of the vertices in increasing
    order.
*/
/*************************************************************************/
void sort_adjacencies(params_t *params, gk_graph_t *graph)
{
  uint64_t i, nvtxs;
  ssize_t *xadj; 
  int32_t *adjncy;

  if (params->nosort)
    return;

  nvtxs  = graph->nvtxs;
  xadj   = graph->xadj;
  adjncy = graph->adjncy;

  for (i=0; i<nvtxs; i++) 
    gk_i32sorti(xadj[i+1]-xadj[i], adjncy+xadj[i]);

  return;
}


/*************************************************************************/
/*! This function analyzes the cache locality of an SPMV operation using
    GKlib's cache simulator and returns the cache's hit rate.
 */
/*************************************************************************/
double compute_spmvstats(params_t *params, gk_graph_t *graph)
{
  uint64_t i, nvtxs;
  ssize_t *xadj; 
  int32_t *adjncy, *vec;

  gk_cache_t *cache = gk_cacheCreate(16, params->lnbits, params->cnbits); /* 8MB total; i7 spec */

  nvtxs  = graph->nvtxs;
  xadj   = graph->xadj;
  adjncy = graph->adjncy;

  vec = gk_i32malloc(nvtxs, "vec");
  for (i=0; i<xadj[nvtxs]; i++) {
    gk_cacheLoad(cache, (size_t)(&adjncy[i]));
    gk_cacheLoad(cache, (size_t)(&vec[adjncy[i]]));
  }

  gk_free((void **)&vec, LTERM);

  double hitrate = gk_cacheGetHitRate(cache);
  gk_cacheDestroy(&cache);

  return hitrate;
}


/*************************************************************************/
/*! The hash-map-based triangle-counting routine that uses the JIK
    triangle enumeration scheme.

    This version implements the following:
      - It does not store location information in L
      - Reverts the order within U's adjancency lists to allow ++ traversal
*/
/*************************************************************************/
double compute_tcstats(params_t *params, gk_graph_t *graph, int32_t *iperm)
{
  int32_t vi, vj, vjj, vk, vl, nvtxs;
  ssize_t ei, eiend, eistart, ej, ejend, ejstart;
  int64_t ntriangles;
  ssize_t *xadj, *uxadj;
  int32_t *adjncy;
  int32_t l, hmsize, *hmap;
  
  gk_cache_t *cache = gk_cacheCreate(16, params->lnbits, params->cnbits); 

  nvtxs  = graph->nvtxs;
  xadj   = graph->xadj;
  adjncy = graph->adjncy;

  /* determine the starting location of the upper trianglular part */
  uxadj = gk_zmalloc(nvtxs, "uxadj");
  for (vi=0; vi<nvtxs; vi++) {
    for (ei=xadj[vi], eiend=xadj[vi+1]; ei<eiend && adjncy[ei]<vi; ei++); 
    uxadj[vi] = ei;
    /* flip the order of Adj(vi)'s upper triangular adjacency list */
    for (ej=xadj[vi+1]-1; ei<ej; ei++, ej--) {
      vj = adjncy[ei];
      adjncy[ei] = adjncy[ej];
      adjncy[ej] = vj;
    }
  }

  /* determine the size of the hash-map and convert it into a format
     that is compatible with a bitwise AND operation */
  for (hmsize=0, vi=0; vi<nvtxs; vi++) 
    hmsize = gk_max(hmsize, (int32_t)(xadj[vi+1]-uxadj[vi]));
  for (l=1; hmsize>(1<<l); l++);
  hmsize = (1<<(l+4))-1;
  hmap = gk_i32smalloc(hmsize+1, 0, "hmap");

  for (ntriangles=0, vjj=0; vjj<nvtxs; vjj++) {
    vj = iperm[vjj];

    gk_cacheLoad(cache, (size_t)(&xadj[vj]));
    gk_cacheLoad(cache, (size_t)(&xadj[vj+1]));
    gk_cacheLoad(cache, (size_t)(&uxadj[vj]));

    if (xadj[vj+1]-uxadj[vj] == 0 || uxadj[vj] == xadj[vj])
      continue;

    /* hash Adj(vj) */
    gk_cacheLoad(cache, (size_t)(&uxadj[vj]));
    gk_cacheLoad(cache, (size_t)(&xadj[vj+1]));
    for (ej=uxadj[vj], ejend=xadj[vj+1]; ej<ejend; ej++) {
      gk_cacheLoad(cache, (size_t)(&adjncy[ej]));
      vk = adjncy[ej];
      for (l=(vk&hmsize); 
           gk_cacheLoad(cache, (size_t)(&hmap[l])) && hmap[l]!=0; 
           l=((l+1)&hmsize));
      hmap[l] = vk;
    }

    /* find intersections */
    gk_cacheLoad(cache, (size_t)(&xadj[vj]));
    gk_cacheLoad(cache, (size_t)(&uxadj[vj]));
    for (ej=xadj[vj], ejend=uxadj[vj]; ej<ejend; ej++) {
      gk_cacheLoad(cache, (size_t)(&adjncy[ej]));
      gk_cacheLoad(cache, (size_t)(&uxadj[vi]));
      vi = adjncy[ej];
      for (ei=uxadj[vi]; gk_cacheLoad(cache, (size_t)(&adjncy[ei])) && adjncy[ei]>vj; ei++) {
        vk = adjncy[ei];
        for (l=vk&hmsize; 
             gk_cacheLoad(cache, (size_t)(&hmap[l])) && hmap[l]!=0 && hmap[l]!=vk; 
             l=((l+1)&hmsize));
        gk_cacheLoad(cache, (size_t)(&hmap[l]));
        if (hmap[l] == vk) 
          ntriangles++;
      }
    }

    /* reset hash */
    gk_cacheLoad(cache, (size_t)(&uxadj[vj]));
    gk_cacheLoad(cache, (size_t)(&xadj[vj+1]));
    for (ej=uxadj[vj], ejend=xadj[vj+1]; ej<ejend; ej++) {
      gk_cacheLoad(cache, (size_t)(&adjncy[ej]));
      vk = adjncy[ej];
      for (l=(vk&hmsize); 
           gk_cacheLoad(cache, (size_t)(&hmap[l])) && hmap[l]!=vk; 
           l=((l+1)&hmsize));
      hmap[l] = 0;
    }
  }
  printf("& compatible hmsize: %"PRId32" #triangles: %"PRIu64"\n", hmsize, ntriangles);

  gk_free((void **)&uxadj, &hmap, LTERM);

  //printf("%zd %zd\n", (ssize_t)cache->nhits, (ssize_t)cache->clock);

  double hitrate = gk_cacheGetHitRate(cache);
  gk_cacheDestroy(&cache);

  return hitrate;
}


/*************************************************************************/
/*! This function computes an increasing degree ordering 
*/
/*************************************************************************/
int32_t *reorder_degrees(params_t *params, gk_graph_t *graph)
{
  int i, v, u, nvtxs, range;
  ssize_t j, *xadj; 
  int32_t *counts, *perm;

  nvtxs  = graph->nvtxs;
  xadj   = graph->xadj;

  for (range=0, i=0; i<nvtxs; i++) 
    range = gk_max(range, xadj[i+1]-xadj[i]);
  range++;

  counts = gk_i32smalloc(range+1, 0, "counts");
  for (i=0; i<nvtxs; i++)
    counts[xadj[i+1]-xadj[i]]++;
  MAKECSR(i, range, counts);

  perm = gk_i32malloc(nvtxs, "perm");
  for (i=0; i<nvtxs; i++)
    perm[i] = counts[xadj[i+1]-xadj[i]]++;

  gk_free((void **)&counts, LTERM);

  return perm;
}


/*************************************************************************/
/*! This function re-orders the graph by:
    - performing a fixed number of most-popular label propagation iterations
    - locally renumbers the vertices with the same label
*/
/*************************************************************************/
int32_t *reorder_freqlpn(params_t *params, gk_graph_t *graph)
{
  int32_t i, ii, k, nvtxs, maxlbl;
  ssize_t j, *xadj; 
  int32_t *adjncy, *labels, *freq, *perm;
  gk_i32kv_t *cand;

  nvtxs  = graph->nvtxs;
  xadj   = graph->xadj;
  adjncy = graph->adjncy;

  labels = gk_i32incset(nvtxs, 0, gk_i32malloc(nvtxs, "labels"));
  freq   = gk_i32smalloc(nvtxs, 0, "freq");
  perm   = gk_i32incset(nvtxs, 0, gk_i32malloc(nvtxs, "perm"));

  for (k=0; k<params->niter; k++) {
    gk_i32randArrayPermuteFine(nvtxs, perm, 0);
    for (ii=0; ii<nvtxs; ii++) {
      i = perm[ii];
      maxlbl = labels[adjncy[xadj[i]]];
      freq[maxlbl] = 1;
      for (j=xadj[i]+1; j<xadj[i+1]; j++) {
        freq[labels[adjncy[j]]]++;
        if (freq[maxlbl] < freq[labels[adjncy[j]]])
          maxlbl = labels[adjncy[j]];
        else if (freq[maxlbl] == freq[labels[adjncy[j]]]) {
          if (RandomInRange(2))
            maxlbl = labels[adjncy[j]];
        }
      }
      for (j=xadj[i]; j<xadj[i+1]; j++) 
        freq[labels[adjncy[j]]] = 0;
      labels[i] = maxlbl;
    }
  }

  cand = gk_i32kvmalloc(nvtxs, "cand");
  for (i=0; i<nvtxs; i++) {
    cand[i].key = labels[i];
    cand[i].val = i;
  }
  gk_i32kvsorti(nvtxs, cand);

  for (i=0; i<nvtxs; i++)
    perm[cand[i].val] = i;

  gk_free((void **)&labels, &freq, &cand, LTERM);

  return perm;
}


/*************************************************************************/
/*! This function re-orders the graph by:
    - performing a fixed number of most-popular label propagation iterations
    - restricts that propagation to take place within similar degree buckets
      of vertices
    - locally renumbers the vertices with the same label
*/
/*************************************************************************/
int32_t *reorder_freqlpn_db(params_t *params, gk_graph_t *graph)
{
  int32_t i, ii, k, nvtxs, maxlbl;
  ssize_t j, *xadj; 
  int32_t *adjncy, *labels, *freq, *perm, *dbucket;
  gk_i32kv_t *cand;

  nvtxs  = graph->nvtxs;
  xadj   = graph->xadj;
  adjncy = graph->adjncy;

  labels  = gk_i32incset(nvtxs, 0, gk_i32malloc(nvtxs, "labels"));
  freq    = gk_i32smalloc(nvtxs, 0, "freq");
  perm    = gk_i32incset(nvtxs, 0, gk_i32malloc(nvtxs, "perm"));
  dbucket = gk_i32malloc(nvtxs, "dbucket");

  for (i=0; i<nvtxs; i++)
    dbucket[i] = ((xadj[i+1]-xadj[i])>>3);

  for (k=0; k<params->niter; k++) {
    gk_i32randArrayPermuteFine(nvtxs, perm, 0);
    for (ii=0; ii<nvtxs; ii++) {
      i = perm[ii];
      maxlbl = labels[i];
      for (j=xadj[i]; j<xadj[i+1]; j++) {
        if (dbucket[i] != dbucket[adjncy[j]])
          continue;

        freq[labels[adjncy[j]]]++;
        if (freq[maxlbl] < freq[labels[adjncy[j]]])
          maxlbl = labels[adjncy[j]];
        else if (freq[maxlbl] == freq[labels[adjncy[j]]]) {
          if (RandomInRange(2))
            maxlbl = labels[adjncy[j]];
        }
      }
      for (j=xadj[i]; j<xadj[i+1]; j++) 
        freq[labels[adjncy[j]]] = 0;
      labels[i] = maxlbl;
    }
  }

  cand = gk_i32kvmalloc(nvtxs, "cand");
  for (i=0; i<nvtxs; i++) {
    cand[i].key = labels[i];
    cand[i].val = i;
  }
  gk_i32kvsorti(nvtxs, cand);

  for (i=0; i<nvtxs; i++)
    perm[cand[i].val] = i;

  gk_free((void **)&labels, &freq, &dbucket, &cand, LTERM);

  return perm;
}


/*************************************************************************/
/*! This function re-orders the graph by:
    - performing a fixed number of min-label propagation iterations
    - locally renumbers the vertices with the same label
*/
/*************************************************************************/
int32_t *reorder_minlpn(params_t *params, gk_graph_t *graph)
{
  int32_t i, ii, k, nvtxs, minlbl;
  ssize_t j, *xadj; 
  int32_t *adjncy, *labels, *perm;
  gk_i32kv_t *cand;

  nvtxs  = graph->nvtxs;
  xadj   = graph->xadj;
  adjncy = graph->adjncy;

  labels = gk_i32incset(nvtxs, 0, gk_i32malloc(nvtxs, "labels"));
  perm   = gk_i32incset(nvtxs, 0, gk_i32malloc(nvtxs, "perm"));

  for (k=0; k<params->niter; k++) {
    for (i=0; i<nvtxs; i++) {
      minlbl = labels[i];
      for (j=xadj[i]; j<xadj[i+1]; j++) {
        if (minlbl > labels[adjncy[j]])
          minlbl = labels[adjncy[j]];
      }
      labels[i] = minlbl;
    }
  }

  cand = gk_i32kvmalloc(nvtxs, "cand");
  for (i=0; i<nvtxs; i++) {
    cand[i].key = labels[i];
    cand[i].val = i;
  }
  gk_i32kvsorti(nvtxs, cand);

  for (i=0; i<nvtxs; i++) {
    perm[cand[i].val] = i;
    //if (i>0 && cand[i].key != cand[i-1].key)
    //  printf("%10d %10d\n", i-1, cand[i-1].key);
  }
  //printf("%10d %10d\n", i-1, cand[i-1].key);

  gk_free((void **)&labels, &cand, LTERM);

  return perm;
}


/*************************************************************************/
/*! This function re-orders the graph by:
    - performing a fixed number of min-label propagation iterations 
    - restricts that propagation to take place within similar degree buckets
      of vertices
    - locally renumbers the vertices with the same label
*/
/*************************************************************************/
int32_t *reorder_minlpn_db(params_t *params, gk_graph_t *graph)
{
  int32_t i, ii, k, nvtxs, minlbl;
  ssize_t j, *xadj; 
  int32_t *adjncy, *labels, *perm, *dbucket;
  gk_i32kv_t *cand;

  nvtxs  = graph->nvtxs;
  xadj   = graph->xadj;
  adjncy = graph->adjncy;

  labels  = gk_i32incset(nvtxs, 0, gk_i32malloc(nvtxs, "labels"));
  perm    = gk_i32incset(nvtxs, 0, gk_i32malloc(nvtxs, "perm"));
  dbucket = gk_i32malloc(nvtxs, "dbucket");

  for (i=0; i<nvtxs; i++)
    dbucket[i] = ((xadj[i+1]-xadj[i])>>3);

  for (k=0; k<params->niter; k++) {
    for (i=0; i<nvtxs; i++) {
      minlbl = labels[i];
      for (j=xadj[i]; j<xadj[i+1]; j++) {
        if (dbucket[i] != dbucket[adjncy[j]])
          continue;

        if (minlbl > labels[adjncy[j]])
          minlbl = labels[adjncy[j]];
      }
      labels[i] = minlbl;
    }
  }

  cand = gk_i32kvmalloc(nvtxs, "cand");
  for (i=0; i<nvtxs; i++) {
    cand[i].key = labels[i];
    cand[i].val = i;
  }
  gk_i32kvsorti(nvtxs, cand);

  for (i=0; i<nvtxs; i++) {
    perm[cand[i].val] = i;
    //if (i>0 && cand[i].key != cand[i-1].key)
    //  printf("%10d %10d\n", i-1, cand[i-1].key);
  }
  //printf("%10d %10d\n", i-1, cand[i-1].key);

  gk_free((void **)&labels, &dbucket, &cand, LTERM);

  return perm;
}


/*************************************************************************/
/*! This function prints run parameters */
/*************************************************************************/
void print_init_info(params_t *params, gk_graph_t *graph)
{
  printf("*******************************************************************************\n");
  printf(" gkgraph\n\n");
  printf("Graph Information ----------------------------------------------------------\n");
  printf(" input file=%s, [%d, %zd]\n", 
      params->infile, graph->nvtxs, graph->xadj[graph->nvtxs]);

  printf("\n");
  printf("Options --------------------------------------------------------------------\n");
  printf(" lnbits=%d, cnbits=%d, type=%d, niter=%d, lamda=%f, eps=%e\n",
      params->lnbits, params->cnbits, params->type, params->niter, 
      params->lamda, params->eps);

  printf("\n");
  printf("Working... -----------------------------------------------------------------\n");
}


/*************************************************************************/
/*! This function prints final statistics */
/*************************************************************************/
void print_final_info(params_t *params)
{
  printf("\n");
  printf("Memory Usage Information -----------------------------------------------------\n");
  printf("   Maximum memory used:              %10zd bytes\n", (ssize_t) gk_GetMaxMemoryUsed());
  printf("   Current memory used:              %10zd bytes\n", (ssize_t) gk_GetCurMemoryUsed());
  printf("********************************************************************************\n");
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

  /* initialize the params data structure */
  params->lnbits    = 6;
  params->cnbits    = 13;
  params->type      = 1;
  params->niter     = 1;
  params->eps       = 1e-10;
  params->lamda     = 0.20;
  params->nosort    = 0;
  params->write     = 0;
  params->infile    = NULL;


  /* Parse the command line arguments  */
  while ((c = gk_getopt_long_only(argc, argv, "", long_options, &option_index)) != -1) {
    switch (c) {
      case CMD_LNBITS:
        if (gk_optarg) params->lnbits = atoi(gk_optarg);
        break;
      case CMD_CNBITS:
        if (gk_optarg) params->cnbits = atoi(gk_optarg);
        break;
      case CMD_TYPE:
        if (gk_optarg) params->type = atoi(gk_optarg);
        break;
      case CMD_NITER:
        if (gk_optarg) params->niter = atoi(gk_optarg);
        break;
      case CMD_EPS:
        if (gk_optarg) params->eps = atof(gk_optarg);
        break;
      case CMD_LAMDA:
        if (gk_optarg) params->lamda = atof(gk_optarg);
        break;
      case CMD_NOSORT:
        params->nosort = 1;
        break;
      case CMD_WRITE:
        params->write = 1;
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

  if (argc-gk_optind != 1) {
    printf("Unrecognized parameters.");
    for (i=0; strlen(shorthelpstr[i]) > 0; i++)
      printf("%s\n", shorthelpstr[i]);
    exit(0);
  }

  params->infile  = gk_strdup(argv[gk_optind++]);

  if (argc-gk_optind > 0) 
    params->outfile = gk_strdup(argv[gk_optind++]);
  else
    params->outfile   = gk_strdup("gkgraph.out");

  if (!gk_fexists(params->infile))
    errexit("input file %s does not exist.\n", params->infile);

  return params;
}

