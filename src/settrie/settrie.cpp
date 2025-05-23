/* Mercury-Settrie

	Copyright 2023 Banco Bilbao Vizcaya Argentaria, S.A.

	This product includes software developed at

	BBVA (https://www.bbva.com/)

	  Licensed under the Apache License, Version 2.0 (the "License");
	you may not use this file except in compliance with the License.
	You may obtain a copy of the License at

	http://www.apache.org/licenses/LICENSE-2.0

	  Unless required by applicable law or agreed to in writing, software
	distributed under the License is distributed on an "AS IS" BASIS,
	WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
	See the License for the specific language governing permissions and
	limitations under the License.
*/
#include <iostream>
#include <fstream>
#include <sstream>
#include <string.h>


#include "settrie.h"


#define MURMUR_SEED	  76493		///< Just a 5 digit prime

/** \brief MurmurHash2, 64-bit versions, by Austin Appleby

	(from https://sites.google.com/site/murmurhash/) a 64-bit hash for 64-bit platforms

	All code is released to the public domain. For business purposes, Murmurhash is
	under the MIT license.

	The same caveats as 32-bit MurmurHash2 apply here - beware of alignment
	and endian-ness issues if used across multiple platforms.

	\param key address of the memory block to hash.
	\param len Number of bytes to hash.
	\return	 64-bit hash of the memory block.

*/
uint64_t MurmurHash64A (const void *key, int len) {
	const uint64_t m = 0xc6a4a7935bd1e995;
	const int	   r = 47;

	uint64_t h = MURMUR_SEED ^ (len*m);

	const uint64_t *data = (const uint64_t *) key;
	const uint64_t *end	 = data + (len/8);

	while(data != end) {
		uint64_t k = *data++;

		k *= m;
		k ^= k >> r;
		k *= m;

		h ^= k;
		h *= m;
	}

	const unsigned char *data2 = (const unsigned char*) data;

	switch(len & 7)	{
	case 7: h ^= uint64_t(data2[6]) << 48;
	case 6: h ^= uint64_t(data2[5]) << 40;
	case 5: h ^= uint64_t(data2[4]) << 32;
	case 4: h ^= uint64_t(data2[3]) << 24;
	case 3: h ^= uint64_t(data2[2]) << 16;
	case 2: h ^= uint64_t(data2[1]) << 8;
	case 1: h ^= uint64_t(data2[0]);

		h *= m;
	};

	h ^= h >> r;
	h *= m;
	h ^= h >> r;

	return h;
}


inline bool image_put(pBinaryImage p_bi, void *p_data, int size) {

	ImageBlock blk;
	int c_block = p_bi->size() - 1;

	if (c_block < 0) {
		blk.size	  = 0;
		blk.block_num = 1;

		p_bi->push_back(blk);

		c_block = 0;
	} else if ((*p_bi)[c_block].size == IMAGE_BUFF_SIZE) {
		blk.size	  = 0;
		blk.block_num = ++c_block + 1;

		p_bi->push_back(blk);
	}

	while (size > 0) {
		int uu_size = (*p_bi)[c_block].size;
		int mv_size = std::min(size, IMAGE_BUFF_SIZE - uu_size);

		if (uu_size > 0)
			blk = (*p_bi)[c_block];
		else {
			blk.size	  = 0;
			blk.block_num = c_block + 1;
		}

		memcpy(&blk.buffer[uu_size], p_data, mv_size);

		p_data	  = (char *) p_data + mv_size;
		size	 -= mv_size;
		blk.size += mv_size;

		(*p_bi)[c_block] = blk;

		if (blk.size == IMAGE_BUFF_SIZE) {
			blk.size	  = 0;
			blk.block_num = ++c_block + 1;

			p_bi->push_back(blk);
		}
	}

	return size == 0;
}


inline bool image_get(pBinaryImage p_bi, int &c_block, int &c_ofs, void *p_data, int size) {

	while (size > 0) {
		if (c_block < 0 || c_block >= (int) p_bi->size())
			return false;

		int uu_size = (*p_bi)[c_block].size;
		int mv_size = std::min(size, uu_size - c_ofs);

		if (mv_size <= 0) {
			if (mv_size < 0 || uu_size != IMAGE_BUFF_SIZE)
				return false;

			c_block++;
			c_ofs = 0;

			uu_size = (*p_bi)[c_block].size;
			mv_size = std::min(size, uu_size - c_ofs);
		}

		memcpy(p_data, &(*p_bi)[c_block].buffer[c_ofs], mv_size);

		p_data  = (char *) p_data + mv_size;
		size   -= mv_size;
		c_ofs  += mv_size;
	}

	return size == 0;
}

// -----------------------------------------------------------------------------------------------------------------------------------------
//	SetTrie Implementation
// -----------------------------------------------------------------------------------------------------------------------------------------

void SetTrie::insert (StringSet set, String str_id) {

	BinarySet b_set = {};

	int size = set.size();

	if (size == 0) {
		String empty = {""};
		assign_hh_nam(0, empty);

		id[insert(b_set)] = str_id;

		return;
	}

	for (int i = 0; i < size; i++) {
		ElementHash hh = MurmurHash64A(set[i].c_str(), set[i].length());

		assign_hh_nam(hh, set[i]);

		b_set.push_back(hh);
	}
	std::sort(b_set.begin(), b_set.end());

	b_set.erase(unique(b_set.begin(), b_set.end()), b_set.end());

	id[insert(b_set)] = str_id;
}


void SetTrie::insert (String str, String str_id, char split) {
	StringSet set;
	std::stringstream ss(str);

	String elem;
	while (std::getline(ss, elem, split))
		set.push_back(elem);

	insert(set, str_id);
}


String SetTrie::find (StringSet set) {
	if (set.size() == 0) {
		IdMap::iterator it;
		if ((it = id.find(0)) != id.end())
			return it->second;

		return "";
	}

	BinarySet b_set = {};
	int size = set.size();

	for (int i = 0; i < size; i++) {
		ElementHash hh = MurmurHash64A(set[i].c_str(), set[i].length());

		if (hh_nam.find(hh) == hh_nam.end())
			return "";

		b_set.push_back(hh);
	}
	std::sort(b_set.begin(), b_set.end());

	b_set.erase(unique(b_set.begin(), b_set.end()), b_set.end());

	int idx = find(b_set);

	if (idx == 0 || tree[idx].state != STATE_HAS_SET_ID)
		return "";

	return id[idx];
}


String SetTrie::find (String str, char split) {
	StringSet set;
	std::stringstream ss(str);

	String elem;
	while (std::getline(ss, elem, split))
		set.push_back(elem);

	return find(set);
}


StringSet SetTrie::supersets (StringSet set) {

	StringSet ret = {};

	int size = set.size();

	if (size == 0) {
		// FIX (2024/02/28): All sets are the supersets of the empty set.
		for (IdMap::iterator it = id.begin(); it != id.end(); ++it)
			ret.push_back(it->second);

		return ret;
	}

	query.clear();

	for (int i = 0; i < size; i++) {
		ElementHash hh = MurmurHash64A(set[i].c_str(), set[i].length());

		if (hh_nam.find(hh) == hh_nam.end())
			return ret;

		query.push_back(hh);
	}
	std::sort(query.begin(), query.end());

	query.erase(unique(query.begin(), query.end()), query.end());

	last_query_idx = query.size() - 1;

	result.clear();

	supersets(tree[0].idx_child, 0);

	size = result.size();
	for (int i = 0; i < size; i++)
		ret.push_back(id[result[i]]);

	return ret;
}


StringSet SetTrie::supersets (String str, char split) {
	StringSet set;
	std::stringstream ss(str);

	String elem;
	while (std::getline(ss, elem, split))
		set.push_back(elem);

	return supersets(set);
}


StringSet SetTrie::subsets (StringSet set) {

	StringSet ret = {};

	IdMap::iterator it;
	if ((it = id.find(0)) != id.end())
		ret.push_back(it->second);

	query.clear();

	int size = set.size();

	for (int i = 0; i < size; i++) {
		ElementHash hh = MurmurHash64A(set[i].c_str(), set[i].length());

		if (hh_nam.find(hh) != hh_nam.end())
			query.push_back(hh);
	}
	if (query.size() == 0)
		return ret;

	std::sort(query.begin(), query.end());

	query.erase(unique(query.begin(), query.end()), query.end());

	last_query_idx = query.size() - 1;

	result.clear();

	subsets(tree[0].idx_child, 0);

	size = result.size();
	for (int i = 0; i < size; i++)
		ret.push_back(id[result[i]]);

	return ret;
}


StringSet SetTrie::subsets (String str, char split) {
	StringSet set;
	std::stringstream ss(str);

	String elem;
	while (std::getline(ss, elem, split))
		set.push_back(elem);

	return subsets(set);
}


StringSet SetTrie::elements	(int idx) {

	StringSet ret = {};

	if (idx > 0 && idx < tree.size() && tree[idx].state == STATE_HAS_SET_ID) {
		String elem;
		while (idx > 0) {
			ElementHash hh = tree[idx].value;

			StringName::iterator it = hh_nam.find(hh);

			if (it != hh_nam.end())
				ret.push_back(it->second.name);

			idx = tree[idx].idx_parent;
		}
	}

	return ret;
}


int SetTrie::remove	(int idx) {

	if (idx < 0 || idx >= tree.size() || tree[idx].state != STATE_HAS_SET_ID)
		return -2;

	IdMap::iterator it = id.find(idx);

	if (it == id.end())
		return -3;

	id.erase(it);

	if (idx == 0) {
		tree[idx].state = STATE_IN_USE;
		StringName::iterator it = hh_nam.begin();
		if (it != hh_nam.end() && it->first == tree[0].value)
			hh_nam.erase(it);

		return 0;
	}

	int i = idx;
	while (i > 0) {
		ElementHash hh = tree[i].value;

		StringName::iterator it = hh_nam.find(hh);

		if (it != hh_nam.end())
			if (--it->second.count == 0)
				hh_nam.erase(it);

		i = tree[i].idx_parent;
	}

	if (tree[idx].idx_child != 0)
		tree[idx].state = STATE_IN_USE;
	else {
		int stop = false;

		while (!stop) {
			int lx = tree[idx].idx_parent, j;

			if ((j = tree[lx].idx_child) == idx) {
				j = tree[idx].idx_next;
				tree[lx].idx_child = j;
				stop = (j != 0) || (tree[lx].state == STATE_HAS_SET_ID) || (lx == 0);
			} else {
				lx = j;
				while ((j = tree[lx].idx_next) != idx) lx = j;
				tree[lx].idx_next = tree[idx].idx_next;
				stop = true;
			}

			tree[idx] = {0xbaadF00DbaadF00D, -1, -1, -1, STATE_IS_GARBAGE};
			num_dirty_nodes++;

			idx = lx;
		}
	}

	return 0;
}


int SetTrie::purge () {

	if (num_dirty_nodes <= 0)
		return -1;

	int ni = 0, size = tree.size();
	std::map<int, int> is = {}, was = {};

	for (int i = 0; i < size; i++) {
		if (tree[i].state != STATE_IS_GARBAGE) {
			was[ni] = i;
			is [i]  = ni;
			ni++;
		}
	}

	size = was.size();
	for (int i = 0; i < size; i++) {
		ni = was[i];
		if (i != ni)
			tree[i] = tree[ni];
		tree[i].idx_child  = is[tree[i].idx_child];
		tree[i].idx_next   = is[tree[i].idx_next];
		tree[i].idx_parent = is[tree[i].idx_parent];
	}

	IdMap id2 = id;
	id = {};
	for (IdMap::iterator it = id2.begin(); it != id2.end(); ++it)
		id[is[it->first]] = it->second;

	tree.resize(size);

	num_dirty_nodes = 0;

	return 0;
}


bool SetTrie::load (pBinaryImage &p_bi) {

	int c_block = 0, c_ofs = 0;

	String		section = "tree";
	ElementHash hs;
	char		buffer[8192];

	if (!image_get(p_bi, c_block, c_ofs, &hs, sizeof(hs)))
		return false;

	if ((hs != MurmurHash64A(section.c_str(), section.length())) || (tree.size() != 1))
		return false;

	int len;

	if (!image_get(p_bi, c_block, c_ofs, &len, sizeof(len)))
		return false;

	for (int i = 0; i < len; i++) {
		SetNode sn;
		if (!image_get(p_bi, c_block, c_ofs, &sn, sizeof(sn)))
			return false;

		if (i == 0)
			tree[0] = sn;
		else
			tree.push_back(sn);
	}

	section = "name";

	if (!image_get(p_bi, c_block, c_ofs, &hs, sizeof(hs)))
		return false;

	if ((hs != MurmurHash64A(section.c_str(), section.length())) || (hh_nam.size() != 0))
		return false;

	if (!image_get(p_bi, c_block, c_ofs, &len, sizeof(len)))
		return false;

	for (int i = 0; i < len; i++) {
		ElementHash hh;
		int			ll, count;

		if (!image_get(p_bi, c_block, c_ofs, &hh, sizeof(hh)))
			return false;

		if (!image_get(p_bi, c_block, c_ofs, &count, sizeof(count)))
			return false;

		if (!image_get(p_bi, c_block, c_ofs, &ll, sizeof(ll)))
			return false;

		if ((ll < 0) || (ll >= 8192))
			return false;

		if (ll == 0) {
			hh_nam[hh].count = count;
			hh_nam[hh].name	 = (char *) "";
		} else {
			if (!image_get(p_bi, c_block, c_ofs, &buffer, ll))
				return false;

			buffer[ll] = 0;

			hh_nam[hh].count = count;
			hh_nam[hh].name	 = buffer;
		}
	}

	section = "id";

	if (!image_get(p_bi, c_block, c_ofs, &hs, sizeof(hs)))
		return false;

	if ((hs != MurmurHash64A(section.c_str(), section.length())) || (id.size() != 0))
		return false;

	if (!image_get(p_bi, c_block, c_ofs, &len, sizeof(len)))
		return false;

	for (int i = 0; i < len; i++) {
		int ii, ll;

		if (!image_get(p_bi, c_block, c_ofs, &ii, sizeof(ii)))
			return false;

		if (!image_get(p_bi, c_block, c_ofs, &ll, sizeof(ll)))
			return false;

		if ((ll < 0) || (ll >= 8192))
			return false;

		if (ll == 0)
			id[ii] = (char *) "";
		else {
			if (!image_get(p_bi, c_block, c_ofs, &buffer, ll))
				return false;

			buffer[ll] = 0;

			id[ii] = buffer;
		}
	}

	section = "end";

	if (!image_get(p_bi, c_block, c_ofs, &hs, sizeof(hs)))
		return false;

	return hs == MurmurHash64A(section.c_str(), section.length());
}


bool SetTrie::save (pBinaryImage &p_bi) {

	String section = "tree";
	ElementHash hs = MurmurHash64A(section.c_str(), section.length());

	image_put(p_bi, &hs, sizeof(hs));

	int len = tree.size();

	image_put(p_bi, &len, sizeof(len));
	image_put(p_bi, tree.data(), len*sizeof(SetNode));

	section = "name";
	hs		= MurmurHash64A(section.c_str(), section.length());

	image_put(p_bi, &hs, sizeof(hs));

	len = hh_nam.size();

	image_put(p_bi, &len, sizeof(len));

	for (StringName::iterator it = hh_nam.begin(); it != hh_nam.end(); ++it) {
		ElementHash hh = it->first;
		image_put(p_bi, &hh, sizeof(hh));
		image_put(p_bi, &it->second.count, sizeof(int));
		int ll = it->second.name.length();
		image_put(p_bi, &ll, sizeof(ll));
		image_put(p_bi, (void *) it->second.name.c_str(), ll);
	}

	section = "id";
	hs		= MurmurHash64A(section.c_str(), section.length());

	image_put(p_bi, &hs, sizeof(hs));

	len = id.size();

	image_put(p_bi, &len, sizeof(len));

	for (IdMap::iterator it = id.begin(); it != id.end(); ++it) {
		int ii = it->first;
		image_put(p_bi, &ii, sizeof(ii));
		int ll = it->second.length();
		image_put(p_bi, &ll, sizeof(ll));
		image_put(p_bi, (void *) it->second.c_str(), ll);
	}

	section = "end";
	hs		= MurmurHash64A(section.c_str(), section.length());

	image_put(p_bi, &hs, sizeof(hs));

	return true;
}

// -----------------------------------------------------------------------------------------------------------------------------------------
//	Python Implementation
// -----------------------------------------------------------------------------------------------------------------------------------------

typedef SetTrie		*pSetTrie;
typedef StringSet	*pStringSet;

typedef std::map<int, pSetTrie>		SetTrieServer;
typedef std::map<int, pStringSet>	IterServer;
typedef std::map<int, pBinaryImage>	BinaryImageServer;

int instance_num  = 0;
int instance_iter = 0;

SetTrieServer	  instance = {};
IterServer		  iterator = {};
BinaryImageServer image	   = {};

int max_id_length	= 1024;
char *p_answer		= nullptr;
char answer_block  [8208];	// 4K + final zero aligned to 16 bytes

const char b64chars[]	   = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
uint8_t	   b64inverse[256] = {0};

char *image_block_as_string(uint8_t *p_in) {

	char *p_out	  = (char *) &answer_block;

	uint8_t v, w;
	for (int i = 0; i < 2048; i++) {
		v = *(p_in++);
		*(p_out++) = b64chars[v >> 2];

		w = *(p_in++);
		*(p_out++) = b64chars[((v << 4) & 0x30) | w >> 4];

		v = *(p_in++);
		*(p_out++) = b64chars[((w << 2) & 0x3c) | v >> 6];
		*(p_out++) = b64chars[v & 0x3f];
	}
	*p_out = 0;

	return (char *) &answer_block;
}


char *image_block_as_string(ImageBlock &blk) {
	uint8_t *p_in = (uint8_t *) &blk;

	return image_block_as_string(p_in);
}


bool string_as_image_block(ImageBlock &blk, char *p_in) {

	if (b64inverse[0] != 0xc0) {
		// create the inverse of b64chars[] in b64inverse[] just once if not exists
		memset(&b64inverse, 0xc0c0c0c0, sizeof(b64inverse));

		for (int i = 0; i < 64; i++)
			b64inverse[(int) b64chars[i]] = i;
	}
	uint8_t *p_out = (uint8_t *) &blk;

	uint8_t v, w;
	for (int i = 0; i < 2048; i++) {
		v = b64inverse[(int) *(p_in++)];
		if ((v & 0xc0) != 0)
			return false;

		w = b64inverse[(int) *(p_in++)];
		if ((w & 0xc0) != 0)
			return false;

		*(p_out++) = (v << 2) | (w >> 4);

		v = b64inverse[(int) *(p_in++)];
		if ((v & 0xc0) != 0)
			return false;

		*(p_out++) = (w << 4) | (v >> 2);

		w = b64inverse[(int) *(p_in++)];
		if ((w & 0xc0) != 0)
			return false;

		*(p_out++) = (v << 6) | w;
	}

	return (uint_fast64_t) p_out - (uint_fast64_t) &blk == sizeof(ImageBlock);
}


String python_set_as_string(char *p_char) {

	int size = strlen(p_char);

	if (size == 5 && strcmp(p_char, "set()") == 0)
		return "";

	if (size > 10 && *((int*) p_char) == 0x7a6f7266) {
		// frozenset({ . . . })
		// ^[0]     ^[9]      ^[size - 1]

		if (p_char[9] != '(' || p_char[size - 1] != ')')
			return "";

		p_char += 10;
		size   -= 11;

		if (size < 3)
			return "";
	}

	if (p_char[0] != '{' || p_char[size - 1] != '}')
		return String(p_char);

	String s;
	p_char++;
	size -= 2;
	int quote_lev = 0;		// 1 for ', 2 for "
	bool trailing = false;

	while (size-- > 0) {
		char cursor;
		switch (cursor = *(p_char++)) {
		case '\'':
			switch (quote_lev) {
			case 0:
				quote_lev = 1;
				break;

			case 1:
				quote_lev = 0;
				break;
			};
			s.push_back('\'');
			trailing = false;
			break;

		case '"':
			switch (quote_lev) {
			case 0:
				quote_lev = 2;
				break;

			case 2:
				quote_lev = 0;
				break;
			};
			s.push_back('"');
			trailing = false;
			break;

		case ' ':
			if (!trailing)
				s.push_back(' ');
			break;

		case ',':
			if (quote_lev == 0) {
				s.push_back(',');
				trailing = true;
			} else
				s.push_back(0x82);
			break;

		default:
			s.push_back(cursor);
			trailing = false;
		}
	}

	return s;
}


/** Return a buffer to write an answer that is as big as max_id_length. max_id_length tracks the largest set id used by insert.
*/
char *get_answer_buffer () {
	if (p_answer == nullptr)
		p_answer = (char *) malloc(max_id_length + 16);

	return p_answer;
}


/** Check if the current set id is larger than max_id_length. If so, reallocate the buffer to the new size.

	\param size  The new size of the id.
*/
void inline set_answer_buffer_size (int size) {
	if (size > max_id_length) {
		if (p_answer != nullptr)
			free(p_answer);

		p_answer = (char *) malloc(size + 16);
		max_id_length = size;
	}
}

// -----------------------------------------------------------------------------------------------------------------------------------------

/** Create a new SetTrie object that can be used via the Python interface.

	\return	A unique ID that can be passed as the st_id parameter for any method in the Python interface.

To free the resources allocated by this ID, the (python) caller must call destroy_settrie() with the st_id and never use the same
st_id after that.
*/
int new_settrie() {

	instance[++instance_num] = new SetTrie();

	return instance_num;
}


/** Destroy a SetTrie object that was used via the Python interface.

	\param st_id  The st_id returned by a previous new_settrie() call.
*/
void destroy_settrie(int st_id) {

	SetTrieServer::iterator it = instance.find(st_id);

	if (it == instance.end())
		return;

	delete it->second;

	instance.erase(it);
}


/** Insert a Python set (serialized by a str() call) into a SetTrie object.

	\param st_id  The st_id returned by a previous new_settrie() call.
	\param set	  A Python set serialized by a str() call.
	\param str_id An id representing this set that will be returned in searches.
*/
void insert	(int st_id, char *set, char *str_id) {

	SetTrieServer::iterator it = instance.find(st_id);

	if (it != instance.end()) {
		String s = String(str_id);
		it->second->insert(python_set_as_string(set), s, ',');
		set_answer_buffer_size(s.length());
	}
}


/** Find a Python set (serialized by a str() call) for a complete match inside a SetTrie object.

	\param st_id  The st_id returned by a previous new_settrie() call.
	\param set	  A Python set serialized by a str() call.

	\return		  The str_id string that was given to the set when it was inserted.
*/
char *find (int st_id, char *set) {

	SetTrieServer::iterator it = instance.find(st_id);

	char *p_ans = get_answer_buffer();

	p_ans[0] = 0;

	if (it != instance.end())
		strcpy(p_ans, it->second->find(python_set_as_string(set), ',').c_str());

	return p_ans;
}


/** Find all the supersets of a given Python set (serialized by a str() call) stored inside a SetTrie object.

	\param st_id  The st_id returned by a previous new_settrie() call.
	\param set	  A Python set serialized by a str() call.

	\return		  0 if no sets were found, or an iter_id > 0 that can be used to retrieve the result using iterator_next()/iterator_size()
				  and must be explicitly destroyed via destroy_iterator()
*/
int supersets (int st_id, char *set) {

	SetTrieServer::iterator it = instance.find(st_id);

	if (it == instance.end())
		return 0;

	StringSet ret = it->second->supersets(python_set_as_string(set), ',');

	if (ret.size() == 0)
		return 0;

	iterator[++instance_iter] = new StringSet(ret);

	return instance_iter;
}


/** Find all the subsets of a given Python set (serialized by a str() call) stored inside a SetTrie object.

	\param st_id  The st_id returned by a previous new_settrie() call.
	\param set	  A Python set serialized by a str() call.

	\return		  0 if no sets were found, or an iter_id > 0 that can be used to retrieve the result using iterator_next()/iterator_size()
				  and must be explicitly destroyed via destroy_iterator()
*/
int subsets (int st_id, char *set) {

	SetTrieServer::iterator it = instance.find(st_id);

	if (it == instance.end())
		return 0;

	StringSet ret = it->second->subsets(python_set_as_string(set), ',');

	if (ret.size() == 0)
		return 0;

	iterator[++instance_iter] = new StringSet(ret);

	return instance_iter;
}


/** Return all the elements in a set from a SetTrie identified by set_id as an iterator of strings.

	\param st_id  The st_id returned by a previous new_settrie() call.
	\param set_id A valid set_id returned by a successful next_set_id() call.

	\return		  0 on error or the empty set, or an iter_id > 0 that can be used to retrieve the result using
				  iterator_next()/iterator_size() and must be explicitly destroyed via destroy_iterator()
*/
int elements (int st_id, int set_id) {

	if (set_id == 0)
		return 0;

	SetTrieServer::iterator it = instance.find(st_id);

	if (it == instance.end())
		return 0;

	StringSet ret = it->second->elements(set_id);

	if (ret.size() == 0)
		return 0;

	iterator[++instance_iter] = new StringSet(ret);

	return instance_iter;
}


/** Return the integer set_id of the next set stored in a SetTrie after a given set_id to iterate over all the sets in the object.

	\param st_id  The st_id returned by a previous new_settrie() call.
	\param set_id A valid set_id returned by previous call or the constant -1 to return the first set. Note that 0 may be the set_id of
				  the empty set in case the empty set is in the SetTrie.

	\return		  A unique integer set_id that can be used for iterating, calling set_name() or elements(). On error, it will return -3
				  if the st_id or the set_id is invalid and -2 if the set_id given is the last set in the object or the object is empty.
*/
int next_set_id (int st_id, int set_id) {

	SetTrieServer::iterator it = instance.find(st_id);

	if (it == instance.end())
		return -3;

	if (set_id == -1) {
		if (it->second->id.size() == 0)
			return -2;

		IdMap::iterator jt = it->second->id.begin();

		return jt->first;
	}

	IdMap::iterator jt = it->second->id.find(set_id);

	if (jt == it->second->id.end())
		return -3;

	++jt;

	if (jt == it->second->id.end())
		return -2;

	return jt->first;
}


/** Return the number of sets in a SetTrie object.

	\param st_id  The st_id returned by a previous new_settrie() call.

	\return		  The number or -1 on invalid st_id.
*/
int num_sets (int st_id) {

	SetTrieServer::iterator it = instance.find(st_id);

	if (it == instance.end())
		return -1;

	return it->second->id.size();
}


/** Return the name (Python id) of a set stored in a SetTrie identified by its binary (int) set_id.

	\param st_id  The st_id returned by a previous new_settrie() call.
	\param set_id A valid set_id returned by a successful next_set_id() call.

	\return		  An empty string on any error and the Python id of the set if both st_id and set_id are valid.
*/
char *set_name (int st_id, int set_id) {

	SetTrieServer::iterator it = instance.find(st_id);

	char *p_ans = get_answer_buffer();

	p_ans[0] = 0;

	if (it != instance.end()) {
		IdMap::iterator jt = it->second->id.find(set_id);

		if (jt != it->second->id.end())
			strcpy(p_ans, jt->second.c_str());
	}

	return p_ans;
}


/** Removes a set from the object by its unique integer id.

	\param st_id  The st_id returned by a previous new_settrie() call.
	\param set_id A valid set_id returned by a successful next_set_id() call.

	\return		  0 on success or a negative error code.
*/
extern int remove (int st_id, int set_id) {

	SetTrieServer::iterator it = instance.find(st_id);

	if (it == instance.end())
		return -1;

	return it->second->remove(set_id);
}


/** Purges (reassigns node integer ids and frees RAM) after a series of remove() calls.

	\param st_id   The st_id returned by a previous new_settrie() call.
	\param dry_run If nonzero, does nothing and returns the number of dirty nodes.

	\return		   A positive num_dirty_nodes on dry_run, zero on successful completion or a negative error code.
*/
extern int purge (int st_id, int dry_run) {

	SetTrieServer::iterator it = instance.find(st_id);

	if (it == instance.end())
		return -1;

	if (dry_run)
		return it->second->num_dirty_nodes;

	return it->second->purge();
}


/** Return the number of unread items in an iterator (returned by subsets() or supersets()).

	\param iter_id  The iter_id returned by a previous subsets() or supersets() call.

	\return			The number of unread items in the iterator.
*/
int iterator_size (int iter_id) {

	IterServer::iterator it = iterator.find(iter_id);

	if (it == iterator.end())
		return 0;

	return it->second->size();
}


/** Return the first unread item in an iterator (returned by subsets() or supersets()).

	\param iter_id  The iter_id returned by a previous subsets() or supersets() call.

	\return			The str_id of an insert()-ed set that matches the query.
*/
char *iterator_next (int iter_id) {

	IterServer::iterator it = iterator.find(iter_id);

	char *p_ans = get_answer_buffer();

	p_ans[0] = 0;

	if (it != iterator.end() && !it->second->empty()) {

		String ss = it->second->back();
		it->second->pop_back();

		strcpy(p_ans, ss.c_str());
	}

	return p_ans;
}


/** Destroy an iterator (returned by subsets() or supersets()).

	\param iter_id  The iter_id returned by a previous subsets() or supersets() call.
*/
void destroy_iterator (int iter_id) {

	IterServer::iterator it = iterator.find(iter_id);

	if (it == iterator.end())
		return;

	delete it->second;

	iterator.erase(it);
}


/** Destroy an iterator for a binary image(returned by save_as_binary_image()).

	\param image_id  The image_id returned by a previous save_as_binary_image() call.
*/
void destroy_binary_image (int image_id) {

	BinaryImageServer::iterator it = image.find(image_id);

	if (it == image.end())
		return;

	delete it->second;

	image.erase(it);
}


/** Saves a SetTrie object as a BinaryImage.

	\param st_id  The st_id returned by a previous new_settrie() call.

	\return		  0 on error, or a binary_image_id > 0 which is the same as st_id and must be destroyed using destroy_binary_image()
*/
int save_as_binary_image (int st_id) {

	SetTrieServer::iterator it = instance.find(st_id);

	if (it == instance.end())
		return 0;

	pBinaryImage p_bi = new BinaryImage;

	if (!it->second->save(p_bi)) {
		delete p_bi;

		return 0;
	}

	destroy_binary_image(st_id);

	image[st_id] = p_bi;

	return st_id;
}


/** Pushes raw image blocks into an initially empty SetTrie object and finally creates it already populated with the binary image.

	\param st_id	The st_id returned by a previous new_settrie() call. The object must be empty (never inserted).
	\param p_block	If non-empty, a block in the right order making a binary image obtained for a previous save_as_binary_image() call.
					If empty, an order to .load() the SetTrie object and destroyed the previously stored blocks.

	\return			True on success.
*/
bool push_binary_image_block (int st_id, char *p_block) {

	SetTrieServer::iterator it_settrie = instance.find(st_id);

	if (it_settrie == instance.end())
		return false;

	if (p_block[0] == 0) {
		BinaryImageServer::iterator it_image = image.find(st_id);

		if (it_image == image.end())
			return false;

		bool ok = it_settrie->second->load(it_image->second);

		destroy_binary_image(st_id);

		return ok;
	}

	ImageBlock blk;

	if (!string_as_image_block(blk, p_block)) {
		destroy_binary_image(st_id);

		return false;
	}

	BinaryImageServer::iterator it_image = image.find(st_id);

	if (it_image == image.end()) {
		if (blk.block_num != 1)
			return false;

		pBinaryImage p_bi = new BinaryImage;

		p_bi->push_back(blk);

		image[st_id] = p_bi;

		return true;
	}

	ImageBlock *p_last = &it_image->second->back();

	if (blk.block_num != p_last->block_num + 1)
		return false;

	it_image->second->push_back(blk);

	return true;
}


/** Return the number of unread binary images blocks in an iterator returned by a save_as_binary_image() call.

	\param image_id	The image_id returned by a previous save_as_binary_image() call.

	\return			The number of unread blocks in the iterator.
*/
int binary_image_size (int image_id) {

	BinaryImageServer::iterator it = image.find(image_id);

	if (it == image.end())
		return 0;

	return it->second->size();
}


/** Return the first unread binary images block in an iterator returned by a save_as_binary_image() call.

	\param image_id	The image_id returned by a previous save_as_binary_image() call.

	\return			The the binary image block serialized as base64 or an empty string on failure.
*/
char *binary_image_next (int image_id) {

	BinaryImageServer::iterator it = image.find(image_id);

	if (it == image.end())
		return (char *) "";

	uint8_t *p_in = (uint8_t *) it->second->data();

	char *p_ret = image_block_as_string(p_in);

	it->second->erase(it->second->begin());

	return p_ret;
}


/** Free the memory allocated for the answer buffer.
*/
void cleanup_globals () {
	if (p_answer != nullptr) {
		free(p_answer);
		p_answer = nullptr;
	}
}

#if defined TEST

#ifndef INCLUDED_CATCH2
#define INCLUDED_CATCH2

#include "catch.hpp"

#endif

void compare_iterating(pSetTrie p1, pSetTrie p2, bool compare_int_id) {

	REQUIRE(p1->id.size() == p2->id.size());

	IdMap::iterator it1 = p1->id.begin();
	IdMap::iterator it2 = p2->id.begin();

	while (it1 != p1->id.end()) {
		REQUIRE(it2 != p2->id.end());
		if (compare_int_id) {
			REQUIRE(it1->first  == it2->first);
			REQUIRE(it1->second == it2->second);

			StringSet el1 = p1->elements(it1->first);
			StringSet el2 = p2->elements(it2->first);

			REQUIRE(el1.size() == el2.size());

			for (int i = 0; i < el1.size(); i++) {
				REQUIRE(el1[i] == el2[i]);
			}
		} else {
			IdMap::iterator it3 = p2->id.begin();

			while (it3 != p2->id.end() && it3->second != it1->second)
				++it3;

			REQUIRE(it1->second == it3->second);

			StringSet el1 = p1->elements(it1->first);
			StringSet el2 = p2->elements(it3->first);

			REQUIRE(el1.size() == el2.size());

			for (int i = 0; i < el1.size(); i++) {
				REQUIRE(el1[i] == el2[i]);
			}
		}
		++it1;
		++it2;
	}
	REQUIRE(it2 == p2->id.end());
}


void recurse_tree(pSetTrie ps, int ii, int &n_nodes, int &n_sets, int level) {
	REQUIRE(level > 0);
	REQUIRE(ii >= 0);
	REQUIRE(ii < ps->tree.size());

	int state = ps->tree[ii].state;

	REQUIRE(state >= 0);
	REQUIRE(state < STATE_IS_GARBAGE);

	if (state == STATE_HAS_SET_ID)
		n_sets--;

	n_nodes--;

	if (ps->tree[ii].idx_child != 0)
		recurse_tree(ps, ps->tree[ii].idx_child, n_nodes, n_sets, level - 1);

	if (ps->tree[ii].idx_next != 0)
		recurse_tree(ps, ps->tree[ii].idx_next, n_nodes, n_sets, level - 1);
}


void check_sets(pSetTrie ps) {
	for (IdMap::iterator it = ps->id.begin(); it != ps->id.end(); ++it) {
		REQUIRE(ps->tree[it->first].state == STATE_HAS_SET_ID);
	}
}


void remove_by_id(pSetTrie ps, char *pID) {

	IdMap::iterator it = ps->id.begin();

	while (it != ps->id.end()) {
		if (strcmp(pID, it->second.c_str()) == 0) {
			REQUIRE(ps->remove(it->first) == 0);

			int n_nodes = ps->tree.size() - ps->num_dirty_nodes;
			int n_sets	= ps->id.size();

			recurse_tree(ps, 0, n_nodes, n_sets, 8);

			REQUIRE(n_nodes == 0);
			REQUIRE(n_sets  == 0);

			check_sets(ps);

			return;
		}

		++it;
	}
	REQUIRE(false);
}


SCENARIO("Test remove() and purge().") {

	int all = new_settrie();
	int con = new_settrie();
	int vow = new_settrie();
	int non = new_settrie();
	int bak = new_settrie();

	REQUIRE(all > 0);
	REQUIRE(con > 0);
	REQUIRE(vow > 0);
	REQUIRE(non > 0);
	REQUIRE(bak > 0);

	pSetTrie p_all = instance[all];
	pSetTrie p_con = instance[con];
	pSetTrie p_vow = instance[vow];
	pSetTrie p_non = instance[non];
	pSetTrie p_bak = instance[bak];

	GIVEN("I load something to my objects.") {
		insert(all, (char *) "a",		  (char *) "s_00");
		insert(all, (char *) "a,b",		  (char *) "s_01");
		insert(all, (char *) "a,b,c",	  (char *) "s_02");
		insert(all, (char *) "a,b,c,d",	  (char *) "s_03");
		insert(all, (char *) "a,b,c,d,e", (char *) "s_04");
		insert(all, (char *) "a,c,d,e",	  (char *) "s_05");
		insert(all, (char *) "a,b,d,e",	  (char *) "s_06");
		insert(all, (char *) "a,b,c,e",	  (char *) "s_07");
		insert(all, (char *) "b",		  (char *) "s_08");
		insert(all, (char *) "b,c",		  (char *) "s_09");
		insert(all, (char *) "b,c,d",	  (char *) "s_10");
		insert(all, (char *) "b,c,d,e",	  (char *) "s_11");
		insert(all, (char *) "b,d,e",	  (char *) "s_12");
		insert(all, (char *) "b,c,e",	  (char *) "s_13");
		insert(all, (char *) "c",		  (char *) "s_14");
		insert(all, (char *) "c,d",		  (char *) "s_15");
		insert(all, (char *) "c,d,e",	  (char *) "s_16");
		insert(all, (char *) "c,e",		  (char *) "s_17");
		insert(all, (char *) "d",		  (char *) "s_18");
		insert(all, (char *) "d,e",		  (char *) "s_19");
		insert(all, (char *) "e",		  (char *) "s_20");
		insert(all, (char *) "a,e",		  (char *) "s_21");

		insert(con, (char *) "b",		  (char *) "s_08");
		insert(con, (char *) "b,c",		  (char *) "s_09");
		insert(con, (char *) "b,c,d",	  (char *) "s_10");
		insert(con, (char *) "c",		  (char *) "s_14");
		insert(con, (char *) "c,d",		  (char *) "s_15");
		insert(con, (char *) "d",		  (char *) "s_18");

		insert(vow, (char *) "a",		  (char *) "s_00");
		insert(vow, (char *) "e",		  (char *) "s_20");
		insert(vow, (char *) "a,e",		  (char *) "s_21");

		pBinaryImage p_bi = new BinaryImage;

		REQUIRE(p_all->save(p_bi));
		REQUIRE(p_bak->load(p_bi));

		delete p_bi;

		compare_iterating(p_all, p_bak, true);

		REQUIRE(p_all->tree.size() == p_bak->tree.size());
		REQUIRE(p_all->num_dirty_nodes == 0);

		remove_by_id(p_all, (char *) "s_00");
		remove_by_id(p_all, (char *) "s_01");
		remove_by_id(p_all, (char *) "s_02");
		remove_by_id(p_all, (char *) "s_03");
		remove_by_id(p_all, (char *) "s_04");
		remove_by_id(p_all, (char *) "s_05");
		remove_by_id(p_all, (char *) "s_06");
		remove_by_id(p_all, (char *) "s_07");
		remove_by_id(p_all, (char *) "s_11");
		remove_by_id(p_all, (char *) "s_12");
		remove_by_id(p_all, (char *) "s_13");
		remove_by_id(p_all, (char *) "s_16");
		remove_by_id(p_all, (char *) "s_17");
		remove_by_id(p_all, (char *) "s_19");
		remove_by_id(p_all, (char *) "s_20");
		remove_by_id(p_all, (char *) "s_21");

		REQUIRE(p_all->tree.size() == p_bak->tree.size());
		REQUIRE(p_all->num_dirty_nodes == 19);

		compare_iterating(p_all, p_con, false);

		REQUIRE(p_all->tree.size() == p_bak->tree.size());
		REQUIRE(p_all->tree.size() != p_con->tree.size());
		REQUIRE(p_all->tree.size() - p_con->tree.size() == 19);

		p_all->purge();

		REQUIRE(p_all->num_dirty_nodes == 0);

		int n_nodes = p_all->tree.size();
		int n_sets	= p_all->id.size();

		recurse_tree(p_all, 0, n_nodes, n_sets, 8);

		REQUIRE(n_nodes == 0);
		REQUIRE(n_sets  == 0);

		check_sets(p_all);

		compare_iterating(p_all, p_con, false);

		REQUIRE(p_all->tree.size() == p_con->tree.size());

		REQUIRE(p_bak->num_dirty_nodes == 0);

		remove_by_id(p_bak, (char *) "s_01");
		remove_by_id(p_bak, (char *) "s_02");
		remove_by_id(p_bak, (char *) "s_03");
		remove_by_id(p_bak, (char *) "s_04");
		remove_by_id(p_bak, (char *) "s_05");
		remove_by_id(p_bak, (char *) "s_06");
		remove_by_id(p_bak, (char *) "s_07");
		remove_by_id(p_bak, (char *) "s_08");
		remove_by_id(p_bak, (char *) "s_09");
		remove_by_id(p_bak, (char *) "s_10");
		remove_by_id(p_bak, (char *) "s_11");
		remove_by_id(p_bak, (char *) "s_12");
		remove_by_id(p_bak, (char *) "s_13");
		remove_by_id(p_bak, (char *) "s_14");
		remove_by_id(p_bak, (char *) "s_15");
		remove_by_id(p_bak, (char *) "s_16");
		remove_by_id(p_bak, (char *) "s_17");
		remove_by_id(p_bak, (char *) "s_18");
		remove_by_id(p_bak, (char *) "s_19");

		REQUIRE(p_bak->num_dirty_nodes == 22);

		compare_iterating(p_bak, p_vow, false);

		REQUIRE(p_bak->tree.size() - p_vow->tree.size() == 22);

		p_bak->purge();

		REQUIRE(p_bak->num_dirty_nodes == 0);

		n_nodes = p_bak->tree.size();
		n_sets	= p_bak->id.size();

		recurse_tree(p_bak, 0, n_nodes, n_sets, 8);

		REQUIRE(n_nodes == 0);
		REQUIRE(n_sets  == 0);

		check_sets(p_bak);

		compare_iterating(p_bak, p_vow, true);

		REQUIRE(p_bak->tree.size() == p_vow->tree.size());

		remove_by_id(p_all, (char *) "s_08");
		remove_by_id(p_all, (char *) "s_09");
		remove_by_id(p_all, (char *) "s_10");
		remove_by_id(p_all, (char *) "s_14");
		remove_by_id(p_all, (char *) "s_15");
		remove_by_id(p_all, (char *) "s_18");

		REQUIRE(p_all->tree.size() == p_con->tree.size());
		REQUIRE(p_all->num_dirty_nodes == 6);

		compare_iterating(p_all, p_non, true);

		REQUIRE(p_all->tree.size() == p_con->tree.size());
		REQUIRE(p_all->tree.size() != p_non->tree.size());
		REQUIRE(p_all->tree.size() - p_non->tree.size() == 6);

		p_all->purge();

		REQUIRE(p_all->num_dirty_nodes == 0);

		n_nodes = p_all->tree.size();
		n_sets	= p_all->id.size();

		recurse_tree(p_all, 0, n_nodes, n_sets, 8);

		REQUIRE(n_nodes == 0);
		REQUIRE(n_sets  == 0);

		check_sets(p_all);

		compare_iterating(p_all, p_non, true);

		REQUIRE(p_all->tree.size() == p_non->tree.size());
		REQUIRE(p_all->tree.size() == 1);
		REQUIRE(p_all->hh_nam.size() == 0);

		remove_by_id(p_bak, (char *) "s_00");
		remove_by_id(p_bak, (char *) "s_20");
		remove_by_id(p_bak, (char *) "s_21");

		REQUIRE(p_bak->tree.size() == p_vow->tree.size());
		REQUIRE(p_bak->num_dirty_nodes == 3);

		compare_iterating(p_bak, p_non, true);

		REQUIRE(p_bak->tree.size() == p_vow->tree.size());
		REQUIRE(p_bak->tree.size() != p_non->tree.size());
		REQUIRE(p_bak->tree.size() - p_non->tree.size() == 3);

		p_bak->purge();

		REQUIRE(p_bak->num_dirty_nodes == 0);

		n_nodes = p_bak->tree.size();
		n_sets	= p_bak->id.size();

		recurse_tree(p_bak, 0, n_nodes, n_sets, 8);

		REQUIRE(n_nodes == 0);
		REQUIRE(n_sets  == 0);

		check_sets(p_bak);

		compare_iterating(p_bak, p_non, true);

		REQUIRE(p_bak->tree.size() == p_non->tree.size());
		REQUIRE(p_bak->tree.size() == 1);
		REQUIRE(p_bak->hh_nam.size() == 0);

		insert(all, (char *) "a",	  (char *) "s_00");
		insert(all, (char *) "a,b,c", (char *) "s_02");

		REQUIRE(p_all->id.size() == 2);
		REQUIRE(p_all->hh_nam.size() == 3);
		REQUIRE(p_all->tree[0].state == STATE_IN_USE);

		insert(all, (char *) "", (char *) "void");

		REQUIRE(p_all->id.size() == 3);
		REQUIRE(p_all->hh_nam.size() == 4);
		REQUIRE(p_all->tree[0].state == STATE_HAS_SET_ID);

		remove_by_id(p_all, (char *) "s_02");

		REQUIRE(p_all->hh_nam.size() == 2);

		REQUIRE(p_all->remove(-1) == -2);
		REQUIRE(p_all->remove(50) == -2);
		REQUIRE(p_all->remove(0) == 0);

		REQUIRE(p_all->id.size() == 1);
		REQUIRE(p_all->hh_nam.size() == 1);
		REQUIRE(p_all->tree[0].state == STATE_IN_USE);

		REQUIRE(p_all->remove(0) == -2);

		REQUIRE(p_all->purge() == 0);
		REQUIRE(p_all->purge() == -1);

		insert(all, (char *) "a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,r,s,t,u,v,w", (char *) "x");
		REQUIRE(p_all->tree.size() == 24);
		REQUIRE(p_all->tree[20].state == STATE_IN_USE);
		REQUIRE(p_all->remove(20) == -2);
		p_all->tree[20].state = STATE_HAS_SET_ID;
		REQUIRE(p_all->remove(20) == -3);
	}
	destroy_settrie(bak);
	destroy_settrie(non);
	destroy_settrie(vow);
	destroy_settrie(con);
	destroy_settrie(all);
}


SCENARIO("Test image_block_as_string() / string_as_image_block()") {

	REQUIRE(sizeof(ImageBlock) == 6*1024);

	ImageBlock block;

	block.size = IMAGE_BUFF_SIZE;
	for (int i = 0; i < IMAGE_BUFF_SIZE; i++)
		block.buffer[i] = i & 0xff;

	block.block_num = 0;

	char *p_str = image_block_as_string(block);

	REQUIRE(strlen(p_str) == 8*1024);

	// 00000001 00000010 00000011 00000100 00000101 00000110 00000111

	// 0         1         2         3         4         5         6
	// 0123456789012345678901234567890123456789012345678901234567890123
	// ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/

	REQUIRE(p_str[12] == 'A');		// 000000
	REQUIRE(p_str[13] == 'Q');		// 010000
	REQUIRE(p_str[14] == 'I');		// 001000
	REQUIRE(p_str[15] == 'D');		// 000011
	REQUIRE(p_str[16] == 'B');		// 000001
	REQUIRE(p_str[17] == 'A');		// 000000
	REQUIRE(p_str[18] == 'U');		// 010100
	REQUIRE(p_str[19] == 'G');		// 000110
	REQUIRE(p_str[20] == 'B');		// 000001

	REQUIRE(p_str[8191] == b64chars[6135 & 0x3f]);
	REQUIRE(p_str[8192] == 0);

	ImageBlock block2;

	REQUIRE(string_as_image_block(block2, p_str));

	for (int ofs = 0; ofs < 8; ofs++) {
		block.size = IMAGE_BUFF_SIZE;
		for (int i = 0; i < IMAGE_BUFF_SIZE; i++)
			block.buffer[i] = (i + ofs) & 0xff;

		block.block_num = 8 - ofs;

		p_str = image_block_as_string(block);

		REQUIRE(strlen(p_str) == 8*1024);

		REQUIRE(string_as_image_block(block2, p_str));

		REQUIRE(block.size == block2.size);
		REQUIRE(block.block_num == block2.block_num);
		REQUIRE(MurmurHash64A(&block, sizeof(ImageBlock)) == MurmurHash64A(&block, sizeof(ImageBlock)));
	}
}


SCENARIO("Test image_put() / image_get()") {

	uint8_t buffer1[4*IMAGE_BUFF_SIZE];
	int		buffer4[IMAGE_BUFF_SIZE];

	memset(buffer1, 0xcafebebe, 4*IMAGE_BUFF_SIZE);

	for (int i = 0; i < IMAGE_BUFF_SIZE; i++)
		buffer4[i] = 2*i + 11;

	pBinaryImage p_bi = new BinaryImage;

	REQUIRE(image_put(p_bi, &buffer1, IMAGE_BUFF_SIZE));
	REQUIRE(image_put(p_bi, &buffer1, IMAGE_BUFF_SIZE - 1));
	REQUIRE(image_put(p_bi, &buffer1, IMAGE_BUFF_SIZE + 2));
	REQUIRE(image_put(p_bi, &buffer1, IMAGE_BUFF_SIZE - 1));

	REQUIRE(image_put(p_bi, &buffer4,  4));
	REQUIRE(image_put(p_bi, &buffer4,  8));
	REQUIRE(image_put(p_bi, &buffer4, 16));
	REQUIRE(image_put(p_bi, &buffer4, 32));

	REQUIRE(image_put(p_bi, &buffer4, 3*IMAGE_BUFF_SIZE));

	for (int i = 0; i < 8192; i++) {
		image_put(p_bi, &i, 4);
		image_put(p_bi, &i, 3);
		image_put(p_bi, &i, 2);
		image_put(p_bi, &i, 1);
	}

	for (int i = 0; i < p_bi->size(); i++)
		REQUIRE((*p_bi)[i].block_num == i + 1);

	uint8_t i_buffer1[4*IMAGE_BUFF_SIZE];
	int		i_buffer4[IMAGE_BUFF_SIZE];

	for (int t = 0; t < 3; t++) {
		int c_block = 0;
		int c_ofs	= 0;

		REQUIRE(image_get(p_bi, c_block, c_ofs, &i_buffer1, IMAGE_BUFF_SIZE));
		REQUIRE(!memcmp(&buffer1, &i_buffer1, IMAGE_BUFF_SIZE));

		REQUIRE(image_get(p_bi, c_block, c_ofs, &i_buffer1, IMAGE_BUFF_SIZE - 1));
		REQUIRE(!memcmp(&buffer1, &i_buffer1, IMAGE_BUFF_SIZE - 1));

		REQUIRE(image_get(p_bi, c_block, c_ofs, &i_buffer1, IMAGE_BUFF_SIZE + 2));
		REQUIRE(!memcmp(&buffer1, &i_buffer1, IMAGE_BUFF_SIZE + 2));

		REQUIRE(image_get(p_bi, c_block, c_ofs, &i_buffer1, IMAGE_BUFF_SIZE - 1));
		REQUIRE(!memcmp(&buffer1, &i_buffer1, IMAGE_BUFF_SIZE - 1));

		REQUIRE(image_get(p_bi, c_block, c_ofs, &i_buffer4,  4));
		REQUIRE(!memcmp(&buffer4, &i_buffer4,  4));

		REQUIRE(image_get(p_bi, c_block, c_ofs, &i_buffer4,  8));
		REQUIRE(!memcmp(&buffer4, &i_buffer4,  8));

		REQUIRE(image_get(p_bi, c_block, c_ofs, &i_buffer4, 16));
		REQUIRE(!memcmp(&buffer4, &i_buffer4, 16));

		REQUIRE(image_get(p_bi, c_block, c_ofs, &i_buffer4, 32));
		REQUIRE(!memcmp(&buffer4, &i_buffer4, 32));

		REQUIRE(image_get(p_bi, c_block, c_ofs, &i_buffer4, 3*IMAGE_BUFF_SIZE));
		REQUIRE(!memcmp(&buffer4, &i_buffer4, 3*IMAGE_BUFF_SIZE));

		bool failed = false;
		for (int i = 0; i < 8192; i++) {
			int i_i;

			image_get(p_bi, c_block, c_ofs, &i_i, 4);
			if (i != i_i)
				failed = true;
			image_get(p_bi, c_block, c_ofs, &i_i, 3);
			if ((i & 0xffffff) != (i_i & 0xffffff))
				failed = true;
			image_get(p_bi, c_block, c_ofs, &i_i, 2);
			if ((i & 0xffff) != (i_i & 0xffff))
				failed = true;
			image_get(p_bi, c_block, c_ofs, &i_i, 1);
			if ((i & 0xff) != (i_i & 0xff))
				failed = true;

			if (failed)
				break;
		}
		REQUIRE(!failed);
	}

	delete p_bi;
}


SCENARIO("Test SetTrie.save() / SetTrie.load()") {

	SetTrie A;

	String key1 = {"no_1"};
	String val1 = {"a b"};

	A.insert(val1, key1, ' ');

	String key0 = {"empty"};
	String val0 = {""};

	A.insert(val0, key0, ',');

	key1 = {"no_2"};
	val1 = {"a c d"};

	A.insert(val1, key1, ' ');

	key1 = {"yes_1"};
	val1 = {"a c e"};

	A.insert(val1, key1, ' ');

	key1 = {"yes_2"};
	val1 = {"a c d e"};

	A.insert(val1, key1, ' ');

	key1 = {"no_3"};
	val1 = {"b d"};

	A.insert(val1, key1, ' ');

	key1 = {"no_4"};
	val1 = {"c d"};

	A.insert(val1, key1, ' ');

	key1 = {"yes_3"};
	val1 = {"c e"};

	A.insert(val1, key1, ' ');

	REQUIRE(A.supersets("c e", ' ').size()	== 3);
	REQUIRE(A.supersets("a", ' ').size()	== 4);
	REQUIRE(A.supersets("c", ' ').size()	== 5);
	REQUIRE(A.supersets("b d", ' ').size()	== 1);

	SetTrie B;

	REQUIRE(B.supersets("c e", ' ').size()	== 0);
	REQUIRE(B.supersets("a", ' ').size()	== 0);
	REQUIRE(B.supersets("c", ' ').size()	== 0);
	REQUIRE(B.supersets("b d", ' ').size()	== 0);

	pBinaryImage p_bi = new BinaryImage;

	REQUIRE(A.save(p_bi));

	REQUIRE(B.load(p_bi));

	delete p_bi;

	REQUIRE(B.supersets("c e", ' ').size()	== 3);
	REQUIRE(B.supersets("a", ' ').size()	== 4);
	REQUIRE(B.supersets("c", ' ').size()	== 5);
	REQUIRE(B.supersets("b d", ' ').size()	== 1);
}


SCENARIO("Test save_as_binary_image() / push_binary_image_block(), etc.") {

	int a = new_settrie();
	int b = new_settrie();

	REQUIRE(a > 0);
	REQUIRE(b > 0);

	GIVEN("I load something to use supersets()") {
		insert(a, (char *) "a,b",				(char *) "sup01");
		insert(a, (char *) "a,c,d",				(char *) "sup02");
		insert(a, (char *) "a,c,e",				(char *) "sup03");
		insert(a, (char *) "a,c,d,e",			(char *) "sup04");
		insert(a, (char *) "b,d",				(char *) "sup05");
		insert(a, (char *) "c,d",				(char *) "sup06");
		insert(a, (char *) "c,e",				(char *) "sup07");
		insert(a, (char *) "c,d,e,f",			(char *) "sup08");
		insert(a, (char *) "c,d,n",				(char *) "sup09");
		insert(a, (char *) "c,d,e,f,x",			(char *) "sup10");
		insert(a, (char *) "c,d,e,f,y",			(char *) "sup11");
		insert(a, (char *) "c,d,e,f,y,z",		(char *) "sup12");
		insert(a, (char *) "d,e",				(char *) "sup13");
		insert(a, (char *) "e,a,b,d,f,n,x,y,z", (char *) "sup14");
		insert(a, (char *) "c",					(char *) "sup15");
		insert(a, (char *) "e,y,z,c",			(char *) "sup16");

		char buffer[64], name[64];

		for (int i = 0; i < 8192; i++) {
			sprintf(buffer, "monster,knot%u,node%u", 3*i, i*i);
			sprintf(name, "document%u", i);
			insert(a, buffer, name);
		}

		WHEN("I supersets() the original") {
			int q1 = supersets(a, (char *) "c,e");
			int q2 = supersets(a, (char *) "y,z");
			int q3 = supersets(a, (char *) "d,e");
			int q4 = supersets(a, (char *) "monster");

			REQUIRE(iterator_size(q1) == 8);
			REQUIRE(iterator_size(q2) == 3);
			REQUIRE(iterator_size(q3) == 7);
			REQUIRE(iterator_size(q4) == 8192);

			int mask = 0;
			String s;

			s = iterator_next(q1);REQUIRE(s != "");
			s = iterator_next(q2);
			if (s == "sup12")
				mask |= 1;
			if (s == "sup14")
				mask |= 2;
			if (s == "sup16")
				mask |= 4;
			s = iterator_next(q3);REQUIRE(s != "");

			s = iterator_next(q1);REQUIRE(s != "");
			s = iterator_next(q2);
			if (s == "sup12")
				mask |= 1;
			if (s == "sup14")
				mask |= 2;
			if (s == "sup16")
				mask |= 4;
			s = iterator_next(q3);REQUIRE(s != "");

			s = iterator_next(q1);REQUIRE(s != "");
			s = iterator_next(q2);
			if (s == "sup12")
				mask |= 1;
			if (s == "sup14")
				mask |= 2;
			if (s == "sup16")
				mask |= 4;
			s = iterator_next(q3);REQUIRE(s != "");

			s = iterator_next(q1);REQUIRE(s != "");
			s = iterator_next(q2);REQUIRE(s == "");
			s = iterator_next(q3);REQUIRE(s != "");

			s = iterator_next(q1);REQUIRE(s != "");
			s = iterator_next(q2);REQUIRE(s == "");
			s = iterator_next(q3);REQUIRE(s != "");

			s = iterator_next(q1);REQUIRE(s != "");
			s = iterator_next(q2);REQUIRE(s == "");
			s = iterator_next(q3);REQUIRE(s != "");

			s = iterator_next(q1);REQUIRE(s != "");
			s = iterator_next(q2);REQUIRE(s == "");
			s = iterator_next(q3);REQUIRE(s != "");

			s = iterator_next(q1);REQUIRE(s != "");
			s = iterator_next(q2);REQUIRE(s == "");
			s = iterator_next(q3);REQUIRE(s == "");

			s = iterator_next(q1);REQUIRE(s == "");
			s = iterator_next(q2);REQUIRE(s == "");
			s = iterator_next(q3);REQUIRE(s == "");

			THEN("I get the expected keys") {
				REQUIRE(mask == 7);
			}

			REQUIRE(iterator_size(q1) == 0);
			REQUIRE(iterator_size(q2) == 0);
			REQUIRE(iterator_size(q3) == 0);

			destroy_iterator(q1);
			destroy_iterator(q2);
			destroy_iterator(q3);
			destroy_iterator(q4);
		}
		WHEN("I supersets() the other object") {
			int q1 = supersets(b, (char *) "c,e");
			int q2 = supersets(b, (char *) "y,z");
			int q3 = supersets(b, (char *) "d,e");

			REQUIRE(q1 == 0);
			REQUIRE(q2 == 0);
			REQUIRE(q3 == 0);
		}
		WHEN("I copy the first object into the second") {
			int i_a = save_as_binary_image(a);

			REQUIRE(i_a == a);

			int i_a_size = binary_image_size(i_a);

			REQUIRE(i_a_size > 0);

			for (int i = 0; i < i_a_size; i++) {
				char *p_base64 = binary_image_next(i_a);

				REQUIRE(strlen(p_base64) == 8192);

				REQUIRE(push_binary_image_block(b, p_base64));
			}
			REQUIRE(binary_image_size(i_a) == 0);
			destroy_binary_image(i_a);

			REQUIRE(push_binary_image_block(b, (char *) ""));

			THEN("I supersets() the other object") {
				int q1 = supersets(b, (char *) "c,e");
				int q2 = supersets(b, (char *) "y,z");
				int q3 = supersets(b, (char *) "d,e");
				int q4 = supersets(a, (char *) "monster");

				REQUIRE(iterator_size(q1) == 8);
				REQUIRE(iterator_size(q2) == 3);
				REQUIRE(iterator_size(q3) == 7);
				REQUIRE(iterator_size(q4) == 8192);

				int mask = 0;
				String s;

				s = iterator_next(q1);REQUIRE(s != "");
				s = iterator_next(q2);
				if (s == "sup12")
					mask |= 1;
				if (s == "sup14")
					mask |= 2;
				if (s == "sup16")
					mask |= 4;
				s = iterator_next(q3);REQUIRE(s != "");

				s = iterator_next(q1);REQUIRE(s != "");
				s = iterator_next(q2);
				if (s == "sup12")
					mask |= 1;
				if (s == "sup14")
					mask |= 2;
				if (s == "sup16")
					mask |= 4;
				s = iterator_next(q3);REQUIRE(s != "");

				s = iterator_next(q1);REQUIRE(s != "");
				s = iterator_next(q2);
				if (s == "sup12")
					mask |= 1;
				if (s == "sup14")
					mask |= 2;
				if (s == "sup16")
					mask |= 4;
				s = iterator_next(q3);REQUIRE(s != "");

				s = iterator_next(q1);REQUIRE(s != "");
				s = iterator_next(q2);REQUIRE(s == "");
				s = iterator_next(q3);REQUIRE(s != "");

				s = iterator_next(q1);REQUIRE(s != "");
				s = iterator_next(q2);REQUIRE(s == "");
				s = iterator_next(q3);REQUIRE(s != "");

				s = iterator_next(q1);REQUIRE(s != "");
				s = iterator_next(q2);REQUIRE(s == "");
				s = iterator_next(q3);REQUIRE(s != "");

				s = iterator_next(q1);REQUIRE(s != "");
				s = iterator_next(q2);REQUIRE(s == "");
				s = iterator_next(q3);REQUIRE(s != "");

				s = iterator_next(q1);REQUIRE(s != "");
				s = iterator_next(q2);REQUIRE(s == "");
				s = iterator_next(q3);REQUIRE(s == "");

				s = iterator_next(q1);REQUIRE(s == "");
				s = iterator_next(q2);REQUIRE(s == "");
				s = iterator_next(q3);REQUIRE(s == "");

				REQUIRE(mask == 7);

				REQUIRE(iterator_size(q1) == 0);
				REQUIRE(iterator_size(q2) == 0);
				REQUIRE(iterator_size(q3) == 0);

				destroy_iterator(q1);
				destroy_iterator(q2);
				destroy_iterator(q3);
				destroy_iterator(q4);
			}
		}
	}
	destroy_settrie(b);
	destroy_settrie(a);
}


SCENARIO("Test python_set_as_string()") {

	String s;

	// Special case: This is how python serializes the empty set.

	s = python_set_as_string((char *) "set()");		  REQUIRE(s == "");
	s = python_set_as_string((char *) "frozenset()"); REQUIRE(s == "");

	// Fall back to doing nothing when not a set.

	s = python_set_as_string((char *) "a, b,c, ',', \",aa\"");	REQUIRE(s == "a, b,c, ',', \",aa\"");
	s = python_set_as_string((char *) "{a, b,c, ',', \",aa\"");	REQUIRE(s == "{a, b,c, ',', \",aa\"");
	s = python_set_as_string((char *) "a, b,c, ',', \",aa\"}");	REQUIRE(s == "a, b,c, ',', \",aa\"}");

	// Remove space after comma.

	s = python_set_as_string((char *) "{}");			 REQUIRE(s == "");
	s = python_set_as_string((char *) "{a}");			 REQUIRE(s == "a");
	s = python_set_as_string((char *) "{a }");			 REQUIRE(s == "a ");
	s = python_set_as_string((char *) "{a, b}");		 REQUIRE(s == "a,b");
	s = python_set_as_string((char *) "{1,2,345}");		 REQUIRE(s == "1,2,345");
	s = python_set_as_string((char *) "{1, 2, 345}");	 REQUIRE(s == "1,2,345");
	s = python_set_as_string((char *) "{1,  2, 345}");	 REQUIRE(s == "1,2,345");
	s = python_set_as_string((char *) "{1,  2, '345'}"); REQUIRE(s == "1,2,'345'");

	s = python_set_as_string((char *) "frozenset({})");				REQUIRE(s == "");
	s = python_set_as_string((char *) "frozenset({a})");			REQUIRE(s == "a");
	s = python_set_as_string((char *) "frozenset({a })");			REQUIRE(s == "a ");
	s = python_set_as_string((char *) "frozenset({a, b})");			REQUIRE(s == "a,b");
	s = python_set_as_string((char *) "frozenset({1,2,345})");		REQUIRE(s == "1,2,345");
	s = python_set_as_string((char *) "frozenset({1, 2, 345})");	REQUIRE(s == "1,2,345");
	s = python_set_as_string((char *) "frozenset({1,  2, 345})");	REQUIRE(s == "1,2,345");
	s = python_set_as_string((char *) "frozenset({1,  2, '345'})");	REQUIRE(s == "1,2,'345'");

	// Replace commas inside quotes by \x82.

	s = python_set_as_string((char *) "{1, 'two', 'three'}");		 REQUIRE(s == "1,'two','three'");
	s = python_set_as_string((char *) "{1, 'two,three', 'four'}");	 REQUIRE(s == "1,'two\x82three','four'");
	s = python_set_as_string((char *) "{1, \"two,three\", 'four'}"); REQUIRE(s == "1,\"two\x82three\",'four'");

	s = python_set_as_string((char *) "frozenset({1, 'two', 'three'})");		REQUIRE(s == "1,'two','three'");
	s = python_set_as_string((char *) "frozenset({1, 'two,three', 'four'})");	REQUIRE(s == "1,'two\x82three','four'");
	s = python_set_as_string((char *) "frozenset({1, \"two,three\", 'four'})");	REQUIRE(s == "1,\"two\x82three\",'four'");

	s = python_set_as_string((char *) "{1, '8', 'six, seven', 10, 555, 44, \"El'even, O'Toole\", 'three', 9999, '\"Dirty\" Harry, 2'}");

	REQUIRE(s == "1,'8','six\x82 seven',10,555,44,\"El'even\x82 O'Toole\",'three',9999,'\"Dirty\" Harry\x82 2'");

	s = python_set_as_string((char *) "frozenset({'2, 3', 1, 'its', 'this', \"it's\"})"); REQUIRE(s == "'2\x82 3',1,'its','this',\"it's\"");
}


SCENARIO("Test the Python interface parts") {

	int st1 = new_settrie();
	int st2 = new_settrie();
	int st3 = new_settrie();

	GIVEN("I load something to use find()") {
		insert(st1, (char *) "set()",	  (char *) "empty_set");
		insert(st1, (char *) "1,3,5",	  (char *) "find1");
		insert(st1, (char *) "1,2,3,4,5", (char *) "find2");
		insert(st1, (char *) "1,2,x",	  (char *) "find3");

		WHEN("I find() it") {
			String f1 = find(st1, (char *) "1,2,x");
			String f2 = find(st1, (char *) "1,2,y");
			String f3 = find(st1, (char *) "set()");

			THEN("I get the expected keys") {
				REQUIRE(f1 == "find3");
				REQUIRE(f2 == "");
				REQUIRE(f3 == "empty_set");
			}
		}
	}

	GIVEN("I load something to use supersets()") {
		insert(st2, (char *) "a,b",				  (char *) "sup01");
		insert(st2, (char *) "a,c,d",			  (char *) "sup02");
		insert(st2, (char *) "a,c,e",			  (char *) "sup03");
		insert(st2, (char *) "a,c,d,e",			  (char *) "sup04");
		insert(st2, (char *) "b,d",				  (char *) "sup05");
		insert(st2, (char *) "c,d",				  (char *) "sup06");
		insert(st2, (char *) "c,e",				  (char *) "sup07");
		insert(st2, (char *) "c,d,e,f",			  (char *) "sup08");
		insert(st2, (char *) "c,d,n",			  (char *) "sup09");
		insert(st2, (char *) "c,d,e,f,x",		  (char *) "sup10");
		insert(st2, (char *) "c,d,e,f,y",		  (char *) "sup11");
		insert(st2, (char *) "c,d,e,f,y,z",		  (char *) "sup12");
		insert(st2, (char *) "d,e",				  (char *) "sup13");
		insert(st2, (char *) "e,a,b,d,f,n,x,y,z", (char *) "sup14");
		insert(st2, (char *) "c",				  (char *) "sup15");
		insert(st2, (char *) "e,y,z,c",			  (char *) "sup16");

		WHEN("I supersets() it") {
			int q1 = supersets(st2, (char *) "c,e");
			int q2 = supersets(st2, (char *) "y,z");
			int q3 = supersets(st2, (char *) "d,e");

			REQUIRE(iterator_size(q1) == 8);
			REQUIRE(iterator_size(q2) == 3);
			REQUIRE(iterator_size(q3) == 7);

			int mask = 0;
			String s;

			s = iterator_next(q1);REQUIRE(s != "");
			s = iterator_next(q2);
			if (s == "sup12")
				mask |= 1;
			if (s == "sup14")
				mask |= 2;
			if (s == "sup16")
				mask |= 4;
			s = iterator_next(q3);REQUIRE(s != "");

			s = iterator_next(q1);REQUIRE(s != "");
			s = iterator_next(q2);
			if (s == "sup12")
				mask |= 1;
			if (s == "sup14")
				mask |= 2;
			if (s == "sup16")
				mask |= 4;
			s = iterator_next(q3);REQUIRE(s != "");

			s = iterator_next(q1);REQUIRE(s != "");
			s = iterator_next(q2);
			if (s == "sup12")
				mask |= 1;
			if (s == "sup14")
				mask |= 2;
			if (s == "sup16")
				mask |= 4;
			s = iterator_next(q3);REQUIRE(s != "");

			s = iterator_next(q1);REQUIRE(s != "");
			s = iterator_next(q2);REQUIRE(s == "");
			s = iterator_next(q3);REQUIRE(s != "");

			s = iterator_next(q1);REQUIRE(s != "");
			s = iterator_next(q2);REQUIRE(s == "");
			s = iterator_next(q3);REQUIRE(s != "");

			s = iterator_next(q1);REQUIRE(s != "");
			s = iterator_next(q2);REQUIRE(s == "");
			s = iterator_next(q3);REQUIRE(s != "");

			s = iterator_next(q1);REQUIRE(s != "");
			s = iterator_next(q2);REQUIRE(s == "");
			s = iterator_next(q3);REQUIRE(s != "");

			s = iterator_next(q1);REQUIRE(s != "");
			s = iterator_next(q2);REQUIRE(s == "");
			s = iterator_next(q3);REQUIRE(s == "");

			s = iterator_next(q1);REQUIRE(s == "");
			s = iterator_next(q2);REQUIRE(s == "");
			s = iterator_next(q3);REQUIRE(s == "");

			THEN("I get the expected keys") {
				REQUIRE(mask == 7);
			}

			REQUIRE(iterator_size(q1) == 0);
			REQUIRE(iterator_size(q2) == 0);
			REQUIRE(iterator_size(q3) == 0);

			destroy_iterator(q1);
			destroy_iterator(q2);
			destroy_iterator(q3);
		}

		GIVEN("I load something to use subsets()") {
			insert(st3, (char *) "1,3,5",	  (char *) "sub1");
			insert(st3, (char *) "1,2,3,4,5", (char *) "sub2");
			insert(st3, (char *) "1,2,x",	  (char *) "sub3");
			insert(st3, (char *) "1,x",		  (char *) "sub4");
			insert(st3, (char *) "1,y",		  (char *) "sub5");

			WHEN("I find() it") {
				int q1 = subsets(st3, (char *) "1,2,3,4,5,x,y");
				int q2 = subsets(st3, (char *) "1,2,x,y");
				int q3 = subsets(st3, (char *) "1,x,y");

				REQUIRE(iterator_size(q1) == 5);
				REQUIRE(iterator_size(q2) == 3);
				REQUIRE(iterator_size(q3) == 2);

				int mask = 0;
				String s;

				s = iterator_next(q1);REQUIRE(s != "");
				s = iterator_next(q2);
				if (s == "sub3")
					mask |= 1;
				if (s == "sub4")
					mask |= 2;
				if (s == "sub5")
					mask |= 4;
				s = iterator_next(q3);REQUIRE(s != "");

				s = iterator_next(q1);REQUIRE(s != "");
				s = iterator_next(q2);
				if (s == "sub3")
					mask |= 1;
				if (s == "sub4")
					mask |= 2;
				if (s == "sub5")
					mask |= 4;
				s = iterator_next(q3);REQUIRE(s != "");

				s = iterator_next(q1);REQUIRE(s != "");
				s = iterator_next(q2);
				if (s == "sub3")
					mask |= 1;
				if (s == "sub4")
					mask |= 2;
				if (s == "sub5")
					mask |= 4;
				s = iterator_next(q3);REQUIRE(s == "");

				s = iterator_next(q1);REQUIRE(s != "");
				s = iterator_next(q2);REQUIRE(s == "");
				s = iterator_next(q3);REQUIRE(s == "");

				s = iterator_next(q1);REQUIRE(s != "");
				s = iterator_next(q2);REQUIRE(s == "");
				s = iterator_next(q3);REQUIRE(s == "");

				s = iterator_next(q1);REQUIRE(s == "");
				s = iterator_next(q2);REQUIRE(s == "");
				s = iterator_next(q3);REQUIRE(s == "");

				THEN("I get the expected keys") {
					REQUIRE(mask == 7);
				}

				REQUIRE(iterator_size(q1) == 0);
				REQUIRE(iterator_size(q2) == 0);
				REQUIRE(iterator_size(q3) == 0);

				destroy_iterator(q1);
				destroy_iterator(q2);
				destroy_iterator(q3);
			}
		}
	}
	destroy_settrie(st1);
	destroy_settrie(st2);
	destroy_settrie(st3);
}


SCENARIO("Small Insert/find test") {

	SetTrie ST;

	BinarySet b_set = {3, 5, 7};

	REQUIRE(ST.find(b_set) == 0);

	String key0 = {"empty"};
	String val0 = {""};

	REQUIRE(strcmp(ST.find(val0, ',').c_str(), "") == 0);

	ST.insert(val0, key0, ',');

	REQUIRE(strcmp(ST.find(val0, ',').c_str(), "empty") == 0);

	String key1 = {"ky1"};
	String val1 = {"1 3 5"};

	ST.insert(val1, key1, ' ');

	REQUIRE(strcmp(ST.find(val1, ' ').c_str(), key1.c_str()) == 0);

	String key2 = {"ky1"};
	String val2 = {"1 2 3 4 5"};

	ST.insert(val2, key2, ' ');

	REQUIRE(strcmp(ST.find(val2, ' ').c_str(), key2.c_str()) == 0);
	REQUIRE(strcmp(ST.find(val1, ' ').c_str(), key1.c_str()) == 0);

	val1.clear();

	ST.insert(val1, key1, ' ');

	val2 = "1 2 x";

	REQUIRE(ST.find(val2, ' ') == "");

	val2 = "1 2 3 4";

	REQUIRE(ST.find(val2, ' ') == "");

	val2 = "";

	REQUIRE(ST.find(val2, ' ') == "ky1");
}


SCENARIO("Small Insert/supersets test") {

	SetTrie ST;

	ST.query = {3, 5, 7};
	ST.last_query_idx = ST.query.size() - 1;
	ST.result.clear();
	ST.supersets(ST.tree[0].idx_child, 0);

	REQUIRE(ST.result.size() == 0);

	ST.supersets(0, 0);

	REQUIRE(ST.result.size() == 0);

	REQUIRE(ST.supersets("c e", ' ').size() == 0);
	REQUIRE(ST.supersets("", ' ').size()	== 0);

	String key1 = {"no_1"};
	String val1 = {"a b"};

	ST.insert(val1, key1, ' ');

	REQUIRE(ST.supersets("", ' ').size()	== 1);

	REQUIRE(ST.supersets("c e", ' ').size() == 0);
	REQUIRE(ST.supersets("a", ' ').size()	== 1);
	REQUIRE(ST.supersets("b", ' ').size()	== 1);
	REQUIRE(ST.supersets("a b", ' ').size() == 1);
	REQUIRE(ST.supersets("c", ' ').size()	== 0);

	String key0 = {"empty"};
	String val0 = {""};

	REQUIRE(strcmp(ST.find(val0, ',').c_str(), "") == 0);

	ST.insert(val0, key0, ',');

	REQUIRE(strcmp(ST.find(val0, ',').c_str(), "empty") == 0);

	StringSet ss = ST.supersets("", ' ');

	REQUIRE(ss.size() == 2);
	REQUIRE(ss[0] == "empty");
	REQUIRE(ss[1] == "no_1");

	key1 = {"no_2"};
	val1 = {"a c d"};

	ST.insert(val1, key1, ' ');

	REQUIRE(ST.supersets("c e", ' ').size() == 0);
	REQUIRE(ST.supersets("a", ' ').size()	== 2);
	REQUIRE(ST.supersets("b", ' ').size()	== 1);
	REQUIRE(ST.supersets("a d", ' ').size() == 1);
	REQUIRE(ST.supersets("c", ' ').size()	== 1);

	key1 = {"yes_1"};
	val1 = {"a c e"};

	ST.insert(val1, key1, ' ');

	REQUIRE(ST.supersets("c e", ' ').size() == 1);
	REQUIRE(ST.supersets("a", ' ').size()	== 3);
	REQUIRE(ST.supersets("b", ' ').size()	== 1);
	REQUIRE(ST.supersets("a c", ' ').size() == 2);
	REQUIRE(ST.supersets("c", ' ').size()	== 2);

	key1 = {"yes_2"};
	val1 = {"a c d e"};

	ST.insert(val1, key1, ' ');

	REQUIRE(ST.supersets("c e", ' ').size() == 2);
	REQUIRE(ST.supersets("a", ' ').size()	== 4);
	REQUIRE(ST.supersets("c", ' ').size()	== 3);
	REQUIRE(ST.supersets("e", ' ').size()	== 2);

	key1 = {"no_3"};
	val1 = {"b d"};

	ST.insert(val1, key1, ' ');

	REQUIRE(ST.supersets("c e", ' ').size() == 2);
	REQUIRE(ST.supersets("a", ' ').size()	== 4);
	REQUIRE(ST.supersets("c", ' ').size()	== 3);
	REQUIRE(ST.supersets("b d", ' ').size()	== 1);

	key1 = {"no_4"};
	val1 = {"c d"};

	ST.insert(val1, key1, ' ');

	REQUIRE(ST.supersets("c e", ' ').size() == 2);
	REQUIRE(ST.supersets("a", ' ').size()	== 4);
	REQUIRE(ST.supersets("c", ' ').size()	== 4);
	REQUIRE(ST.supersets("b d", ' ').size()	== 1);

	key1 = {"yes_3"};
	val1 = {"c e"};

	ST.insert(val1, key1, ' ');

	REQUIRE(ST.supersets("c e", ' ').size() == 3);
	REQUIRE(ST.supersets("a", ' ').size()	== 4);
	REQUIRE(ST.supersets("c", ' ').size()	== 5);
	REQUIRE(ST.supersets("b d", ' ').size()	== 1);

	key1 = {"yes_4"};
	val1 = {"c d e f"};

	ST.insert(val1, key1, ' ');

	REQUIRE(ST.supersets("c e", ' ').size() == 4);
	REQUIRE(ST.supersets("f", ' ').size()	== 1);
	REQUIRE(ST.supersets("c", ' ').size()	== 6);
	REQUIRE(ST.supersets("e f", ' ').size()	== 1);

	key1 = {"no_5"};
	val1 = {"c d n"};

	ST.insert(val1, key1, ' ');

	REQUIRE(ST.supersets("c e", ' ').size() == 4);
	REQUIRE(ST.supersets("n", ' ').size()	== 1);
	REQUIRE(ST.supersets("c", ' ').size()	== 7);
	REQUIRE(ST.supersets("c d", ' ').size()	== 5);

	key1 = {"yes_5"};
	val1 = {"c d e f x"};

	ST.insert(val1, key1, ' ');

	REQUIRE(ST.supersets("c e", ' ').size() == 5);
	REQUIRE(ST.supersets("n", ' ').size()	== 1);
	REQUIRE(ST.supersets("c", ' ').size()	== 8);
	REQUIRE(ST.supersets("c d", ' ').size()	== 6);

	key1 = {"yes_6"};
	val1 = {"c d e f y"};

	ST.insert(val1, key1, ' ');

	REQUIRE(ST.supersets("c e", ' ').size() == 6);
	REQUIRE(ST.supersets("z", ' ').size()	== 0);
	REQUIRE(ST.supersets("c", ' ').size()	== 9);
	REQUIRE(ST.supersets("c d", ' ').size()	== 7);

	key1 = {"yes_7"};
	val1 = {"c d e f y z"};

	ST.insert(val1, key1, ' ');

	REQUIRE(ST.supersets("c e", ' ').size() == 7);
	REQUIRE(ST.supersets("z", ' ').size()	== 1);
	REQUIRE(ST.supersets("f y", ' ').size()	== 2);
	REQUIRE(ST.supersets("c d", ' ').size()	== 8);
	REQUIRE(ST.supersets("c", ' ').size()	== 10);

	key1 = {"no_6"};
	val1 = {"d e"};

	ST.insert(val1, key1, ' ');

	REQUIRE(ST.supersets("c e", ' ').size() == 7);
	REQUIRE(ST.supersets("z", ' ').size()	== 1);
	REQUIRE(ST.supersets("f y", ' ').size()	== 2);
	REQUIRE(ST.supersets("c d", ' ').size()	== 8);
	REQUIRE(ST.supersets("c", ' ').size()	== 10);

	key1 = {"no_7"};
	val1 = {"e a b d f n x y z"};

	ST.insert(val1, key1, ' ');

	REQUIRE(ST.supersets("c e", ' ').size() == 7);
	REQUIRE(ST.supersets("z", ' ').size()	== 2);
	REQUIRE(ST.supersets("f y", ' ').size()	== 3);
	REQUIRE(ST.supersets("d e", ' ').size()	== 7);
	REQUIRE(ST.supersets("c", ' ').size()	== 10);

	key1 = {"no_8"};
	val1 = {"c"};

	ST.insert(val1, key1, ' ');

	REQUIRE(ST.supersets("c e", ' ').size() == 7);
	REQUIRE(ST.supersets("z", ' ').size()	== 2);
	REQUIRE(ST.supersets("f y", ' ').size()	== 3);
	REQUIRE(ST.supersets("d e", ' ').size()	== 7);
	REQUIRE(ST.supersets("c", ' ').size()	== 11);

	key1 = {"yes_8"};
	val1 = {"e y z c"};

	ST.insert(val1, key1, ' ');

	REQUIRE(ST.supersets("c e", ' ').size() == 8);
	REQUIRE(ST.supersets("z", ' ').size()	== 3);
	REQUIRE(ST.supersets("y z", ' ').size()	== 3);
	REQUIRE(ST.supersets("d e", ' ').size()	== 7);
	REQUIRE(ST.supersets("c", ' ').size()	== 12);

	REQUIRE(ST.supersets("", ' ').size()	   == 17);
	REQUIRE(ST.supersets("xy", ' ').size()	   == 0);
	REQUIRE(ST.supersets("a b xy", ' ').size() == 0);

	ST.query = {3, 5, 7};
	ST.last_query_idx = ST.query.size() - 1;
	ST.result.clear();
	ST.supersets(ST.tree[0].idx_child, 0);

	REQUIRE(ST.result.size() == 0);

	ST.supersets(0, 0);

	REQUIRE(ST.result.size() == 0);

	int tot = 0;

	ss = ST.supersets("c e", ' ');

	for (int i = 0; i < 8; i ++) {
		if (ss[i] == "yes_1")
			tot += 1;
		if (ss[i] == "yes_2")
			tot += 2;
		if (ss[i] == "yes_3")
			tot += 4;
		if (ss[i] == "yes_4")
			tot += 8;
		if (ss[i] == "yes_5")
			tot += 16;
		if (ss[i] == "yes_6")
			tot += 32;
		if (ss[i] == "yes_7")
			tot += 64;
		if (ss[i] == "yes_8")
			tot += 128;
	}
	REQUIRE(tot == 255);
}


SCENARIO("Small Insert/subsets test") {

	SetTrie ST;

	ST.query = {3, 5, 7};
	ST.last_query_idx = ST.query.size() - 1;
	ST.result.clear();
	ST.subsets(ST.tree[0].idx_child, 0);

	REQUIRE(ST.result.size() == 0);

	ST.subsets(0, 0);

	REQUIRE(ST.result.size() == 0);

	REQUIRE(ST.subsets("c e", ' ').size() == 0);
	REQUIRE(ST.subsets("", ' ').size()	  == 0);

	String key1 = {"key_1"};
	String val1 = {"a b"};

	ST.insert(val1, key1, ' ');

	REQUIRE(ST.subsets("", ' ').size() == 0);

	REQUIRE(ST.subsets("c e", ' ').size()	== 0);
	REQUIRE(ST.subsets("a", ' ').size()		== 0);
	REQUIRE(ST.subsets("b", ' ').size()		== 0);
	REQUIRE(ST.subsets("a b", ' ').size()	== 1);
	REQUIRE(ST.subsets("c a b", ' ').size()	== 1);

	key1 = {"key_2"};
	val1 = {"a c d"};

	ST.insert(val1, key1, ' ');

	REQUIRE(ST.subsets("a", ' ').size()			== 0);
	REQUIRE(ST.subsets("x", ' ').size()			== 0);
	REQUIRE(ST.subsets("a b", ' ').size()		== 1);
	REQUIRE(ST.subsets("c a b", ' ').size()		== 1);
	REQUIRE(ST.subsets("c a d", ' ').size()		== 1);
	REQUIRE(ST.subsets("c a b d", ' ').size()	== 2);

	String key0 = {"empty"};
	String val0 = {""};

	REQUIRE(strcmp(ST.find(val0, ',').c_str(), "") == 0);

	ST.insert(val0, key0, ',');

	REQUIRE(strcmp(ST.find(val0, ',').c_str(), "empty") == 0);

	StringSet ss = ST.subsets("", ' ');

	REQUIRE(ss.size() == 1);
	REQUIRE(ss[0] == "empty");

	key1 = {"key_3"};
	val1 = {"a c e"};

	ST.insert(val1, key1, ' ');

	REQUIRE(ST.subsets("a", ' ').size()			== 1);
	REQUIRE(ST.subsets("x", ' ').size()			== 1);
	REQUIRE(ST.subsets("a b x", ' ').size()		== 2);
	REQUIRE(ST.subsets("c a b", ' ').size()		== 2);
	REQUIRE(ST.subsets("a c e", ' ').size()		== 2);
	REQUIRE(ST.subsets("c a b d e", ' ').size()	== 4);

	key1 = {"key_4"};
	val1 = {"a c d e"};

	ST.insert(val1, key1, ' ');

	REQUIRE(ST.subsets("a", ' ').size()			== 1);
	REQUIRE(ST.subsets("x y z", ' ').size()		== 1);
	REQUIRE(ST.subsets("a b x", ' ').size()		== 2);
	REQUIRE(ST.subsets("c a d e", ' ').size()	== 4);
	REQUIRE(ST.subsets("a c e", ' ').size()		== 2);
	REQUIRE(ST.subsets("c a b d e", ' ').size()	== 5);

	key1 = {"key_5"};
	val1 = {"b"};

	ST.insert(val1, key1, ' ');

	REQUIRE(ST.subsets("a", ' ').size()			== 1);
	REQUIRE(ST.subsets("b", ' ').size()			== 2);
	REQUIRE(ST.subsets("x y z", ' ').size()		== 1);
	REQUIRE(ST.subsets("a b x", ' ').size()		== 3);
	REQUIRE(ST.subsets("c a d e", ' ').size()	== 4);
	REQUIRE(ST.subsets("a c e", ' ').size()		== 2);
	REQUIRE(ST.subsets("c a b d e", ' ').size()	== 6);

	key1 = {"key_6"};
	val1 = {"c d"};

	ST.insert(val1, key1, ' ');

	REQUIRE(ST.subsets("a", ' ').size()			== 1);
	REQUIRE(ST.subsets("b", ' ').size()			== 2);
	REQUIRE(ST.subsets("c d", ' ').size()		== 2);
	REQUIRE(ST.subsets("c d x y z", ' ').size()	== 2);
	REQUIRE(ST.subsets("a b x", ' ').size()		== 3);
	REQUIRE(ST.subsets("c a d e", ' ').size()	== 5);
	REQUIRE(ST.subsets("a c e", ' ').size()		== 2);
	REQUIRE(ST.subsets("c a b d e", ' ').size()	== 7);

	key1 = {"key_7"};
	val1 = {"c e"};

	ST.insert(val1, key1, ' ');

	REQUIRE(ST.subsets("a", ' ').size()			== 1);
	REQUIRE(ST.subsets("b", ' ').size()			== 2);
	REQUIRE(ST.subsets("c d", ' ').size()		== 2);
	REQUIRE(ST.subsets("c d x y z", ' ').size()	== 2);
	REQUIRE(ST.subsets("a b x", ' ').size()		== 3);
	REQUIRE(ST.subsets("c a d e", ' ').size()	== 6);
	REQUIRE(ST.subsets("c e", ' ').size()		== 2);
	REQUIRE(ST.subsets("c a b d e", ' ').size()	== 8);

	key1 = {"key_8"};
	val1 = {"c d e f"};

	ST.insert(val1, key1, ' ');

	REQUIRE(ST.subsets("a", ' ').size()				== 1);
	REQUIRE(ST.subsets("b", ' ').size()				== 2);
	REQUIRE(ST.subsets("c d e f", ' ').size()		== 4);
	REQUIRE(ST.subsets("c d e f x y", ' ').size()	== 4);
	REQUIRE(ST.subsets("c a b d e f", ' ').size()	== 9);

	key1 = {"key_9"};
	val1 = {"c d n"};

	ST.insert(val1, key1, ' ');

	REQUIRE(ST.subsets("a", ' ').size()				==  1);
	REQUIRE(ST.subsets("b", ' ').size()				==  2);
	REQUIRE(ST.subsets("c d n", ' ').size()			==  3);
	REQUIRE(ST.subsets("c d n f x y", ' ').size()	==  3);
	REQUIRE(ST.subsets("c a b d e f n", ' ').size()	== 10);

	key1 = {"key_a"};
	val1 = {"c d e f x"};

	ST.insert(val1, key1, ' ');

	REQUIRE(ST.subsets("a", ' ').size()					==  1);
	REQUIRE(ST.subsets("b", ' ').size()					==  2);
	REQUIRE(ST.subsets("c d e f x", ' ').size()			==  5);
	REQUIRE(ST.subsets("c d e f x y z xx", ' ').size()	==  5);
	REQUIRE(ST.subsets("c a b d e f n x", ' ').size()	== 11);

	key1 = {"key_b"};
	val1 = {"c d e f y"};

	ST.insert(val1, key1, ' ');

	REQUIRE(ST.subsets("a", ' ').size()					==  1);
	REQUIRE(ST.subsets("b", ' ').size()					==  2);
	REQUIRE(ST.subsets("c d e f y", ' ').size()			==  5);
	REQUIRE(ST.subsets("c d e f x y z xx", ' ').size()	==  6);
	REQUIRE(ST.subsets("c a b d e f n x y", ' ').size()	== 12);

	key1 = {"key_c"};
	val1 = {"c d e f y z"};

	ST.insert(val1, key1, ' ');

	REQUIRE(ST.subsets("a", ' ').size()						==  1);
	REQUIRE(ST.subsets("b", ' ').size()						==  2);
	REQUIRE(ST.subsets("c d e f y z", ' ').size()			==  6);
	REQUIRE(ST.subsets("c d e f y z xx yy", ' ').size()		==  6);
	REQUIRE(ST.subsets("c a b d e f n x y z", ' ').size()	== 13);

	key1 = {"key_d"};
	val1 = {"d e"};

	ST.insert(val1, key1, ' ');

	REQUIRE(ST.subsets("a", ' ').size()						==  1);
	REQUIRE(ST.subsets("b", ' ').size()						==  2);
	REQUIRE(ST.subsets("d e", ' ').size()					==  2);
	REQUIRE(ST.subsets("d e xx yy", ' ').size()				==  2);
	REQUIRE(ST.subsets("c a b d e f n x y z", ' ').size()	== 14);

	key1 = {"key_e"};
	val1 = {"e a b d f n x y z"};

	ST.insert(val1, key1, ' ');

	REQUIRE(ST.subsets("a", ' ').size()						==  1);
	REQUIRE(ST.subsets("b", ' ').size()						==  2);
	REQUIRE(ST.subsets("e a b d f n x y z", ' ').size()		==  5);
	REQUIRE(ST.subsets("e a b d f n x y z xxy", ' ').size()	==  5);
	REQUIRE(ST.subsets("c a b d e f n x y z", ' ').size()	== 15);

	key1 = {"key_f"};
	val1 = {"c"};

	ST.insert(val1, key1, ' ');

	REQUIRE(ST.subsets("a", ' ').size()						==  1);
	REQUIRE(ST.subsets("b", ' ').size()						==  2);
	REQUIRE(ST.subsets("c", ' ').size()						==  2);
	REQUIRE(ST.subsets("c xx yy", ' ').size()				==  2);
	REQUIRE(ST.subsets("c a b d e f n x y z", ' ').size()	== 16);

	key1 = {"key_0"};
	val1 = {"e y z c"};

	ST.insert(val1, key1, ' ');

	REQUIRE(ST.subsets("xx yy aa bb cc dd ee", ' ').size()	==  1);
	REQUIRE(ST.subsets("a", ' ').size()						==  1);
	REQUIRE(ST.subsets("c", ' ').size()						==  2);
	REQUIRE(ST.subsets("e y z c", ' ').size()				==  4);
	REQUIRE(ST.subsets("e y z c xx yy", ' ').size()			==  4);
	REQUIRE(ST.subsets("a b c d e f n x y z", ' ').size()	== 17);

	ST.query = {3, 5, 7};
	ST.last_query_idx = ST.query.size() - 1;
	ST.result.clear();
	ST.subsets(ST.tree[0].idx_child, 0);

	REQUIRE(ST.result.size() == 0);

	ST.subsets(0, 0);

	REQUIRE(ST.result.size() == 0);

	int tot = 0;

	ss = ST.subsets("a b c d e f n x y z", ' ');

	for (int i = 0; i < 17; i ++) {
		if (ss[i] == "key_0")
			tot += 1;
		if (ss[i] == "key_1")
			tot += 2;
		if (ss[i] == "key_2")
			tot += 4;
		if (ss[i] == "key_3")
			tot += 8;
		if (ss[i] == "key_4")
			tot += 16;
		if (ss[i] == "key_5")
			tot += 32;
		if (ss[i] == "key_6")
			tot += 64;
		if (ss[i] == "key_7")
			tot += 128;
		if (ss[i] == "key_8")
			tot += 256;
		if (ss[i] == "key_9")
			tot += 512;
		if (ss[i] == "key_a")
			tot += 1024;
		if (ss[i] == "key_b")
			tot += 2048;
		if (ss[i] == "key_c")
			tot += 4096;
		if (ss[i] == "key_d")
			tot += 8192;
		if (ss[i] == "key_e")
			tot += 16384;
		if (ss[i] == "key_f")
			tot += 32768;
		if (ss[i] == "empty")
			tot += 65536;
	}
	REQUIRE(tot == 131071);
}

#endif
