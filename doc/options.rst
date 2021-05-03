Configuration
=============

METIS has a number of configuration options that can be specified using the `pymetis.Options` class

.. code:: python

    import pymetis
    options = pymetis.Options()
    options.contig = 1     # Require contiguous partitions
    options.seed   = 1234  # Random seed for partitioning
 
    ...
 
    n_cuts, membership = pymetis.part_graph(
        n_parts,
        adjacency=adjacency_list,
        options=options
    ) 

The following options are available. For a description of the options see the
METIS documentation.

.. list-table:: Configuration Options
   :widths: 25 25
   :header-rows: 1

   * - pyMetis
     - METIS
   * - options.ncuts
     - METIS_OPTION_NCUTS
   * - options.nseps
     - METIS_OPTION_NSEPS
   * - options.numbering
     - METIS_OPTION_NUMBERING
   * - options.niter
     - METIS_OPTION_NITER
   * - options.minconn
     - METIS_OPTION_MINCONN
   * - options.no2hop
     - METIS_OPTION_NO2HOP
   * - options.seed
     - METIS_OPTION_SEED
   * - options.contig
     - METIS_OPTION_CONTIG
   * - options.compress
     - METIS_OPTION_COMPRESS
   * - options.ccorder
     - METIS_OPTION_CCORDER
   * - options.pfactor
     - METIS_OPTION_PFACTOR
   * - options.ufactor
     - METIS_OPTION_UFACTOR
