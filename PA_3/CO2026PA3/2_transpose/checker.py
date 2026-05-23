import re


# Blocked C data type keywords
BLOCKED_TYPES = [
    "int", "char", "short", "long", "float", "double",
    "signed", "unsigned", "struct", "union", "enum", "void",
    "_Bool",
    "size_t", "ssize_t", "ptrdiff_t",
    "int8_t", "int16_t", "int32_t", "int64_t",
    "uint8_t", "uint16_t", "uint32_t", "uint64_t",
    "intptr_t", "uintptr_t",
    "intmax_t", "uintmax_t",
]

BLOCKED_PATTERN = re.compile(
    r"\b(" + "|".join(map(re.escape, BLOCKED_TYPES)) + r")\b"
)
    

def strip_comments_and_strings(code: str) -> str:
    """
    Remove C/C++ comments and string/char literals before keyword checking.
    """
    pattern = r"""
        //.*?$                         | # line comments
        /\*.*?\*/                      | # block comments
    """
    return re.sub(pattern, "", code, flags=re.MULTILINE | re.DOTALL | re.VERBOSE)


def find_blocked_types(code: str) -> list[tuple[str, int]]:
    """
    Return a list of (keyword, position) for blocked type keywords found.
    """
    clean_code = strip_comments_and_strings(code)
    matches = []

    for match in BLOCKED_PATTERN.finditer(clean_code):
        matches.append((match.group(1), match.start()))

    return matches


def main() -> None:
    snippet = "snippet.c"
    try:
        with open(snippet, "r", encoding="utf-8") as f:
            code = f.read()
    except Exception as e:
        print(f"[CE] {e}")
        return 0

    print("[PASS] snippet loaded")
    matches = find_blocked_types(code)
    if matches:
        for keyword, pos in matches:
            print(f"  {keyword} at offset {pos}")
        return 0


if __name__ == "__main__":
    main()