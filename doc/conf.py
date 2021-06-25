from urllib.request import urlopen

_conf_url = \
        "https://raw.githubusercontent.com/inducer/sphinxconfig/main/sphinxconfig.py"
with urlopen(_conf_url) as _inf:
    exec(compile(_inf.read(), _conf_url, "exec"), globals())

copyright = "2013-2021, Andreas Kloeckner"
author = "Andreas Kloeckner"


def get_version():
    conf = {}
    exec(
        compile(
            open("../pymetis/__init__.py").read(), "../pymetis/__init__.py", "exec"
        ),
        conf,
    )
    return conf["version"]


version = get_version()
