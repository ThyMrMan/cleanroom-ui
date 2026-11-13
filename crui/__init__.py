"""Cleanroom UI's build: the menu movies (mxmlc), the Papyrus scripts and the F4SE DLL, into a mod folder.

    python -m crui all --out dist/CleanroomUI
"""

import subprocess
import sys
from pathlib import Path

#: The repo's root folder.
REPO = Path(__file__).resolve().parents[1]
#: Spread into every subprocess call: no console window per tool on Windows.
POPEN_FLAGS = {'creationflags': subprocess.CREATE_NO_WINDOW} if sys.platform == 'win32' else {}
#: Where built movies are kept by their sources' hash between builds.
CACHE = REPO / 'out' / 'cache'
