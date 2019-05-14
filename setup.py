#!/usr/bin/env python


def get_config_schema():
    from aksetup_helper import ConfigSchema, StringListOption

    return ConfigSchema([
        StringListOption("CXXFLAGS", [],
            help="Any extra C++ compiler options to include"),
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

    extra_defines["HAVE_MREMAP"] = 0  # mremap() buggy on amd64?

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
              'License :: Free for non-commercial use',
              'Natural Language :: English',
              'Programming Language :: C',
              'Programming Language :: C++',
              'Programming Language :: Python',
              'Programming Language :: Python :: 2',
              'Programming Language :: Python :: 2.6',
              'Programming Language :: Python :: 2.7',
              'Programming Language :: Python :: 3',
              'Programming Language :: Python :: 3.3',
              'Programming Language :: Python :: 3.4',
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

          install_requires=["six"],

          libraries=[
              (
                  "metis",
                  {
                      "sources": (
                           glob.glob("src/metis/GKlib/*.c")
                           + glob.glob("src/metis/*.c")
                           + glob.glob("src/metis/libmetis/*.c")
                           ),
                      "include_dirs": [
                          "src/metis/GKlib",
                          "src/metis/include",
                          "src/metis/libmetis"
                          ],
                  }
              )],

          ext_modules=[
              Extension(
                  "pymetis._internal",
                  ["src/wrapper/wrapper.cpp"],
                  define_macros=list(extra_defines.items()),
                  include_dirs=["src/metis/include"]
                  + [
                        get_pybind_include(),
                        get_pybind_include(user=True)
                        ],
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
