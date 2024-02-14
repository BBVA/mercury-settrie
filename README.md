# Mercury Settrie

![](https://img.shields.io/badge/-c++-black?logo=c%2B%2B&style=social)
![](https://img.shields.io/pypi/v/mercury-settrie?label=latest%20pypi%20build)
[![Python 3.8](https://img.shields.io/badge/python-3.8-blue.svg)](https://www.python.org/downloads/release/python-3816/)
[![Python 3.9](https://img.shields.io/badge/python-3.9-blue.svg)](https://www.python.org/downloads/release/python-3916/)
[![Python 3.10](https://img.shields.io/badge/python-3.10-blue.svg)](https://www.python.org/downloads/release/python-31011/)
[![Python 3.11](https://img.shields.io/badge/python-3.11-blue.svg)](https://www.python.org/downloads/release/python-3113/)
[![Apache 2 license](https://shields.io/badge/license-Apache%202-blue)](http://www.apache.org/licenses/LICENSE-2.0)
[![Ask Me Anything !](https://img.shields.io/badge/Ask%20me-anything-1abc9c.svg)](https://github.com/BBVA/mercury-settrie/issues)

## What is this?

### TLDR: A SetTrie is a container of sets that performs efficient subset and superset queries.

**Settrie** is a Python library implemented in C++ to create, update and query SetTrie objects.

  * Highly efficient C++ implementation.
  * 100% pythonic interface: objects are serializable, use iterators and support native Python sets.

### What problems is Settrie good for?

**Settrie** was born from the need of a better implementation of the algorithm for our recommender system. It has direct application to text
indexing for search in logarithmic time of documents containing a set of words. Think of a collection of documents as a container of
the set of words that each document has. Finding all the documents that contain a set of words is finding the superset of the set of words
in the query. A SetTrie will give you the answer --the list of document names-- in logarithmic time. The data structure is also used in
auto-complete applications.

### What data size can Settrie tackle?

Settrie is a C++ implementation with a Python interface. It is single threaded and can seamlessly operate over really large collections
of sets. Note that the main structure is a tree and a tree node is exactly 20 bytes, a billion nodes is 20 Gb, of course plus some
structures to store identifiers, etc. Note that the tree is compressing the documents by sharing the common parts and documents are already
compressed by considering them a set of words. An of-the-shelf computer can store in RAM a representation of terabytes of documents and
query result in much less than typing speed.

### How does this implementation compare to other Python implementations?

It is about 200 times faster and 20 times more memory efficient that a pure Python implementation.

## Try it without any installation in Google Colab

The API is very easy to use. You can see this benchmark notebook for reference.

  * Benchmark: Comparing two Settrie implementations [![Open In Colab](https://colab.research.google.com/assets/colab-badge.svg)](https://colab.research.google.com/github/BBVA/mercury-settrie/blob/master/notebooks/settrie_benchmark_colab.ipynb)


## Install

```bash
pip install mercury-settrie
```

## Usage

```python
from settrie import SetTrie

# Create a SetTrie object
stt = SetTrie()

# Insert some sets
stt.insert({2, 3}, 'id1')
stt.insert({2, 3, 4.4}, 'id2')
stt.insert({'Mon', 'Tue'}, 'days')

# Find id by set
print(stt.find({2, 3}))

# Find ids of all supersets
for id in stt.supersets({2, 3}):
    print(id)

# Find ids of all subsets
for id in stt.subsets({2, 3}):
    print(id)

# Nested iteration over the sets and elements
for st in stt:
    print(st.id)
    for e in st.elements:
        print('  ', e)

# Store as a pickle file file
import pickle
with open('my_settrie.pickle', 'wb') as f:
    pickle.dump(stt, f)

# Load from a pickle file
with open('my_settrie.pickle', 'rb') as f:
    tt = pickle.load(f)

# Check that they are identical
for t, st in zip(tt, stt):
    assert t.id == st.id
    for et, est in zip(t.elements, st.elements):
        assert et == est
```

## Clone and set up a development environment to work with it

To work with Settrie command line or develop Settrie, you can set up an environment with git, gcc, make and the following tools:

  * catch2 (Already included in source code)
  * doxygen 1.9.5 or better (to render C++ documentation)
  * mkdocs 1.4.2 or better (to render Python documentation)
  * swig 4.0.2
  * python 3.x with appropriate paths to python.h (see Makefile)

```bash
git clone https://github.com/BBVA/mercury-settrie.git
cd mercury-settrie/src

make
```

Make without arguments gives help. Try all the options. Everything should work assuming the tools are installed.

## Documentation

  * [Python API](https://bbva.github.io/mercury-settrie/reference/python/reference/settrie/)
  * [C++ API](https://bbva.github.io/mercury-settrie/reference/html/classSetTrie.html)
  * [Algorithm description: Index Data Structure for Fast Subset and Superset Queries](https://osebje.famnit.upr.si/~savnik/papers/cdares13.pdf)

## License

```text
                         Apache License
                   Version 2.0, January 2004
                http://www.apache.org/licenses/

     Copyright 2021-23, Banco de Bilbao Vizcaya Argentaria, S.A.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0
```