#!/usr/bin/env python

import sys
import setuptools
from setuptools.command.build_ext import build_ext


def get_config_schema():
    from aksetup_helper import ConfigSchema, StringListOption

    return ConfigSchema([
        StringListOption("CXXFLAGS", [],
            help="Any extra C++ compiler options to include"),
        ])


# {{{ boilerplate from https://github.com/pybind/python_example/blob/2ed5a68759cd6ff5d2e5992a91f08616ef457b5c/setup.py  # noqa

class get_pybind_include(object):  # noqa: N801
    """Helper class to determine the pybind11 include path

    The purpose of this class is to postpone importing pybind11
    until it is actually installed, so that the ``get_include()``
    method can be invoked. """

    def __init__(self, user=False):
        self.user = user

    def __str__(self):
        import pybind11
        return pybind11.get_include(self.user)


# As of Python 3.6, CCompiler has a `has_flag` method.
# cf http://bugs.python.org/issue26689
def has_flag(compiler, flagname):
    """Return a boolean indicating whether a flag name is supported on
    the specified compiler.
    """
    import tempfile
    with tempfile.NamedTemporaryFile('w', suffix='.cpp', delete=False) as f:
        f.write('int main (int argc, char **argv) { return 0; }')
        fname = f.name
    try:
        compiler.compile([fname], extra_postargs=[flagname])
    except setuptools.distutils.errors.CompileError:
        return False
    return True


def cpp_flag(compiler):
    """Return the -std=c++[11/14] compiler flag.

    The c++14 is prefered over c++11 (when it is available).
    """
    if has_flag(compiler, '-std=c++14'):
        return '-std=c++14'
    elif has_flag(compiler, '-std=c++11'):
        return '-std=c++11'
    else:
        raise RuntimeError('Unsupported compiler -- at least C++11 support '
                           'is needed!')


class BuildExt(build_ext):
    """A custom build extension for adding compiler-specific options."""
    c_opts = {
        'msvc': ['/EHsc'],
        'unix': [],
    }

    if sys.platform == 'darwin':
        c_opts['unix'] += ['-stdlib=libc++', '-mmacosx-version-min=10.7']

    def build_extensions(self):
        ct = self.compiler.compiler_type
        opts = self.c_opts.get(ct, [])
        if ct in ['unix', 'mingw32']:
            opts.append('-DVERSION_INFO="%s"' % self.distribution.get_version())
            opts.append(cpp_flag(self.compiler))
            if has_flag(self.compiler, '-fvisibility=hidden'):
                opts.append('-fvisibility=hidden')
        elif ct == 'msvc':
            opts.append('/DVERSION_INFO=\\"%s\\"' % self.distribution.get_version())
        for ext in self.extensions:
            ext.extra_compile_args = ext.extra_compile_args + opts
        build_ext.build_extensions(self)

# }}}


def main():
    import glob
    from setuptools import find_packages
    from aksetup_helper import (
            check_pybind11,
            hack_distutils, get_config, setup, Extension)

    check_pybind11()

    hack_distutils()
    conf = get_config(get_config_schema(),
            warn_about_no_config=False)

    EXTRA_DEFINES = {}

    EXTRA_DEFINES["HAVE_MREMAP"] = 0  # mremap() buggy on amd64?

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

          ext_modules=[
              Extension(
                  "pymetis._internal",
                  ["src/wrapper/wrapper.cpp"] +
                  glob.glob("src/metis/GKlib/*.c") +
                  glob.glob("src/metis/*.c") +
                  glob.glob("src/metis/libmetis/*.c"),
                  define_macros=list(EXTRA_DEFINES.items()),
                  include_dirs=["src/metis/GKlib"] +
                  ["src/metis/include"] +
                  ["src/metis/libmetis"] +
                  [
                        get_pybind_include(),
                        get_pybind_include(user=True)
                        ],
                  extra_compile_args=conf["CXXFLAGS"],
                  ),
              ],
            cmdclass={'build_ext': BuildExt},
            zip_safe=False)


if __name__ == '__main__':
    main()

# vim: foldmethod=marker
