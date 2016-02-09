#!/usr/bin/env python


def get_config_schema():
    from aksetup_helper import (ConfigSchema, Switch,
            BoostLibraries, StringListOption, make_boost_base_options)

    return ConfigSchema(make_boost_base_options() + [
        Switch("USE_SHIPPED_BOOST", True, "Use included Boost library"),

        BoostLibraries("python"),

        StringListOption("CXXFLAGS", [],
            help="Any extra C++ compiler options to include"),
        ])


def main():
    import glob
    from aksetup_helper import (
            hack_distutils, get_config, setup, Extension,
            set_up_shipped_boost_if_requested)

    hack_distutils()
    conf = get_config(get_config_schema(),
            warn_about_no_config=False)

    EXTRA_OBJECTS, EXTRA_DEFINES = \
            set_up_shipped_boost_if_requested("pymetis", conf)

    INCLUDE_DIRS = conf["BOOST_INC_DIR"]
    LIBRARY_DIRS = conf["BOOST_LIB_DIR"]
    LIBRARIES = conf["BOOST_PYTHON_LIBNAME"]

    EXTRA_DEFINES["HAVE_MREMAP"] = 0  # mremap() buggy on amd64?

    version_file = open("pymetis/__init__.py")
    ver_dic = {}
    try:
        version_file_contents = version_file.read()
    finally:
        version_file.close()
    exec(compile(version_file_contents, "pymetis/__init__.py", 'exec'), ver_dic)

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

          packages=["pymetis"],
          install_requires=["six"],
          ext_modules=[
              Extension(
                  "pymetis._internal",
                  glob.glob("src/metis/GKlib/*.c") +
                  glob.glob("src/metis/*.c") +
                  glob.glob("src/metis/libmetis/*.c") +
                  ["src/wrapper/wrapper.cpp"] + EXTRA_OBJECTS,
                  define_macros=list(EXTRA_DEFINES.items()),
                  include_dirs=["src/metis/GKlib"] +
                  ["src/metis/include"] +
                  ["src/metis/libmetis"] +
                  INCLUDE_DIRS,
                  library_dirs=LIBRARY_DIRS,
                  libraries=LIBRARIES,
                  extra_compile_args=conf["CXXFLAGS"],
                  ),
              ]
          )


if __name__ == '__main__':
    main()
