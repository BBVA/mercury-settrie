help:
	@echo ""
	@echo "make [void],"
	@echo "make help    : Show this help."
	@echo ""
	@echo "make settrie : Make the command line settrie executable"
	@echo "make test    : Make the DEBUG&TEST settrie executable."
	@echo "make doc_cpp : Build the settrie C++ documentation."
	@echo "make doc_py  : Build the settrie Python documentation."
	@echo ""
	@echo "make clean   : Clean up all files not stored in the repo."
	@echo "make package : Clean, compile all and build the Python settrie package."

	@echo ""

RFLAGS := -DNDEBUG -O3
TFLAGS := -DDEBUG -DTEST -g

ifneq ("$(wildcard mode_release)","")
	CPPFLAGS := $(RFLAGS)
endif

ifneq ("$(wildcard mode_test)","")
	CPPFLAGS := $(TFLAGS)
endif

CXXFLAGS := -std=c++11 -Isettrie -Icatch2

VPATH = settrie catch2

st_main.o: st_main.h settrie.h
settrie.o: settrie.h

mode_release:
	@echo "Switching to mode RELEASE ..."
	@make clean
	@touch mode_release
	$(eval CPPFLAGS = $(RFLAGS))

mode_test:
	@echo "Switching to mode DEBUG&TEST ..."
	@make clean
	@touch mode_test
	$(eval CPPFLAGS = $(TFLAGS))

settrie: mode_release st_main.o settrie.o
	@echo "Making settrie as settrie_cli ..."
	g++ -o settrie_cli st_main.o settrie.o

test: mode_test st_main.o settrie.o
	@echo "Making settrie as settrie_test ..."
	g++ -o settrie_test st_main.o settrie.o

.PHONY : clean
clean  :
	@echo "Cleaning up all files not stored in the repo ..."
	@rm -f *.o SetTrie/*.o
	@rm -f set_trie tset_trie link_gram tlink_gram
	@rm -f mode_*
	@rm -f SetTrie/*.so
	@find . | grep __pycache__ | xargs rm -rf
	@find . | grep -e ".ipynb_checkpoints$" | xargs rm -rf

.PHONY  : package
package :
	cd reels && swig -python py_reels.i
