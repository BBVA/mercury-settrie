# RAM Usage, Known Limitations and How to Tweak Structure Size


## Purpose of this Document

This document explains technical details about the RAM usage. It was created to address the following github issue:

  * https://github.com/BBVA/mercury-settrie/issues/23


## settrie RAM usage

In general, the RAM usage of settrie should only be limited by available RAM. The "Pythonization" of the C++ data storage if done
byt the `image_put()`/`image_get()` methods which can handle any number of `ImageBlock` records. Despite `ImageBlock` being fixed size,
that is not a limitation. The copy of the memory C++ buffers in Pytyhon becomes a list of strings, each string being a base64 encoded
`ImageBlock`.

Nevertheless, there are some choices in the C++ code that impose limits the authors considered reasonable to keep the source simple and
are related with C++/Python serialization that can be tweaked (see below).


## Known Limitations

The following structures are fixed in size:


char answer_buffer [8192];
char answer_block  [8208];	// 4K + final zero aligned to 16 bytes

bool SetTrie::load (pBinaryImage &p_bi) {
	char		buffer[8192];
