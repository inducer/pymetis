from importlib import metadata
from urllib.request import urlopen


_conf_url = \
        "https://raw.githubusercontent.com/inducer/sphinxconfig/main/sphinxconfig.py"
with urlopen(_conf_url) as _inf:
    exec(compile(_inf.read(), _conf_url, "exec"), globals())

copyright = "2013-2024, Andreas Kloeckner"
author = "Andreas Kloeckner"
release = metadata.version("pymetis")
version = ".".join(release.split(".")[:2])
