from pathlib import Path

path = Path("source/main.cpp")
text = path.read_text()
old = "#include <cstdint>\n#include <cstdio>\n"
new = "#include <cstdint>\n#include <cstdio>\n#include <optional>\n"
if old not in text:
    raise RuntimeError("main include anchor missing")
path.write_text(text.replace(old, new, 1))
