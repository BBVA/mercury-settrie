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

from . import new_settrie
from . import destroy_settrie
from . import insert
from . import find
from . import supersets
from . import subsets
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
    def __init__(self, iter_id):
        self.iter_id = iter_id

    def __del__(self):
        destroy_iterator(self.iter_id)

    def __iter__(self):
        return self

    def __next__(self):
        if iterator_size(self.iter_id) > 0:
            return iterator_next(self.iter_id)
        else:
            raise StopIteration


class SetTrie:
    """ Mapping container for efficient storage of key-value pairs where
    the keys are sets. Uses an efficient trie implementation. Supports querying
    for values associated to subsets or supersets of stored key sets.

    Example:
        ```python
        >>> from mercury.dynamics.SetTrie import SetTrie
        >>> s = SetTrie()
        >>> s.insert({2,3}, 'id1')
        >>> s.insert({2,3,4}, 'id2')
        ```
    """
    def __init__(self, binary_image=None):
        self.st_id = new_settrie()

        if binary_image is not None:
            self.load_from_binary_image(binary_image)

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
            id: String representind the ID for the test
        """
        insert(self.st_id, str(set), id)

    def find(self, set) -> str:
        """ Finds the ID of the set matching the one provided.

        Args:
            set: Set for searching
        Returns:
            id of the set with the exact match. An empty string if no match was found.
        """
        return find(self.st_id, str(set))

    def supersets(self, set) -> Result:
        """ Find all the supersets of a given set.

        Args:
            set: set for which we want to find all the supersets

        Returns:
            Iterator object with the IDs of the matching supersets.
        """
        return Result(supersets(self.st_id, str(set)))

    def subsets(self, set) -> Result:
        """ Find all the subsets for a given set.

        Args:
            set: set for which we want to find all the supersets

        Returns:
            Iterator object with the IDs of the matching subsets.
        """
        return Result(subsets(self.st_id, str(set)))

    def save_as_binary_image(self):
        """ Saves the state of the c++ SetTrie object as a Python
            list of strings referred to a binary_image.

        Returns:
            The binary_image containing the state of the SetTrie. There is
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
            binary_image: A list of strings returned by save_as_binary_image()

        Returns:
            True on success, destroys, initializes and returns false on failure.
        """
        failed = False

        for binary_image_block in binary_image:
            if not push_binary_image_block(self.st_id, binary_image_block):
                failed = True
                break

        if not failed:
            failed = not push_binary_image_block(self.st_id, '')

        if failed:
            destroy_settrie(self.st_id)
            self.st_id = new_settrie()
            return False

        return True
