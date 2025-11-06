#include <pybind11/pybind11.h>
#include <pybind11/warnings.h>
#include <metis.h>
#include <memory>
#include <vector>
#include <stdexcept>


namespace py = pybind11;
using namespace std;




namespace
{
  enum {
        DEFAULT = 0
  };

  class noncopyable {
  public:
    noncopyable() = default;
    ~noncopyable() = default;

  private:
    noncopyable(const noncopyable&) = delete;
    noncopyable& operator=(const noncopyable&) = delete;
  };


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


  // after making the array, these should be checked for size accuracy!
  template<class T>
  char typecode_for_type()
  {
    throw new logic_error("no type code known for type");
  }

  template<>
  char typecode_for_type<idx_t>()
  {
    if (IDXTYPEWIDTH == 64)
      return 'q';
    else
      return 'i';
  }

  template<class T>
  const char *typecodes_for_type()
  {
    throw new logic_error("no type code known for type");
  }

  template<>
  const char *typecodes_for_type<idx_t>()
  {
    return "ilq";
  }

  template<>
  const char *typecodes_for_type<real_t>()
  {
    return "f";
  }

  bool contains_one_of(const char *chars, const char *haystack)
  {
    while (*chars && !strchr(haystack, *chars))
      ++chars;
    return *chars;
  }


  template<class T>
  class array_for_py
  {
    py::bytearray m_bytearray;

    public:
      array_for_py(size_t size)
      : m_bytearray(nullptr, size * sizeof(T))
      {}

      T *get()
      {
        return reinterpret_cast<T *>(PyByteArray_AS_STRING(m_bytearray.ptr()));
      }

      py::object as_array()
      {
        py::module_ array_mod(py::module_::import("array"));
        py::object ary = array_mod.attr("array")(typecode_for_type<T>(), m_bytearray);

        if (ary.attr("itemsize").cast<int>() != sizeof(T))
          throw new logic_error("failed to identify size of output data type");

        return ary;
      }
  };


  class not_a_buffer_error : public std::runtime_error
  {
    using std::runtime_error::runtime_error;
  };


  struct py_buffer_wrapper : public noncopyable
  {
    Py_buffer m_buf;

    py_buffer_wrapper(const char *name, PyObject *obj, int flags, bool warn_on_failure)
    {
      if (PyObject_GetBuffer(obj, &m_buf, flags)) {
        if (warn_on_failure)
        {
          py::handle etype, evalue, etraceback;
          PyErr_Fetch(&etype.ptr(), &evalue.ptr(), &etraceback.ptr());
          try
          {
            std::string msg = py::str("For {}: GetBuffer failed: {}").attr("format")(
                                        py::str(name),
                                        evalue.ptr()
                                        ? py::str(evalue)
                                        : py::str("<no message>")
                                      ).cast<std::string>();
            py::warnings::warn(msg.c_str(), PyExc_BytesWarning, 3);
          }
          catch (...)
          {
            etype.dec_ref();
            evalue.dec_ref();
            etraceback.dec_ref();
          }
        }
        else
          PyErr_Clear();
        throw not_a_buffer_error("not a buffer object");
      }
    }

    virtual ~py_buffer_wrapper()
    {
      PyBuffer_Release(&m_buf);
    }
  };


  template<class T>
  class array_from_py
  {
    std::unique_ptr<std::vector<T>> m_vec;
    std::unique_ptr<py_buffer_wrapper> m_buf;
    std::size_t m_size;

    public:
      array_from_py(const char *name, py::object obj, bool warn_on_copies, bool required = true)
      {
        if (Py_IsNone(obj.ptr()))
        {
          if (required)
            throw py::value_error(
                      std::string("missing a required sequence/buffer value for ")
                    + std::string(name));
          m_size = 0;
          return;
        }

        try
        {
          m_buf.reset(new py_buffer_wrapper(name, obj.ptr(), PyBUF_FORMAT, warn_on_copies));
        }
        catch (not_a_buffer_error &ex)
        {
          m_buf.reset(nullptr);
        }

        // check buffer for suitability
        if (m_buf.get() && m_buf->m_buf.itemsize != sizeof(T))
        {
          if (warn_on_copies)
          {
            std::string msg = py::str("For {}: unexpected itemsize").attr("format")(
                                        py::str(name)
                                      ).cast<std::string>();
            py::warnings::warn(msg.c_str(), PyExc_BytesWarning, 3);
          }

          m_buf.reset(nullptr);
        }

        const char *needed_tcs = typecodes_for_type<T>();
        if (m_buf.get() && !contains_one_of(needed_tcs, m_buf->m_buf.format))
        {
          if (warn_on_copies)
          {
            std::string msg = py::str("For {}: unexpected dtype, needed one of '{}', got '{}'").attr("format")(
                                        py::str(name),
                                        py::str(std::string(needed_tcs)),
                                        py::str(m_buf->m_buf.format)
                                      ).cast<std::string>();
            py::warnings::warn(msg.c_str(), PyExc_BytesWarning, 3);
          }

          m_buf.reset(nullptr);
        }

        if (m_buf.get())
        {
          m_size = m_buf->m_buf.len / sizeof(T);
        }
        else
        {
          m_vec.reset(new std::vector<T>);
          for (auto it: obj)
            m_vec->push_back(py::cast<T>(*it));

          m_size = m_vec->size();
        }
      }

      T *get() const
      {
        if (m_buf.get())
          return reinterpret_cast<T *>(m_buf->m_buf.buf);
        else if (m_vec.get())
          return m_vec->empty() ? nullptr: m_vec->data();
        else
          return nullptr;
      }

      size_t size() const
      {
        return m_size;
      }

  };


  /**
   * This function verifies that the partitioning was computed correctly.
   */
  int
  wrap_verify_nd(const py::object &perm_py, const py::object &iperm_py)
  {
    int rcode=0;
    idx_t i;
    idx_t nvtxs = py::len(perm_py);

    array_from_py<idx_t> perm_ary("perm", perm_py, false);
    array_from_py<idx_t> iperm_ary("iperm", iperm_py, false);
    idx_t *perm = perm_ary.get();
    idx_t *iperm = iperm_ary.get();

    for (i=0; i<nvtxs; i++)
      if (i != perm[iperm[i]])
        rcode = 1;

    for (i=0; i<nvtxs; i++)
      if (i != iperm[perm[i]])
        rcode = 2;

    return rcode;
  }


  py::object
  wrap_node_nd(const py::object &xadj_py, const py::object &adjncy_py,
      const py::object &vwgt_py,
      metis_options &options)
  {
    array_from_py<idx_t> xadj("xadj", xadj_py, false);
    if (xadj.size() == 0)
      throw py::value_error("xadj cannot be empty");

    idx_t nvtxs = xadj.size() - 1;

    array_from_py<idx_t> adjncy("adjncy", adjncy_py, false);

    array_from_py<idx_t> vwgt("vwgt", vwgt_py, false, false);

    if (vwgt.size() != 0 && vwgt.size() != nvtxs)
      throw py::value_error("vwgt must be empty or have length nvtxs");

    array_for_py<idx_t> perm(nvtxs), iperm(nvtxs);

    int info = METIS_NodeND(
      &nvtxs, xadj.get(), adjncy.get(), vwgt.get(), options.m_options,
      perm.get(), iperm.get());

    assert_ok(info, "METIS_NodeND failed");

    return py::make_tuple(perm.as_array(), iperm.as_array());
  }

  py::object
  wrap_part_graph(
      idx_t nparts,
      const py::object &xadj_py,
      const py::object &adjncy_py,
      const py::object &vwgt_py,
      const py::object &adjwgt_py,
      const py::object &tpwgts_py,
      metis_options &options,
      bool recursive,
      bool warn_on_copies
    )
  {
    array_from_py<idx_t> xadj("xadj", xadj_py, warn_on_copies);
    if (xadj.size() == 0)
      throw py::value_error("xadj cannot be empty");

    idx_t nvtxs = xadj.size() - 1;

    array_from_py<idx_t> adjncy("adjncy", adjncy_py, warn_on_copies);
    array_from_py<idx_t> vwgt("vwgt", vwgt_py, warn_on_copies, false);
    array_from_py<idx_t> adjwgt("adjwgt", adjwgt_py, warn_on_copies, false);
    array_from_py<real_t> tpwgts("tpwgts", tpwgts_py, warn_on_copies, false);

    // partition weights
    idx_t ncon = 1;

    // pymetis defaults to the minimizing-edge-cut objective
    idx_t * pvsize = nullptr;

    real_t *pubvec = nullptr;

    idx_t edgecut;
    array_for_py<idx_t> part(nvtxs);

    if (recursive)
    {
      int info = METIS_PartGraphRecursive(
        &nvtxs, &ncon, xadj.get(), adjncy.get(),
        vwgt.get(), pvsize, adjwgt.get(), &nparts, tpwgts.get(),
        pubvec, options.m_options, &edgecut, part.get());

      assert_ok(info, "METIS_PartGraphRecursive failed");
    }
    else
    {
      int info = METIS_PartGraphKway(
        &nvtxs, &ncon, xadj.get(), adjncy.get(),
        vwgt.get(), pvsize, adjwgt.get(), &nparts, tpwgts.get(),
        pubvec, options.m_options, &edgecut, part.get());

      assert_ok(info, "METIS_PartGraphKway failed");
    }

    return py::make_tuple(edgecut, part.as_array());
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
    array_for_py<idx_t> elemPart(nElements), vertPart(nVertex);

    array_from_py<idx_t> connectivityOffsets("connectivityOffsets", connectivityOffsets_py, false);
    array_from_py<idx_t> connectivity("connectivity", connectivity_py, false);
    array_from_py<real_t> tpwgts("tpwgts", tpwgts_py, false);


    if(gtype == METIS_GTYPE_NODAL)
    {
        int info = METIS_PartMeshNodal(&nElements, &nVertex,
          connectivityOffsets.get(), connectivity.get(),
          nullptr, nullptr, &nParts, tpwgts.get(), options.m_options,
          &edgeCuts, elemPart.get(), vertPart.get());
        assert_ok(info, "METIS_PartMeshNodal failed");
    }
    else if(gtype == METIS_GTYPE_DUAL)
    {
        idx_t objval = 1;
        int info = METIS_PartMeshDual(&nElements, &nVertex,
          connectivityOffsets.get(), connectivity.get(),
          nullptr, nullptr, &ncommon, &nParts, tpwgts.get(), options.m_options,
          &objval, elemPart.get(), vertPart.get());
        assert_ok(info, "METIS_PartMeshNodal failed");
    }
    else {
        assert_ok(METIS_ERROR_INPUT, "Invalid value. "\
            "`gtype` is supposed to be either `METIS_GTYPE_NODAL`"\
            " or `METIS_GTYPE_DUAL`.");
    }

    return py::make_tuple(edgeCuts,
                          elemPart.as_array(),
                          vertPart.as_array()
                          );
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
  #pragma clang diagnostic push
  #pragma clang diagnostic ignored "-Wunused-parameter"
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
  #pragma clang diagnostic pop

  m.def("verify_nd", wrap_verify_nd);
  m.def("node_nd", wrap_node_nd);
  m.def("edge_nd", wrap_node_nd);  // DEPRECATED
  m.def("part_graph", wrap_part_graph,
        py::arg("nparts"),
        py::arg("xadj"),
        py::arg("adjncy"),
        py::arg("vwgt"),
        py::arg("adjwgt"),
        py::arg("tpwgts"),
        py::arg("options"),
        py::arg("recursive"),
        py::arg("warn_on_copies")=false
        );
  m.def("part_mesh", wrap_part_mesh);
  m.def("_idx_type_width", []() { return IDXTYPEWIDTH; });
}
