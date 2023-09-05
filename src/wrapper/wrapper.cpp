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
#define COPY_REALTYPE_LIST(NAME) \
  { \
    for (auto it: NAME##_py) \
      NAME.push_back(py::cast<real_t>(*it)); \
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

  py::object
  wrap_part_mesh(idx_t &nParts,
    const py::object &connectivityOffsets_py,
    const py::object &connectivity_py,
    const py::object &tpwgts_py,
    idx_t &gtype,
    idx_t &nElements,
    idx_t &nVertex,
    idx_t &ncommon,
    metis_options &options)
  {
    idx_t edgeCuts = 0;
    std::unique_ptr<idx_t []> elemPart(new idx_t[nElements]);
    std::unique_ptr<idx_t []> vertPart(new idx_t[nVertex]);

    std::vector<idx_t> connectivityOffsets, connectivity;
    std::vector<real_t> tpwgts;
    COPY_IDXTYPE_LIST(connectivityOffsets);
    COPY_IDXTYPE_LIST(connectivity);
    COPY_REALTYPE_LIST(tpwgts);
    real_t* pTpwgts = nullptr;
    if(tpwgts.size() != 0)
        pTpwgts = tpwgts.data();


    if(gtype == METIS_GTYPE_NODAL) 
    {
        int info = METIS_PartMeshNodal(&nElements, &nVertex,
          connectivityOffsets.data(), connectivity.data(),
          nullptr, nullptr, &nParts, pTpwgts, options.m_options,
          &edgeCuts, elemPart.get(), vertPart.get());
        assert_ok(info, "METIS_PartMeshNodal failed");
    }
    else if(gtype == METIS_GTYPE_DUAL)
    {
        idx_t objval = 1;
        int info = METIS_PartMeshDual(&nElements, &nVertex,
          connectivityOffsets.data(), connectivity.data(),
          nullptr, nullptr, &ncommon, &nParts, pTpwgts, options.m_options,
          &objval, elemPart.get(), vertPart.get());
        assert_ok(info, "METIS_PartMeshNodal failed");
    }
    else {
        assert_ok(METIS_ERROR_INPUT, "Invalid value. "\
            "`gtype` is supposed to be either `METIS_GTYPE_NODAL`"\
            " or `METIS_GTYPE_DUAL`.");
    }
    COPY_OUTPUT(elemPart, nElements);
    COPY_OUTPUT(vertPart, nVertex);

    return py::make_tuple(edgeCuts, elemPart_py, vertPart_py);
  }

  class options_indices { };
  class Status { };
  class OPType { };
  class OptionKey { };
  class PType { };
  class GType { };
  class CType { };
  class IPType { };
  class RType { };
  class DebugLevel { };
  class ObjType { };
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

    ADD_OPT(PTYPE);
    ADD_OPT(OBJTYPE);
    ADD_OPT(CTYPE);
    ADD_OPT(IPTYPE);
    ADD_OPT(RTYPE);
    ADD_OPT(DBGLVL);
    ADD_OPT(NITER);
    ADD_OPT(NCUTS);
    ADD_OPT(SEED);
    ADD_OPT(NO2HOP);
    ADD_OPT(MINCONN);
    ADD_OPT(CONTIG);
    ADD_OPT(COMPRESS);
    ADD_OPT(CCORDER);
    ADD_OPT(PFACTOR);
    ADD_OPT(NSEPS);
    ADD_OPT(UFACTOR);
    ADD_OPT(NUMBERING);

    ADD_OPT(HELP);
    ADD_OPT(TPWGTS);
    ADD_OPT(NCOMMON);
    ADD_OPT(NOOUTPUT);
    ADD_OPT(BALANCE);
    ADD_OPT(GTYPE);
    ADD_OPT(UBVEC);

#undef ADD_OPT
  }
#define DEF_CLASS(NAME) py::class_<NAME> cls(m, #NAME)
#define ADD_ENUM(NAME, VALUE) cls.def_property_readonly_static(#NAME,\
      [](py::object self) { return (int) METIS_##VALUE; })
  {
    {
      DEF_CLASS(Status);
      ADD_ENUM(OK, OK);
      ADD_ENUM(ERROR_INPUT, ERROR_INPUT);
      ADD_ENUM(ERROR_MEMORY, ERROR_MEMORY);
      ADD_ENUM(ERROR, ERROR);
    }
    {
      DEF_CLASS(OPType);
      ADD_ENUM(PMETIS, OP_PMETIS);
      ADD_ENUM(KMETIS, OP_KMETIS);
      ADD_ENUM(OMETIS, OP_OMETIS);
    }
    { 
      DEF_CLASS(OptionKey);
      ADD_ENUM(PTYPE, OPTION_PTYPE);
      ADD_ENUM(OBJTYPE, OPTION_OBJTYPE);
      ADD_ENUM(CTYPE, OPTION_CTYPE);
      ADD_ENUM(IPTYPE, OPTION_IPTYPE);
      ADD_ENUM(RTYPE, OPTION_RTYPE);
      ADD_ENUM(DBGLVL, OPTION_DBGLVL);
      ADD_ENUM(NITER, OPTION_NITER);
      ADD_ENUM(NCUTS, OPTION_NCUTS);
      ADD_ENUM(SEED, OPTION_SEED);
      ADD_ENUM(NO2HOP, OPTION_NO2HOP);
      ADD_ENUM(MINCONN, OPTION_MINCONN);
      ADD_ENUM(CONTIG, OPTION_CONTIG);
      ADD_ENUM(COMPRESS, OPTION_COMPRESS);
      ADD_ENUM(CCORDER, OPTION_CCORDER);
      ADD_ENUM(PFACTOR, OPTION_PFACTOR);
      ADD_ENUM(NSEPS, OPTION_NSEPS);
      ADD_ENUM(UFACTOR, OPTION_UFACTOR);
      ADD_ENUM(NUMBERING, OPTION_NUMBERING);
      ADD_ENUM(HELP, OPTION_HELP);
      ADD_ENUM(TPWGTS, OPTION_TPWGTS);
      ADD_ENUM(NCOMMON, OPTION_NCOMMON);
      ADD_ENUM(NOOUTPUT, OPTION_NOOUTPUT);
      ADD_ENUM(BALANCE, OPTION_BALANCE);
      ADD_ENUM(GTYPE, OPTION_GTYPE);
      ADD_ENUM(UBVEC, OPTION_UBVEC);
    }
    {
      DEF_CLASS(PType);
      ADD_ENUM(RB, PTYPE_RB);
      ADD_ENUM(KWAY, PTYPE_KWAY);
    }
    {

      DEF_CLASS(GType);
      ADD_ENUM(DUAL, GTYPE_DUAL);
      ADD_ENUM(NODAL, GTYPE_NODAL);
    }
    {
      DEF_CLASS(CType);
      ADD_ENUM(RM, CTYPE_RM);
      ADD_ENUM(SHEM, CTYPE_SHEM);
    }
    {
      DEF_CLASS(IPType);
      ADD_ENUM(GROW, IPTYPE_GROW);
      ADD_ENUM(RANDOM, IPTYPE_RANDOM);
      ADD_ENUM(EDGE, IPTYPE_EDGE);
      ADD_ENUM(NODE, IPTYPE_NODE);
      ADD_ENUM(METISRB, IPTYPE_METISRB);
    }
    {
      DEF_CLASS(RType);
      ADD_ENUM(FM, RTYPE_FM);
      ADD_ENUM(GREEDY, RTYPE_GREEDY);
      ADD_ENUM(SEP2SIDED, RTYPE_SEP2SIDED);
      ADD_ENUM(SEP1SIDED, RTYPE_SEP1SIDED);
    }
    {
      DEF_CLASS(DebugLevel);
      ADD_ENUM(INFO, DBG_INFO);
      ADD_ENUM(TIME, DBG_TIME);
      ADD_ENUM(COARSEN, DBG_COARSEN);
      ADD_ENUM(REFINE, DBG_REFINE);
      ADD_ENUM(IPART, DBG_IPART);
      ADD_ENUM(MOVEINFO, DBG_MOVEINFO);
      ADD_ENUM(SEPINFO, DBG_SEPINFO);
      ADD_ENUM(CONNINFO, DBG_CONNINFO);
      ADD_ENUM(CONTIGINFO, DBG_CONTIGINFO);
      ADD_ENUM(MEMORY, DBG_MEMORY);
    }
    {
      DEF_CLASS(ObjType);
      ADD_ENUM(CUT, OBJTYPE_CUT);
      ADD_ENUM(VOL, OBJTYPE_VOL);
      ADD_ENUM(NODE, OBJTYPE_NODE);
    }
  }
#undef ADD_ENUM
#undef DEF_CLASS

  m.def("verify_nd", wrap_verify_nd);
  m.def("node_nd", wrap_node_nd);
  m.def("edge_nd", wrap_node_nd);  // DEPRECATED
  m.def("part_graph", wrap_part_graph);
  m.def("part_mesh", wrap_part_mesh);
}
