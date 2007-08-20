#!/usr/bin/env python

import sys
import glob

try:
    execfile("siteconf.py")
except IOError:
    print "*** Please run configure first."
    sys.exit(1)

from distutils.core import setup,Extension

def non_matching_config():
    print "*** The version of your configuration template does not match"
    print "*** the version of the setup script. Please re-run configure."
    sys.exit(1)

try:
    PYMETIS_CONF_TEMPLATE_VERSION
except NameError:
    non_matching_config()

if PYMETIS_CONF_TEMPLATE_VERSION != 1:
    non_matching_config()

INCLUDE_DIRS = BOOST_INCLUDE_DIRS
LIBRARY_DIRS = BOOST_LIBRARY_DIRS
LIBRARIES = BPL_LIBRARIES

execfile("src/python/__init__.py")
setup(name="PyMetis",
      version=version,
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
          include_dirs=["src/gklib", "src/metis"] + INCLUDE_DIRS,
          library_dirs=LIBRARY_DIRS,
          libraries=LIBRARIES,
          ),
        ]
     )
