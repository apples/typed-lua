Typed Lua
=========

Extremely WIP!

This will eventually be a statically typed superset of Lua.

Current roadmap:

1. ~~Implement a Lua 5.3 parser that can output the same or equivalent code.~~
2. ~~Add types and static type checking to the parser.~~
3. ~~Add interfaces and classes.~~ (moved classes to stretch goals).
4. ~~Add imported type checking.~~

Goals:

- Superset of Lua, existing Lua code should still be usable.
- Strict static type checking, including enforcing nil checks.
- Algebraic data types.

Stretch Goals:

- Classes with inheritance.
- Metamethod support.
- Function parameter type deduction.
- Compile directly to bytecode.

Not Goals:

- Runtime syntax extensions such as spread operators, object deconstruction, import/export, etc.
- Additional standard libraries.
- Additional primitive data types.
- Working outside of the PUC Lua or LuaJit runtimes.
- Bundling or minification
