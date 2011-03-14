#!/usr/bin/env python

import sys

def get_config_schema():
    from aksetup_helper import ConfigSchema, Option, \
            IncludeDir, LibraryDir, Libraries, BoostLibraries, \
            Switch, StringListOption, make_boost_base_options

    return ConfigSchema(make_boost_base_options() + [
        BoostLibraries("python"),

        StringListOption("CXXFLAGS", [], 
            help="Any extra C++ compiler options to include"),
        ])




def main():
    import glob
    from aksetup_helper import hack_distutils, get_config, setup, Extension

    hack_distutils()
    conf = get_config(get_config_schema())

    INCLUDE_DIRS = conf["BOOST_INC_DIR"]
    LIBRARY_DIRS = conf["BOOST_LIB_DIR"]
    LIBRARIES = conf["BOOST_PYTHON_LIBNAME"]
    EXTRA_DEFINES = {"HAVE_MREMAP": 0} # mremap() buggy on amd64?

    execfile("pymetis/__init__.py", conf)
    setup(name="PyMetis",
          version="2011.1",
          description="A Graph Partitioning Package",
          long_description="""
          PyMetis is a Python wrapper for the
          `Metis <http://glaros.dtc.umn.edu/gkhome/views/metis>`_ graph
          partititioning software by George Karypis, Vipin Kumar and others. It
          includes version 5.0pre2 of Metis and wraps it using the 
          `Boost Python <http://www.boost.org/libs/python/doc/>`_ wrapper generator
          library. So far, it only wraps the most basic graph partitioning
          functionality (which is enough for my current use), but extending it
          in case you need more should be quite straightforward. Using PyMetis
          to partition your meshes is really easy--essentially all you need to
          pass into PyMetis is an adjacency list for the graph and the number
          of parts you would like.
          """,
          author="Andreas Kloeckner",
          author_email="inform@tiker.net",
          license = "wrapper: MIT/code: Free for research and non-commercial use",
          url="http://mathema.tician.de/software/pymetis",
          classifiers=[
            'Development Status :: 4 - Beta',
            'Intended Audience :: Developers',
            'Intended Audience :: Other Audience',
            'Intended Audience :: Science/Research',
            'License :: OSI Approved :: MIT License',
            'License :: Free for non-commercial use',
            'Natural Language :: English',
            'Programming Language :: C',
            'Programming Language :: C++',
            'Programming Language :: Python',
            'Topic :: Multimedia :: Graphics :: 3D Modeling',
            'Topic :: Scientific/Engineering',
            'Topic :: Scientific/Engineering :: Mathematics',
            'Topic :: Scientific/Engineering :: Visualization',
            'Topic :: Software Development :: Libraries',
            ],

          packages = [ "pymetis" ],
          ext_modules = [
            Extension(
              "pymetis._internal", 
              glob.glob("src/gklib/*.c") 
              + glob.glob("src/metis/*.c") 
              + ["src/wrapper/wrapper.cpp"],
              define_macros=list(EXTRA_DEFINES.iteritems()),
              include_dirs=["src/gklib", "src/metis"] + INCLUDE_DIRS,
              library_dirs=LIBRARY_DIRS,
              libraries=LIBRARIES,
              extra_compile_args=conf["CXXFLAGS"],
              ),
            ]
         )




if __name__ == '__main__':
    main()
