# RocketGE Coding Style Guide

This document includes our basic coding style guides for RocketGE to make code easier to read and maintain.

### 1. Namespaces
All engine code must be inside the `rocket` namespace. \
The legacy `util` namespace is an exception and new functions may not be added or removed without explicit approval.

### 2. Names
Use names that are meaningful and readable. \
Names of classes, functions and variables should be `snake_case` in general. \
Member Variables also follow the same pattern, no extra prefix or suffix \
**NOTE:** Global variables should be prefixed with `g_`

#### 2.1 Class/Interface/Enum Names
Class names should be in `snake_case` and end with `_t` if concrete. \
\
Example:
```cpp
class frame_timer_t { ... };
```

If a class is an interface and cannot be instantiated (eg. PureVirtual Class) it must end with `_i` \
\
Example:
```cpp
class renderer_2d_i { ... };
```

Enum names should follow `_t` suffixing. \
**NOTE:** Enums should always be of `enum class` not C-style enums.

### 2.2 Function Names

Use prefixes when they are useful: \
-> get / set - functions that are used to get / set a value \
-> is - functions that return a boolean and ask a question \
\
Examples:
```cpp
void set_size();
int get_size();

bool is_active();
```


### 2.3. File Names
File names should be in snake case. \
Use the suffix `.hpp` for header files and `.cpp` for source files. \
Use `#pragma once` so you don't waste your time.

### 3. Formatting
#### 3.1 Curly Braces
Use Curly Braces in general. \
Place curly braces in the same line. \
\
Examples:

```cpp
if (condition) {
    // do something
    // do more
}

for (int i = 0; i < 5; i++) {
    // do something
    // do more
}

void some_function() {
    // do something
}
```
#### 3.2 if / else if
```cpp
if (c1) {
    // ...
} else if (c2) {
    // ...
} else {
    // ...
}
```
Curly Braces are optional for a single statement. \
\
Example:

```cpp
if (condition) {
    // do something
}
```
<br/>

If anywhere in the codebase you see code not conforming to this style guide you may submit a PR to fix it.
