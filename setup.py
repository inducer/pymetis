#!/usr/bin/env python


def get_config_schema():
    from aksetup_helper import (ConfigSchema, StringListOption, Switch,
            IncludeDir, LibraryDir, Libraries)

    return ConfigSchema([
        Switch("USE_SHIPPED_METIS", True, "Use included copy of metis"),
        StringListOption("CXXFLAGS", [],
            help="Any extra C++ compiler options to include"),

        IncludeDir("METIS", []),
        LibraryDir("METIS", []),
        Libraries("METIS", ["metis"]),
        ])


def main():
    import glob
    from setuptools import find_packages
    from aksetup_helper import (
            check_pybind11,
            hack_distutils, get_config, setup, Extension,
            get_pybind_include, PybindBuildExtCommand)
    from setuptools.command.build_clib import build_clib

    check_pybind11()

    hack_distutils()
    conf = get_config(get_config_schema(),
            warn_about_no_config=False)

    extra_defines = {}

    if conf["USE_SHIPPED_METIS"]:
        extra_defines["HAVE_MREMAP"] = 0  # mremap() buggy on amd64?
        metis_source_files = (
                      glob.glob("src/metis/GKlib/*.c")
                      + glob.glob("src/metis/*.c")
                      + glob.glob("src/metis/libmetis/*.c")
                      )
        metis_include_dirs = [
                      "src/metis/include",
                      "src/metis/GKlib",
                      "src/metis/include",
                      "src/metis/libmetis",
                      ]
        metis_library_dirs = []
        metis_libraries = []
    else:
        metis_source_files = []
        metis_include_dirs = conf["METIS_INC_DIR"]
        metis_library_dirs = conf["METIS_LIB_DIR"]
        metis_libraries = conf["METIS_LIBNAME"]

    ver_filename = "pymetis/version.py"
    version_file = open(ver_filename)
    ver_dic = {}
    try:
        version_file_contents = version_file.read()
    finally:
        version_file.close()
    exec(compile(version_file_contents, ver_filename, 'exec'), ver_dic)

    setup(name="PyMetis",
          version=ver_dic["version"],
          description="A Graph Partitioning Package",
          long_description=open("README.rst", "rt").read(),
          author="Andreas Kloeckner",
          author_email="inform@tiker.net",
          license="wrapper: MIT/METIS: Apache 2",
          url="http://mathema.tician.de/software/pymetis",
          classifiers=[
              'Development Status :: 4 - Beta',
              'Intended Audience :: Developers',
              'Intended Audience :: Other Audience',
              'Intended Audience :: Science/Research',
              'License :: OSI Approved :: MIT License',
              'License :: OSI Approved :: Apache Software License',
              'Natural Language :: English',
              'Programming Language :: C',
              'Programming Language :: C++',
              'Programming Language :: Python',
              'Programming Language :: Python :: 3',
              'Topic :: Multimedia :: Graphics :: 3D Modeling',
              'Topic :: Scientific/Engineering',
              'Topic :: Scientific/Engineering :: Mathematics',
              'Topic :: Scientific/Engineering :: Visualization',
              'Topic :: Software Development :: Libraries',
              ],

          packages=find_packages(),

          setup_requires=[
              "pybind11",
               ],

          python_requires="~=3.6",
          install_requires=["six"],

          ext_modules=[
              Extension(
                  "pymetis._internal",
                  ["src/wrapper/wrapper.cpp"] + metis_source_files,
                  define_macros=list(extra_defines.items()),
                  include_dirs=metis_include_dirs + [
                      get_pybind_include(),
                      get_pybind_include(user=True)
                      ],
                  library_dirs=metis_library_dirs,
                  libraries=metis_libraries,
                  extra_compile_args=conf["CXXFLAGS"],
                  ),
              ],

           cmdclass={
               'build_clib': build_clib,
               'build_ext': PybindBuildExtCommand
               },

           zip_safe=False)


if __name__ == '__main__':
    main()

# vim: foldmethod=marker
