luastrip
========

Strips Lua files of specific statements. The outputted Lua files are identically formatted to the input, including line numbers even after the skipped statements.

**Remove all calls to assert(...):**

`luastrip -fun assert -o stripped.lua input.lua`

**Remove all calls to Log.info(...), Log.warning(...) etc:**

`luastrip -fb Log -o stripped.lua input.lua`
