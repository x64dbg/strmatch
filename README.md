# strmatch

Simple string matching plugin for x64dbg. Supports UTF8, UTF16 and Local codepages.

```
[strmatch] documentation:
  1. Command: strmatch_set index, "string"
  2. Expression function: [action]_[encoding](va, index)
  Actions: strcmp, stricmp, strstr, stristr
  Encodings: utf8, utf16, local
  Example: strcmp_utf16(va, index)
```

# Example

```
strmatch_set 1337, "hello"
stristr_utf8(edx, 1337)
```
