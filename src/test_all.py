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

import os, pickle

from settrie import SetTrie, Result, destroy_settrie, next_set_id, elements, set_name


def test_basic():
    s = SetTrie()

    s.insert({2, 3, 4}, 'id2')
    s.insert({2, 3, 4, 5}, 'id4')

    assert s.find({4, 3, 2}) == 'id2'

    ll = list(s.subsets({3, 4, 2}))

    assert len(ll) == 1 and ll[0] == 'id2'

    ll = list(s.supersets({2, 3, 4}))

    assert len(ll) == 2


def test_one_page_save_load():
    u = SetTrie()

    u.insert({2, 3, 4}, 'id2')
    u.insert({2, 3, 4, 5}, 'id4')

    bi = u.save_as_binary_image()

    assert type(bi) == list

    v = SetTrie(bi)

    assert v.find({2, 3, 4}) == 'id2'

    ll = list(v.subsets({2, 3, 4}))

    assert len(ll) == 1 and ll[0] == 'id2'

    ll = list(v.supersets({2, 3, 4}))

    assert len(ll) == 2


def test_multi_page_save_load():
    u = SetTrie()

    for i in range(2000):
        u.insert({2021, 3000 + i, 4000 + i*i}, 'idx_%s' % i)

    assert u.find({2021, 3003, 4009}) == 'idx_3'

    ll = list(u.supersets({2021}))

    assert len(ll) == 2000

    ll = list(u.supersets({3033}))

    assert len(ll) == 1 and ll[0] == 'idx_33'

    bi = u.save_as_binary_image()

    assert type(bi) == list

    v = SetTrie(bi)
    w = SetTrie()

    w.load_from_binary_image(bi)

    ll = list(v.supersets({2021}))

    assert len(ll) == 2000

    ll = list(w.supersets({3033}))

    assert len(ll) == 1 and ll[0] == 'idx_33'


def test_pickle_save_load():
    s = SetTrie()

    s.insert({2, 3, 4}, 'id2')
    s.insert({2, 3, 4, 5}, 'id4')

    assert s.find({4, 3, 2}) == 'id2'

    ll = list(s.subsets({3, 4, 2}))

    assert len(ll) == 1 and ll[0] == 'id2'

    ll = list(s.supersets({2, 3, 4}))

    assert len(ll) == 2

    with open('settrie.pickle', 'wb') as f:
        pickle.dump(s, f)

    del s

    with open('settrie.pickle', 'rb') as f:
        t = pickle.load(f)

    assert t.find({4, 3, 2}) == 'id2'

    ll = list(t.subsets({3, 4, 2}))

    assert len(ll) == 1 and ll[0] == 'id2'

    ll = list(t.supersets({2, 3, 4}))

    assert len(ll) == 2

    os.remove('settrie.pickle')


def test_force_errors():
    s = SetTrie()
    s.insert({1, 2, 3, 4}, 'id')
    destroy_settrie(s.st_id)
    s.st_id = 12345
    assert s.save_as_binary_image() is None

    s = SetTrie()
    assert not s.load_from_binary_image(['Load this, please.'])


def get_iterator_dataset():
    stt = SetTrie()

    names = ['integers', 'days', 'spanglish', 'días', 'more_integers', 'void', 'planets', 'pai']
    sets  = [{1, 2, 3, 4}, {'Monday', 'Tuesday', 'Wednesday', 'Thursday', 'Friday'},
             {'lunes', 'martes', 'Wednesday', 'Thursday', 'viernes'},
             {'lunes', 'martes', 'miércoles', 'jueves', 'viernes'}, {1, 2, 3, 4, 5}, {}, {'Earth', 'Mars', '...'}, {3.0, 3.1, 3.14}]

    for s, n in zip(sets, names):
       stt.insert(s, n)

    return stt, names, sets


def test_nested_iterator_calls():
    stt, names, sets = get_iterator_dataset()

    assert next_set_id(54321, -1) == -3

    st_id  = stt.st_id
    N      = 0
    seen   = []
    set_id = -1

    assert next_set_id(st_id, 54321) == -3

    st2 = SetTrie()

    assert next_set_id(st2.st_id, set_id) == -2

    assert elements(54321, 0) == 0
    assert elements(st2.st_id, 0) == 0
    assert elements(st_id, 54321) == 0

    while set_id != -2:
        set_id = next_set_id(st_id, set_id)

        if set_id == -2:
            break

        assert set_id >= 0

        name = set_name(st_id, set_id)
        assert name in names
        assert name not in seen
        seen.append(name)

        iix = elements(st_id, set_id)

        if name == 'void':
            assert iix == 0
        else:
            assert iix > 0

            ee = Result(iix, True)
            ix = names.index(name)
            Ne = 0
            Le = []

            for e in ee:
                assert e in sets[ix]
                assert e not in Le
                Le.append(e)
                Ne += 1

            assert Ne == len(sets[ix])

        N += 1

    assert N == len(names)


def test_nested_iterators():
    stt, names, sets = get_iterator_dataset()

    N    = 0
    seen = []

    for st in stt:
        assert st.id in names
        assert st.id not in seen
        seen.append(st.id)

        if st.id == 'void':
            assert st.elements is None
        else:
            ix = names.index(st.id)
            Ne = 0
            Le = []

            for e in st.elements:
                assert e in sets[ix]
                assert e not in Le
                Le.append(e)
                Ne += 1

            assert Ne == len(sets[ix])

        N += 1

    assert N == len(names)


# test_basic()
# test_one_page_save_load()
# test_multi_page_save_load()
# test_pickle_save_load()
# test_force_errors()
# test_nested_iterator_calls()
# test_nested_iterators()
