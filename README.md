# C Shadow Data

This is my implementation of dynamic arrays in c using a hidden header when allocating the dynamic arrays.

The implementation provides you the dynamic and static allocation of the array while simultaneously allowing you to keep the direct access using the **[ ]** operator.
Like in Java, you are also given the tools to easily stream arrays through different types of functions and chain them one after another without leaking memory.

those stream functions can vary between Map, Filter, Distinct, Sort, Max/Min, ForEach and many more other functions.

In addition you are provided with some safety measures to the dynamic arrays to increase its security, like the header canary and the validation function, and many safety checks in each function.

Shoutout to the youtuber Tsoding and his video on shadow data which inspired me to make this project.

His channel:<a href='https://www.youtube.com/@Tsoding'>Tsoding</a>

## Installation

`dyn_array.h` is a single-header library. 

1. Download `dyn_array.h` from the `release/` folder.
2. Drop it into your project folder.
3. In **exactly one** `.c` file, define the implementation macro before including the header:

    ```c
    #define DYN_ARRAY_IMPLEMENTATION
    #include "dyn_array.h"
    ```

In all other files where you need the API, use `#include "dyn_array.h"` as normal.

