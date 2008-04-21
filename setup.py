#!/usr/bin/env python

import sys

def get_config_schema():
    from aksetup_helper import ConfigSchema, Option, \
            IncludeDir, LibraryDir, Libraries, \
            Switch, StringListOption

    return ConfigSchema([
        IncludeDir("BOOST", []),
        LibraryDir("BOOST", []),
        Libraries("BOOST_PYTHON", ["boost_python-gcc42-mt"]),

        StringListOption("CXXFLAGS", [], 
            help="Any extra C++ compiler options to include"),
        ])




def main():
    import glob
    from aksetup_helper import hack_distutils, get_config, setup, Extension

    hack_distutils()
    conf = get_config()

    INCLUDE_DIRS = conf["BOOST_INC_DIR"]
    LIBRARY_DIRS = conf["BOOST_LIB_DIR"]
    LIBRARIES = conf["BOOST_PYTHON_LIBNAME"]
    EXTRA_DEFINES = {"HAVE_MREMAP": 0} # mremap() buggy on amd64?

    execfile("src/python/__init__.py", conf)
    setup(name="PyMetis",
          version="0.91",
          description="A wrapper around Metis",
          author="Andreas Kloeckner",
          author_email="inform@tiker.net",
          license = "BSD for the wrapper/",
          url="http:///software/pymetis",
          packages = [ "pymetis" ],
          package_dir={"pymetis": "src/python"},
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
