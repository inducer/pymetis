#!/usr/bin/env python

import sys
import glob

def main():
    try:
        conf = {}
        execfile("siteconf.py", conf)
    except IOError:
        print "*** Please run configure first."
        sys.exit(1)

    from distutils.core import setup,Extension

    def non_matching_config():
        print "*** The version of your configuration template does not match"
        print "*** the version of the setup script. Please re-run configure."
        sys.exit(1)

    if "PYMETIS_CONF_TEMPLATE_VERSION" not in conf:
        non_matching_config()

    if conf["PYMETIS_CONF_TEMPLATE_VERSION"] != 1:
        non_matching_config()

    INCLUDE_DIRS = conf["BOOST_INCLUDE_DIRS"]
    LIBRARY_DIRS = conf["BOOST_LIBRARY_DIRS"]
    LIBRARIES = conf["BPL_LIBRARIES"]
    EXTRA_DEFINES = {"HAVE_MREMAP": 0} # mremap() buggy on amd64?

    execfile("src/python/__init__.py", conf)
    setup(name="PyMetis",
          version=conf["version"],
          description="A wrapper around Metis",
          author="Andreas Kloeckner",
          author_email="inform@tiker.net",
          license = "BSD for the wrapper/",
          url="http://news.tiker.net/software/pymetis",
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
              extra_compile_args=conf["EXTRA_COMPILE_ARGS"],
              ),
            ]
         )




if __name__ == '__main__':
    # hack distutils.sysconfig to eliminate debug flags
    # stolen from mpi4py
    import sys
    if not sys.platform.lower().startswith("win"):
        from distutils import sysconfig

        cvars = sysconfig.get_config_vars()
        cflags = cvars.get('OPT')
        if cflags:
            cflags = cflags.split()
            for bad_prefix in ('-g', '-O', '-Wstrict-prototypes'):
                for i, flag in enumerate(cflags):
                    if flag.startswith(bad_prefix):
                        cflags.pop(i)
                        break
                if flag in cflags:
                    cflags.remove(flag)
            cflags.append("-g")
            cvars['OPT'] = str.join(' ', cflags)
            cvars["CFLAGS"] = cvars["BASECFLAGS"] + " " + cvars["OPT"]
    # and now call main
    main()
