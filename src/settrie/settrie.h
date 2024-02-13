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
#ifndef INCLUDED_SETTRIE
#define INCLUDED_SETTRIE

#include <algorithm>
#include <functional>
#include <map>
#include <string>
#include <vector>

#define IMAGE_BUFF_SIZE					6136

typedef uint64_t 						ElementHash;
typedef std::string						String;

typedef std::vector<ElementHash>		BinarySet;
typedef std::vector<String>				StringSet;
typedef std::vector<int>				IdList;

typedef std::map<ElementHash, String>	StringName;
typedef std::map<int, String>			IdMap;

struct SetNode {
	ElementHash value;

	int idx_next, idx_child, idx_parent;

	bool is_flagged;
};

typedef std::vector<SetNode>			BinaryTree;

// This structure is 64encoded to 8K (3 input bytes == 24 bit -> 4 output chars == 24 bit). Therefore, its size is 6K.
struct ImageBlock {
	int size, block_num;

	uint8_t buffer[IMAGE_BUFF_SIZE];		// makes sizeof(ImageBlock) == 6K
};

typedef std::vector<ImageBlock>			BinaryImage;
typedef BinaryImage					   *pBinaryImage;


class SetTrie {

	public:

		SetTrie() {
			SetNode root = {0, 0, 0, -1, false};
			tree.push_back(root);
		}

		void	  insert	(StringSet set, String id);
		void	  insert	(String str, String str_id, char split);
		String	  find		(StringSet set);
		String	  find		(String str, char split);
		StringSet supersets	(StringSet set);
		StringSet supersets	(String str, char split);
		StringSet subsets	(StringSet set);
		StringSet subsets	(String str, char split);
		StringSet elements	(int idx);
		bool	  load		(pBinaryImage &p_bi);
		bool	  save		(pBinaryImage &p_bi);

		IdMap	   id	  = {};

#ifndef TEST
	private:
#endif

	inline int insert(int idx, ElementHash value) {

		if (tree[idx].idx_child == 0) {
			SetNode node = {value, 0, 0, idx, false};

			tree.push_back(node);

			int idx_c = tree.size() - 1;

			tree[idx].idx_child = idx_c;

			return idx_c;
		}

		idx	= tree[idx].idx_child;

		while (true) {
			if (tree[idx].value == value)
				return idx;

			if (tree[idx].idx_next == 0) {
				SetNode node = {value, 0, 0, tree[idx].idx_parent, false};

				tree.push_back(node);

				int idx_c = tree.size() - 1;

				tree[idx].idx_next = idx_c;

				return idx_c;
			}

			idx = tree[idx].idx_next;
		}
	}

	inline int insert(BinarySet set) {

		int idx	 = 0;
		int size = set.size();

		if (size == 0) {
			tree[0].is_flagged = true;

			return 0;
		}

		for (int i = 0; i < size; i++)
			idx	= insert(idx, set[i]);

		tree[idx].is_flagged = true;

		return idx;
	}

	inline int find(int idx, ElementHash value) {

		if ((idx = tree[idx].idx_child) == 0)
			return 0;

		while (true) {
			if (tree[idx].value == value)
				return idx;

			if ((idx = tree[idx].idx_next) == 0)
				return 0;
		}
	}

	inline int find(BinarySet set) {

		int idx	 = 0;
		int size = set.size();

		for (int i = 0; i < size; i++)
			if ((idx = find(idx, set[i])) == 0)
				return 0;

		return idx;
	}

	inline void all_supersets(int t_idx) {

		while (t_idx != 0) {
			if (tree[t_idx].is_flagged)
				result.push_back(t_idx);

			if (int ci = tree[t_idx].idx_child)
				all_supersets(ci);

			t_idx = tree[t_idx].idx_next;
		}
	}

	inline void supersets(int t_idx, int s_idx) {

		while (t_idx != 0) {
			ElementHash t_value, q_value;

			int found = 0;

			if ((t_value = tree[t_idx].value) == (q_value = query[s_idx])) {
				if (s_idx == last_query_idx) {

					if (tree[t_idx].is_flagged)
						result.push_back(t_idx);

					if (int ci = tree[t_idx].idx_child)
						all_supersets(ci);

				} else {
					found = 1;
					q_value = query[s_idx + 1];
				}
			}

			int ni;
			if (t_value < q_value && (ni = tree[t_idx].idx_child) != 0)
				supersets(ni, s_idx + found);

			t_idx = tree[t_idx].idx_next;
		}
	}

	inline void subsets(int t_idx, int s_idx) {

		while (t_idx != 0) {
			ElementHash t_value;
			if ((t_value = tree[t_idx].value) >= query[s_idx]) {
				int ns_idx = s_idx;

				while (ns_idx < last_query_idx && query[ns_idx] < t_value)
					ns_idx++;

				if (query[ns_idx] == t_value) {
					if (tree[t_idx].is_flagged)
						result.push_back(t_idx);

					int ni;
					if ((ni = tree[t_idx].idx_child) != 0) {
						ns_idx++;

						if (ns_idx <= last_query_idx)
							subsets(ni, ns_idx);
					}
				}
			}

			t_idx = tree[t_idx].idx_next;
		}
	}

	int last_query_idx;

	BinarySet  query  = {};
	IdList	   result = {};
	BinaryTree tree	  = {};
	StringName name	  = {};
};
#endif
