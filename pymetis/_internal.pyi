from enum import IntEnum, auto

class GType(IntEnum):
    NODAL = auto()
    DUAL = auto()

class Options:
    def __init__(self) -> None: ...
    def _get(self, idx: int, /) -> int: ...
    def _set(self, idx: int, value: int, /) -> None: ...

def _idx_type_width() -> int: ...
