from importlib import metadata


def _parse_version(version: str) -> tuple[tuple[int, ...], str]:
    import re

    m = re.match(r"^([0-9.]+)([a-z0-9]*?)$", VERSION_TEXT)
    assert m is not None

    return tuple(int(nr) for nr in m.group(1).split(".")), m.group(2)


VERSION_TEXT = metadata.version("pymetis")
VERSION, VERSION_STATUS = _parse_version(VERSION_TEXT)

# deprecated
version = VERSION_TEXT
version_tuple = VERSION
