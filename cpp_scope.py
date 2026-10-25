"""Namespace-scope declarations of C++ source, for tools that bind names across units.

statements() walks a translation unit's tokens and yields each statement at
namespace scope with the namespaces around it and its linkage. It skips
comments, string and character literals, and preprocessor lines, and folds the
bodies of functions, classes and braced initializers. variables() then reads a
statement's single variable declarator, if it has one. This is not a C++ parser:
statements it does not understand yield no variable, so a caller that needs one
gets nothing rather than a guess.
"""
from __future__ import annotations

import re
from collections.abc import Iterator
from dataclasses import dataclass

TOKEN = re.compile(r"""
    (?P<space>\s+)
  | (?P<comment>//[^\n]*|/\*.*?\*/)
  | (?P<literal>"(?:\\.|[^"\\\n])*"|'(?:\\.|[^'\\\n])*')
  | (?P<word>[A-Za-z_]\w*)
  | (?P<number>\.?\d(?:[\w.']|[eEpP][+-])*)
  | (?P<punct>::|.)
""", re.VERBOSE | re.DOTALL)
DIRECTIVE = re.compile(r"^[ \t]*#(?:\\\n|[^\n])*", re.MULTILINE)      # with its continuation lines
SPECIFIERS = {"extern", "static", "inline", "thread_local", "register", "mutable", "__declspec"}
SKIPPED = {"typedef", "using", "template", "static_assert", "friend", "namespace", "class", "struct", "union",
           "enum", "operator", "return"}


@dataclass(frozen=True)
class Token:
    text: str
    start: int
    end: int


@dataclass(frozen=True)
class Statement:
    tokens: tuple[Token, ...]         # bodies folded to a single "{}" token
    namespace: tuple[str, ...]        # enclosing named namespaces, outermost first
    c_linkage: bool                   # inside extern "C" { } or declared extern "C"
    anonymous: bool                   # inside an anonymous namespace
    start: int                        # character span in the source, terminator included
    end: int
    text: str                         # the source of that span


@dataclass(frozen=True)
class Variable:
    name: str
    type: str                         # declared type, without storage specifiers or array bounds
    bounds: str                       # array bounds as written, e.g. "[3]", or ""
    defined: bool                     # a definition, not only an extern declaration
    external: bool                    # external linkage
    reference: bool                   # declared as a reference: an alias, not storage
    initializer: str                  # the initializer's source, braces included, or ""
    statement: Statement


@dataclass(frozen=True)
class Function:
    name: str
    returns: str                      # the return type, without storage specifiers or calling convention
    convention: str                   # "__fastcall", "__cdecl", ... or "" for the default
    params: tuple[str, ...]           # parameter declarations as written, () for none or (void)
    defined: bool                     # has a body
    external: bool                    # external linkage
    statement: Statement


def tokens(text: str) -> list[Token]:
    """Tokens with their offsets in text; preprocessor lines are blanked first, so offsets hold."""
    text = DIRECTIVE.sub(lambda m: re.sub(r"[^\n]", " ", m.group()), text)
    return [Token(m.group(), m.start(), m.end()) for m in TOKEN.finditer(text)
            if m.lastgroup not in {"space", "comment"}]


def closing(items: list[Token], i: int) -> int:
    """Index of the brace closing the one at i."""
    depth = 0
    for j in range(i, len(items)):
        depth += (items[j].text == "{") - (items[j].text == "}")
        if depth == 0:
            return j
    raise ValueError("Unbalanced braces")


def statements(text: str) -> Iterator[Statement]:
    items = tokens(text)
    scopes: list[tuple[str, str | None]] = []           # ("namespace", name or None) or ("extern", None)
    current: list[Token] = []
    i = 0
    while i < len(items):
        t = items[i].text
        if not current and t == "namespace":
            j = i + 1
            names = []
            while j < len(items) and items[j].text not in {"{", "=", ";"}:
                if items[j].text not in {"::", "inline"}:
                    names.append(items[j].text)
                j += 1
            if j < len(items) and items[j].text == "{":
                scopes.extend(("namespace", n) for n in names or [None])
                scopes.append(("group", str(len(names or [None]))))
                i = j + 1
                continue
        if not current and t == "extern" and i + 2 < len(items) and items[i + 1].text in {'"C"', '"C++"'} \
                and items[i + 2].text == "{":
            scopes.append(("extern", items[i + 1].text))
            i += 3
            continue
        if t == "}" and not current:
            if scopes and scopes[-1][0] == "group":
                for _ in range(int(scopes.pop()[1])):
                    scopes.pop()
            elif scopes:
                scopes.pop()
            i += 1
            continue
        if t == "{":
            end = closing(items, i)
            current.append(Token("{}", items[i].start, items[end].end))
            i = end + 1
            words = [x.text for x in current]
            if "(" in words and "=" not in words and not set(words) & {"class", "struct", "union", "enum"}:
                yield statement(current, scopes, text)              # a function body ends its statement
                current = []
            continue
        if t == ";":
            current.append(items[i])
            yield statement(current, scopes, text)
            current = []
            i += 1
            continue
        current.append(items[i])
        i += 1


def statement(current: list[Token], scopes: list[tuple[str, str | None]], text: str) -> Statement:
    namespaces = [name for kind, name in scopes if kind == "namespace"]
    words = [t.text for t in current]
    c_linkage = ("extern", '"C"') in scopes or words[:2] == ["extern", '"C"']
    start, end = current[0].start, current[-1].end
    return Statement(tuple(current), tuple(n for n in namespaces if n), c_linkage, None in namespaces,
                     start, end, text[start:end])


def top_level_const(type_words: list[str]) -> bool:
    """Whether the declared object itself is const: 'const int', 'int const', 'T* const', not 'const T*'."""
    if "constexpr" in type_words or type_words[-1:] == ["const"]:
        return True
    return "const" in type_words and "*" not in type_words[type_words.index("const"):]


def top_level_comma(words: list[str]) -> bool:
    """A comma outside parentheses and brackets (braces are already folded): a list of declarators."""
    depth = 0
    for w in words:
        depth += (w in {"(", "["}) - (w in {")", "]"})
        if w == "," and depth == 0:
            return True
    return False


def variables(s: Statement) -> Variable | None:
    """The statement's variable, when it declares exactly one plainly typed variable."""
    words = [t.text for t in s.tokens]
    if not words or words[0] in SKIPPED or words[-1] not in {";", "{}"} or "operator" in words:
        return None
    body = words[:-1] if words[-1] == ";" else words
    if top_level_comma(body):
        return None                     # several declarators
    if "=" in body:
        declarator, initializer = body[:body.index("=")], s.tokens[body.index("=") + 1:len(body)]
    elif body and body[-1] == "{}":
        declarator, initializer = body[:-1], s.tokens[len(body) - 1:len(body)]
    else:
        declarator, initializer = body, ()
    initialized = bool(initializer)
    if any(w in {",", "(", ")", "{}", "<", ">", "...", "->"} for w in declarator) or len(declarator) < 2:
        return None                     # several declarators, functions, templates: not a plain variable
    bounds = []
    while declarator[-1] == "]":
        start = len(declarator) - 1 - declarator[::-1].index("[")
        bounds.insert(0, "".join(declarator[start:]))
        declarator = declarator[:start]
    name = declarator[-1]
    if not re.fullmatch(r"[A-Za-z_]\w*", name):
        return None
    prefix = declarator[:-1]
    if prefix[:2] == ["extern", '"C"']:
        prefix = ["extern", *prefix[2:]]
    extern = "extern" in prefix
    type_words = [w for w in prefix if w not in SPECIFIERS and w != '"C"']
    if not type_words:
        return None
    internal = "static" in prefix or s.anonymous or (top_level_const(type_words) and not extern and not s.c_linkage)
    source = s.text[initializer[0].start - s.start:initializer[-1].end - s.start] if initializer else ""
    return Variable(name, spell(type_words), "".join(bounds), initialized or not extern, not internal,
                    "&" in type_words, source, s)


CONVENTIONS = {"__fastcall", "__cdecl", "__stdcall", "__thiscall", "__vectorcall"}


def functions(s: Statement) -> Function | None:
    """The statement's function, when it declares or defines exactly one plain (non-template, non-member,
    non-operator) function: 'extern "C" int __fastcall f(void* self, void*) noexcept(false) { ... }'."""
    items = without_declspecs(s.tokens)
    words = [t.text for t in items]
    if not words or words[0] in SKIPPED or words[-1] not in {";", "{}"} or "operator" in words or "(" not in words:
        return None
    opening = words.index("(")
    closing_ = opening + matching_paren(words[opening:])
    if closing_ < opening or opening < 2 or not re.fullmatch(r"[A-Za-z_]\w*", words[opening - 1]):
        return None
    prefix, tail = words[:opening - 1], words[closing_ + 1:]
    if prefix[:2] == ["extern", '"C"']:
        prefix = ["extern", *prefix[2:]]
    if prefix[-1:] == ["::"] or any(w in {"=", ",", "(", ")", "{}"} for w in prefix):
        return None                     # members defined out of class, initializers, pointers to functions
    if any(w not in {"noexcept", "(", ")", "false", "true", "throw", ";", "{}"} for w in tail) or "{}" in tail[:-1]:
        return None                     # trailing return types, attributes, initializer lists
    convention = [w for w in prefix if w in CONVENTIONS]
    type_words = [w for w in prefix if w not in SPECIFIERS and w not in CONVENTIONS]
    if len(convention) > 1 or not type_words:
        return None
    inner = words[opening + 1:closing_]
    params = () if inner in ([], ["void"]) else tuple(spell(group) for group in split_top(inner))
    return Function(words[opening - 1], spell(type_words), "".join(convention), params, words[-1] == "{}",
                    "static" not in prefix and not s.anonymous, s)


def without_declspecs(items: tuple[Token, ...]) -> list[Token]:
    """items without '__declspec( ... )' groups."""
    out, i = [], 0
    while i < len(items):
        if items[i].text == "__declspec" and i + 1 < len(items) and items[i + 1].text == "(":
            i += 1 + matching_paren([t.text for t in items[i + 1:]]) + 1
            continue
        out.append(items[i])
        i += 1
    return out


def matching_paren(words: list[str]) -> int:
    """Index of the parenthesis closing words[0], or -1."""
    depth = 0
    for j, w in enumerate(words):
        depth += (w == "(") - (w == ")")
        if depth == 0:
            return j
    return -1


def split_top(words: list[str]) -> list[list[str]]:
    """Parameter words grouped at top-level commas (template arguments included)."""
    groups: list[list[str]] = [[]]
    depth = 0
    for w in words:
        depth += (w in {"(", "[", "<"}) - (w in {")", "]", ">"})
        if w == "," and depth == 0:
            groups.append([])
        else:
            groups[-1].append(w)
    return groups


def spell(words: list[str]) -> str:
    """Tokens back to a type: 'std :: uint32_t volatile' -> 'std::uint32_t volatile'."""
    text = " ".join(words)
    return re.sub(r"\s*::\s*", "::", re.sub(r"\s+([*&])", r"\1", text))
