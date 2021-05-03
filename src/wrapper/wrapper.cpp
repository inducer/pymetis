#include <pybind11/pybind11.h>
#include <metis.h>
#include <memory>
#include <vector>
#include <algorithm>
#include <stdexcept>


namespace py = pybind11;
using namespace std;




#define COPY_IDXTYPE_LIST(NAME) \
  { \
    for (auto it: NAME##_py) \
      NAME.push_back(py::cast<idx_t>(*it)); \
  }

#define COPY_OUTPUT(NAME, LEN) \
  py::list NAME##_py; \
  { \
    for (idx_t i = 0; i<LEN; ++i) \
      NAME##_py.append(NAME.get()[i]); \
  }

namespace
{
  enum {
        DEFAULT = 0
  };

  inline idx_t * maybe_data(vector<idx_t> & vect)
  {
    if (vect.empty())
    {
      return NULL;
    }
    else
    {
      return &vect.front();
    }
  }

  inline void assert_ok(idx_t info, const char * message)
  {
    switch (info) {
      case METIS_OK:
        return;
      case METIS_ERROR_INPUT:
        throw new invalid_argument(message);
      case METIS_ERROR_MEMORY:
        throw new bad_alloc();
      case METIS_ERROR:
        throw new logic_error(message);
      default:
        throw new runtime_error(message);
    }
  }

  /**
   * This function verifies that the partitioning was computed correctly.
   */
  int
  wrap_verify_nd(const py::object &perm_py, const py::object &iperm_py)
  {
    int rcode=0;
    idx_t i;
    idx_t nvtxs = py::len(perm_py);

    vector<idx_t> perm, iperm;
    COPY_IDXTYPE_LIST(perm);
    COPY_IDXTYPE_LIST(iperm);

    for (i=0; i<nvtxs; i++)
      if (i != perm[iperm[i]])
        rcode = 1;

    for (i=0; i<nvtxs; i++)
      if (i != iperm[perm[i]])
        rcode = 2;

    return rcode;
  }


  class metis_options
  {
    public:
      idx_t m_options[METIS_NOPTIONS];

      metis_options()
      {
        METIS_SetDefaultOptions(m_options);
      }

      idx_t get(int i) const
      {
        if (i < 0 || i >= METIS_NOPTIONS)
          throw new invalid_argument("options index is out of range");

        return m_options[i];
      }

      void set(int i, idx_t value)
      {
        if (i < 0 || i >= METIS_NOPTIONS)
          throw new invalid_argument("options index is out of range");

        m_options[i] = value;
      }

      void set_defaults()
      {
        METIS_SetDefaultOptions(m_options);
      }
  };


  py::object
  wrap_node_nd(const py::object &xadj_py, const py::object &adjncy_py,
      metis_options &options)
  {
    idx_t nvtxs = py::len(xadj_py) - 1;

    vector<idx_t> xadj, adjncy;
    COPY_IDXTYPE_LIST(xadj);
    COPY_IDXTYPE_LIST(adjncy);
    idx_t * vwgt = NULL;

    std::unique_ptr<idx_t []> perm(new idx_t[nvtxs]);
    std::unique_ptr<idx_t []> iperm(new idx_t[nvtxs]);

    int info = METIS_NodeND(
      &nvtxs, &xadj.front(), &adjncy.front(), vwgt, options.m_options,
      perm.get(), iperm.get());

    assert_ok(info, "METIS_NodeND failed");

    COPY_OUTPUT(perm, nvtxs);
    COPY_OUTPUT(iperm, nvtxs);

    return py::make_tuple(perm_py, iperm_py);
  }

  py::object
  wrap_part_graph(
      idx_t nparts,
      const py::object &xadj_py,
      const py::object &adjncy_py,
      const py::object &vwgt_py,
      const py::object &adjwgt_py,
      metis_options &options,
      bool recursive)
  {
    idx_t nvtxs = py::len(xadj_py) - 1;
    vector<idx_t> xadj, adjncy, vwgt, adjwgt;
    COPY_IDXTYPE_LIST(xadj);
    COPY_IDXTYPE_LIST(adjncy);

    // pymetis does not currently support partition weights and constraints.
    idx_t ncon = 1;
    real_t * tpwgts = NULL;
    real_t * pubvec = NULL;

    // pymetis defaults to the minimizing-edge-cut objective
    idx_t * pvsize = NULL;

    if (!vwgt_py.is_none())
    {
      COPY_IDXTYPE_LIST(vwgt);
    }
    if (!adjwgt_py.is_none())
    {
      COPY_IDXTYPE_LIST(adjwgt);
    }

    idx_t edgecut;
    std::unique_ptr<idx_t []> part(new idx_t[nvtxs]);

    if (recursive)
    {
      int info = METIS_PartGraphRecursive(
        &nvtxs, &ncon, &xadj.front(), &adjncy.front(),
        maybe_data(vwgt), pvsize, maybe_data(adjwgt), &nparts, tpwgts,
        pubvec, options.m_options, &edgecut, part.get());

      assert_ok(info, "METIS_PartGraphRecursive failed");
    }
    else
    {
      int info = METIS_PartGraphKway(
        &nvtxs, &ncon, &xadj.front(), &adjncy.front(),
        maybe_data(vwgt), pvsize, maybe_data(adjwgt), &nparts, tpwgts,
        pubvec, options.m_options, &edgecut, part.get());

      assert_ok(info, "METIS_PartGraphKway failed");
    }

    COPY_OUTPUT(part, nvtxs);

    return py::make_tuple(edgecut, part_py);
  }

  class options_indices { };
}

PYBIND11_MODULE(_internal, m)
{
  {
    typedef metis_options cls;
    py::class_<cls>(m, "Options")
      .def(py::init<>())
      .def("_len", []() { return METIS_NOPTIONS; })
      .def("_get", &cls::get)
      .def("_set", &cls::set)
      .def("set_defaults", &cls::set_defaults)
      ;
  }
  {
#define ADD_OPT(NAME) cls.def_property_readonly_static(#NAME,\
      [](py::object self) { return (int) METIS_OPTION_##NAME; })
    py::class_<options_indices> cls(m, "options_indices");

    ADD_OPT(NCUTS);
    ADD_OPT(NSEPS);
    ADD_OPT(NUMBERING);
    ADD_OPT(NITER);
    ADD_OPT(SEED);
    ADD_OPT(MINCONN);
    ADD_OPT(NO2HOP);
    ADD_OPT(CONTIG);
    ADD_OPT(COMPRESS);
    ADD_OPT(CCORDER);
    ADD_OPT(PFACTOR);
    ADD_OPT(UFACTOR);

#undef ADD_OPT
  }

  m.def("verify_nd", wrap_verify_nd);
  m.def("node_nd", wrap_node_nd);
  m.def("edge_nd", wrap_node_nd);  // DEPRECATED
  m.def("part_graph", wrap_part_graph);
}
