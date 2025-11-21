project(
    'pymetis',
    'c',
    'cpp',
    license: 'MIT',
    meson_version: '>=1.0.0',
    default_options: ['warning_level=2', 'cpp_std=c++17'],
)

metis_defs = [
    '-DIDXTYPEWIDTH=64',
    '-DREALTYPEWIDTH=64',
]

# {{{ gather includes

py = import('python').find_installation(pure: false)
pybind11_dep = dependency('pybind11')

# }}}

# {{{
if get_option('use-shipped-metis')
    gklib_srcs = [
        'src/metis/GKlib/src/b64.c',
        'src/metis/GKlib/src/blas.c',
        'src/metis/GKlib/src/csr.c',
        'src/metis/GKlib/src/error.c',
        'src/metis/GKlib/src/evaluate.c',
        'src/metis/GKlib/src/fkvkselect.c',
        'src/metis/GKlib/src/fs.c',
        'src/metis/GKlib/src/getopt.c',
        'src/metis/GKlib/src/gkregex.c',
        'src/metis/GKlib/src/graph.c',
        'src/metis/GKlib/src/htable.c',
        'src/metis/GKlib/src/io.c',
        'src/metis/GKlib/src/itemsets.c',
        'src/metis/GKlib/src/mcore.c',
        'src/metis/GKlib/src/memory.c',
        #'src/metis/GKlib/src/omp.c',
        #'src/metis/GKlib/src/pdb.c',
        'src/metis/GKlib/src/pqueue.c',
        'src/metis/GKlib/src/random.c',
        'src/metis/GKlib/src/rw.c',
        'src/metis/GKlib/src/seq.c',
        'src/metis/GKlib/src/sort.c',
        'src/metis/GKlib/src/string.c',
        'src/metis/GKlib/src/timers.c',
        'src/metis/GKlib/src/tokenizer.c',
        'src/metis/GKlib/src/gk_util.c',
    ]
    gklib_inc = include_directories('src/metis/GKlib/include')

    metis_srcs = [
        'src/metis/libmetis/auxapi.c',
        'src/metis/libmetis/balance.c',
        'src/metis/libmetis/bucketsort.c',
        'src/metis/libmetis/checkgraph.c',
        'src/metis/libmetis/coarsen.c',
        'src/metis/libmetis/compress.c',
        'src/metis/libmetis/contig.c',
        'src/metis/libmetis/debug.c',
        'src/metis/libmetis/fm.c',
        'src/metis/libmetis/fortran.c',
        'src/metis/libmetis/frename.c',
        'src/metis/libmetis/gklib.c',
        'src/metis/libmetis/graph.c',
        'src/metis/libmetis/initpart.c',
        'src/metis/libmetis/kmetis.c',
        'src/metis/libmetis/kwayfm.c',
        'src/metis/libmetis/kwayrefine.c',
        'src/metis/libmetis/mcutil.c',
        'src/metis/libmetis/mesh.c',
        'src/metis/libmetis/meshpart.c',
        'src/metis/libmetis/minconn.c',
        'src/metis/libmetis/mincover.c',
        'src/metis/libmetis/mmd.c',
        'src/metis/libmetis/ometis.c',
        'src/metis/libmetis/options.c',
        'src/metis/libmetis/parmetis.c',
        'src/metis/libmetis/pmetis.c',
        'src/metis/libmetis/refine.c',
        'src/metis/libmetis/separator.c',
        'src/metis/libmetis/sfm.c',
        'src/metis/libmetis/srefine.c',
        'src/metis/libmetis/stat.c',
        'src/metis/libmetis/timing.c',
        'src/metis/libmetis/util.c',
        'src/metis/libmetis/wspace.c',
    ]
    metis_inc = include_directories('src/metis/include', 'src/metis/libmetis')

    openmp_dep = dependency('openmp', required: false)
    metis_lib = static_library(
        'metis_internal_static',
        gklib_srcs + metis_srcs,
        include_directories: [gklib_inc, metis_inc],
        c_args: [metis_defs],
        cpp_args: [metis_defs],
        dependencies: [openmp_dep],
        pic: true,
    )

    metis_dep = declare_dependency(
        include_directories: [gklib_inc, metis_inc],
        link_with: [metis_lib],
    )
else
    cc = meson.get_compiler('c')
    metis_dep = dependency('metis', required: false)

    if not metis_dep.found()
        metis_dep = cc.find_library(
            'metis',
            has_headers: ['metis.h'],
            required: false,
        )
    endif

    if not metis_dep.found()
        error(
            'Dependency "metis" not found, tried pkgconfig, cmake and find_library',
        )
    endif
endif

# }}}

# {{{ extension

py.extension_module(
    '_internal',
    ['src/wrapper/wrapper.cpp'],
    dependencies: [pybind11_dep, metis_dep],
    c_args: [metis_defs],
    cpp_args: [metis_defs],
    subdir: 'pymetis',
    install: true,
)

py.install_sources(
    [
        'pymetis/__init__.py',
        'pymetis/version.py',
        'pymetis/_internal.pyi',
        'pymetis/py.typed',
    ],
    subdir: 'pymetis',
)

# }}}
