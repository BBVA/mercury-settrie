# Mercury-Settrie

#     Copyright 2023 Banco Bilbao Vizcaya Argentaria, S.A.

#     This product includes software developed at

#     BBVA (https://www.bbva.com/)

#       Licensed under the Apache License, Version 2.0 (the "License");
#     you may not use this file except in compliance with the License.
#     You may obtain a copy of the License at

#     http://www.apache.org/licenses/LICENSE-2.0

#       Unless required by applicable law or agreed to in writing, software
#     distributed under the License is distributed on an "AS IS" BASIS,
#     WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#     See the License for the specific language governing permissions and
#     limitations under the License.

import re

from . import new_settrie
from . import destroy_settrie
from . import insert
from . import find
from . import supersets
from . import subsets
from . import elements
from . import num_sets
from . import next_set_id
from . import set_name
from . import remove
from . import purge
from . import iterator_size
from . import iterator_next
from . import destroy_iterator
from . import save_as_binary_image
from . import push_binary_image_block
from . import binary_image_size
from . import binary_image_next
from . import destroy_binary_image

from typing import Set


class Result:
    """ Container holding the results of several operations of SetTrie.
    It behaves, basically, like an iterator.
    """
    def __init__(self, iter_id, auto_serialize = False):
        self.iter_id = iter_id
        self.as_is   = True
        if auto_serialize:
            self.as_is     = False
            self.to_string = re.compile("^'(.+)'$")
            self.to_float  = re.compile('^.*\\..*$')

    def __del__(self):
        destroy_iterator(self.iter_id)

    def __iter__(self):
        return self

    def __len__(self):
        return iterator_size(self.iter_id)

    def __next__(self):
        if iterator_size(self.iter_id) > 0:
            if self.as_is:
                return iterator_next(self.iter_id)

            s = iterator_next(self.iter_id)

            if self.to_string.match(s):
                return self.to_string.sub('\\1', s).replace('\udc82', ',')

            if self.to_float.match(s):
                return float(s)

            return int(s)

        else:
            raise StopIteration


class TreeSet:
    """ Class returned by the iterator of SetTrie to simplify iterating over the elements
    while not computing a list of strings (calling c++ elements()) unless it is required.
    """
    def __init__(self, st_id, set_id):
        self.st_id  = st_id
        self.set_id = set_id

    @property
    def id(self):
        return set_name(self.st_id, self.set_id)

    @property
    def elements(self):
        return Result(elements(self.st_id, self.set_id), auto_serialize=True)


class SetTrie:
    """ Mapping container for efficient storage of key-value pairs where
    the keys are sets. Uses an efficient trie implementation. Supports querying
    for values associated to subsets or supersets of stored key sets.

    Example:
        ```python
        >>> from settrie import SetTrie
        >>>
        >>> # Create a SetTrie object
        >>> stt = SetTrie()
        >>>
        >>> # Insert some sets
        >>> stt.insert({2, 3}, 'id1')
        >>> stt.insert({2, 3, 4.4}, 'id2')
        >>> stt.insert({'Mon', 'Tue'}, 'days')
        >>>
        >>> # Find id by set
        >>> print(stt.find({2, 3}))
        >>>
        >>> # Find ids of all supersets
        >>> for id in stt.supersets({2, 3}):
        >>>     print(id)
        >>>
        >>> # Find ids of all subsets
        >>> for id in stt.subsets({2, 3}):
        >>>     print(id)
        >>>
        >>> # Nested iteration over the sets and elements
        >>> for st in stt:
        >>>     print(st.id)
        >>>     for e in st.elements:
        >>>         print('  ', e)
        >>>
        >>> # Store as a pickle file file
        >>> import pickle
        >>> with open('my_settrie.pickle', 'wb') as f:
        >>>     pickle.dump(stt, f)
        >>>
        >>> # Load from a pickle file
        >>> with open('my_settrie.pickle', 'rb') as f:
        >>>     tt = pickle.load(f)
        >>>
        >>> # Check that they are identical
        >>> for t, st in zip(tt, stt):
        >>>     assert t.id == st.id
        >>>     for et, est in zip(t.elements, st.elements):
        >>>         assert et == est
        >>>
        >>> # Remove sets by id
        >>> stt.remove('id2')
        >>> stt.remove('days')
        >>>
        >>> # After many .remove() calls, the tree has nodes marked as dirty,
        >>> # calling .purge() removes them completely and frees RAM.
        >>> stt.purge()
        ```
    """
    def __init__(self, binary_image=None):
        self.st_id	 = new_settrie()
        self.set_id	 = -1
        self.int_ids = None
        if binary_image is not None:
            self.load_from_binary_image(binary_image)

    def __iter__(self):
        return self

    def __len__(self):
        return num_sets(self.st_id)

    def __next__(self):
        if self.set_id < 0:
            self.set_id = -1

        self.set_id = next_set_id(self.st_id, self.set_id)

        if self.set_id < 0:
            raise StopIteration

        return TreeSet(self.st_id, self.set_id)

    def __del__(self):
        destroy_settrie(self.st_id)

    def __getstate__(self):
        """ Used by pickle.dump() (See https://docs.python.org/3/library/pickle.html)
        """
        return self.save_as_binary_image()

    def __setstate__(self, state):
        """ Used by pickle.load() (See https://docs.python.org/3/library/pickle.html)
        """
        self.st_id = new_settrie()
        self.load_from_binary_image(state)

    def insert(self, set: Set, id: str):
        """ Inserts a new set into a SetTrie object.

        Args:
            set: Set to add
            id: String representing the ID for the test
        """
        self.int_ids = None
        insert(self.st_id, str(set), id)

    def find(self, set) -> str:
        """ Finds the ID of the set matching the one provided.

        Args:
            set (set): Set for searching
        Returns:
            id of the set with the exact match. An empty string if no match was found.
        """
        return find(self.st_id, str(set))

    def supersets(self, set) -> Result:
        """ Find all the supersets of a given set.

        Args:
            set (set): set for which we want to find all the supersets

        Returns:
            Iterator object with the IDs of the matching supersets.
        """
        return Result(supersets(self.st_id, str(set)))

    def subsets(self, set) -> Result:
        """ Find all the subsets for a given set.

        Args:
            set (set): set for which we want to find all the supersets

        Returns:
            Iterator object with the IDs of the matching subsets.
        """
        return Result(subsets(self.st_id, str(set)))

    def remove(self, id):
        """ Removes a set from the object either by string identifier or by its unique integer id.

        If you need to remove multiple elements, it is more efficient to avoid inserting or purging between remove() calls. The
        object needs to build a dictionary with all the unique integer ids. Since ids change by insert() and purge() calls, the
        dictionary will be rebuilt each time, which requires iterating over the whole object.

        Args:
            id (str): Either the same string used as the id when the set was inserted via `insert()` or the unique integer id, if known.
                The latter is faster. The unique integer id is the id used by the iterator when you iterate over the whole object
                to identify the specific set.

        Returns:
            (int): Zero if the set was removed, a negative integer code on error.
        """
        self.set_id	= -1

        if type(id) is int:
            self.int_ids = None
            return remove(self.st_id, id)

        if self.int_ids is None:
            self.int_ids = dict()
            int_id = next_set_id(self.st_id, -1)
            while int_id >= 0:
                self.int_ids[set_name(self.st_id, int_id)] = int_id
                int_id = next_set_id(self.st_id, int_id)

        int_id = self.int_ids.pop(id, None)
        if int_id is None:
            return -1

        return remove(self.st_id, int_id)

    def purge(self):
        """ Purges (reassigns node integer ids and frees RAM) after a series of remove() calls.

        If you need to remove multiple elements, it is more efficient to avoid inserting or purging between remove() calls.
        Call purge() just once after you have finished removing.

        Returns:
            (int): The number of tree nodes freed.
        """
        self.set_id	= -1

        size = purge(self.st_id, 1) # dry run

        if size == 0:
            return 0

        self.int_ids = None

        purge(self.st_id, 0)

        return size

    def save_as_binary_image(self):
        """ Saves the state of the c++ SetTrie object as a Python
            list of strings referred to a binary_image.

        Returns:
            (list): The binary_image containing the state of the SetTrie. There is
                not much you can do with it except serializing it as a Python
                (e.g., pickle) object and loading it into another SetTrie object.
                Pass it to the constructor to create an initialized object,
        """
        bi_idx = save_as_binary_image(self.st_id)
        if bi_idx == 0:
            return None

        bi = []
        bi_size = binary_image_size(bi_idx)
        for t in range(bi_size):
            bi.append(binary_image_next(bi_idx))

        destroy_binary_image(bi_idx)

        return bi

    def load_from_binary_image(self, binary_image):
        """ Load the state of the c++ SetTrie object from a binary_image
            returned by a previous save_as_binary_image() call.

        Args:
            binary_image (list): A list of strings returned by save_as_binary_image()

        Returns:
            (bool): True on success, destroys, initializes and returns false on failure.
        """
        self.int_ids = None

        failed = False

        for binary_image_block in binary_image:
            if not push_binary_image_block(self.st_id, binary_image_block):
                failed = True
                break

        if not failed:
            failed = not push_binary_image_block(self.st_id, '')

        self.set_id = -1

        if failed:
            destroy_settrie(self.st_id)
            self.st_id = new_settrie()
            return False

        return True

    def __deepcopy__(self, memo):
        return SetTrie(binary_image=self.save_as_binary_image())
