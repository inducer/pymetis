#include <boost/foreach.hpp>
#include <boost/python.hpp>
#include <boost/scoped_array.hpp>
#include <boost/python/stl_iterator.hpp>
#include <metis.h>
#include <vector>
#include <algorithm>




using namespace boost::python;
using namespace std;




#define COPY_IDXTYPE_LIST(NAME) \
  { \
    stl_input_iterator<idxtype> begin(NAME##_py), end; \
    std::copy(begin, end, back_inserter(NAME)); \
  }
#define ITL(NAME) (&NAME.front())

#define COPY_OUTPUT(NAME, LEN) \
  list NAME##_py; \
  { \
    BOOST_FOREACH(idxtype i, std::make_pair(NAME.get(), NAME.get()+(LEN))) \
        NAME##_py.append(i); \
  }




namespace
{
  object wrap_part_graph(
      int nparts,
      const object &xadj_py,
      const object &adjncy_py,
      const object &vwgt_py,
      const object &adjwgt_py,
      bool recursive)
  {
    int n = len(xadj_py) - 1;
    vector<idxtype> xadj, adjncy, vwgt, adjwgt;
    COPY_IDXTYPE_LIST(xadj);
    COPY_IDXTYPE_LIST(adjncy);

    int wgtflag = 0;
    if (vwgt_py != object())
    {
      COPY_IDXTYPE_LIST(vwgt);
      wgtflag |= 2;
    }
    if (adjwgt_py != object())
    {
      COPY_IDXTYPE_LIST(adjwgt);
      wgtflag |= 1;
    }
    int numflag = 0;

    int options[5];
    options[0] = 0;

    int edgecut;
    boost::scoped_array<idxtype> part(new idxtype[n]);

    if (recursive)
      METIS_PartGraphRecursive(&n, ITL(xadj), ITL(adjncy), ITL(vwgt), ITL(adjwgt),
          &wgtflag, &numflag, &nparts, options, &edgecut, part.get());
    else
      METIS_PartGraphKway(&n, ITL(xadj), ITL(adjncy), ITL(vwgt), ITL(adjwgt),
          &wgtflag, &numflag, &nparts, options, &edgecut, part.get());

    COPY_OUTPUT(part, n);

    return make_tuple(edgecut, part_py);
  }
}




BOOST_PYTHON_MODULE(_internal)
{
  def("part_graph", wrap_part_graph);
}
