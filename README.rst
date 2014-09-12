PyMetis is a Python wrapper for the `Metis
<http://glaros.dtc.umn.edu/gkhome/views/metis>`_ graph partititioning software
by George Karypis, Vipin Kumar and others. It includes version 5.1.0 of Metis
and wraps it using the `Boost Python <http://www.boost.org/libs/python/doc/>`_
wrapper generator library. So far, it only wraps the most basic graph
partitioning functionality (which is enough for my current use), but extending
it in case you need more should be quite straightforward. Using PyMetis to
partition your meshes is really easy--essentially all you need to pass into
PyMetis is an adjacency list for the graph and the number of parts you would
like.

Installation
============

The following line should do the job::

    pip install pymetis
