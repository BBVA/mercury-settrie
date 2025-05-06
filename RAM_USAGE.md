# RAM Usage, Known Limitations and How to Tweak Structure Size


## Purpose of this Document

This document explains technical details about the RAM usage. It was created to address the following github issue:

  * https://github.com/BBVA/mercury-settrie/issues/23


## settrie RAM usage

In general, the RAM usage of settrie should only be limited by available RAM. The "Pythonization" of the C++ data storage if done
byt the `image_put()`/`image_get()` methods which can handle any number of `ImageBlock` records. Despite `ImageBlock` being fixed size,
that is not a limitation. The copy of the memory C++ buffers in Pytyhon becomes a list of strings, each string being a base64 encoded
`ImageBlock`.

Nevertheless, before version 1.5.0 there were some choices in the C++ code that imposed limits the authors considered reasonable to keep
the source simple and were related with C++/Python serialization.


## Returning iterators with dynamic allocation

Version 1.5.1 introduced a new way to return iterators. This fixes the above mentioned issue.


## Known limitations

There are none known limitations. Memory allocation should only be limited by the available RAM. If you find any issues, please open
an issue in: https://github.com/BBVA/mercury-settrie/issues
