help:
	@echo ""
	@echo "make [void],"
	@echo "make help       : Show this help."
	@echo ""
	@echo "make set_trie   : Make the RELEASE SetTrie c++ executable"
	@echo "make tset_trie  : Make the DEBUG&TEST SetTrie executable."
	@echo "make link_gram  : Make the RELEASE LinkGrammar c++ executable"
	@echo "make tlink_gram : Make the DEBUG&TEST LinkGrammar executable."
	@echo ""
	@echo "make clean      : Clean up all files not stored in the repo."
	@echo "make package    : Clean, compile all and build the mercury-dynamics package."

	@echo ""

RFLAGS := -DNDEBUG -O3
TFLAGS := -DDEBUG -DTEST -g

ifneq ("$(wildcard mode_release)","")
	CPPFLAGS := $(RFLAGS)
endif

ifneq ("$(wildcard mode_test)","")
	CPPFLAGS := $(TFLAGS)
endif

CXXFLAGS := -std=c++11 -ILinkGrammar -ISetTrie -Icatch2

VPATH = LinkGrammar SetTrie catch2

st_main.o: st_main.h SetTrie.h
SetTrie.o: SetTrie.h
lg_main.o: lg_main.h LinkGrammar.h
LinkGrammar.o: LinkGrammar.h

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

set_trie: mode_release st_main.o SetTrie.o
	@echo "Making SetTrie as set_trie ..."
	g++ -o set_trie st_main.o SetTrie.o

tset_trie: mode_test st_main.o SetTrie.o
	@echo "Making SetTrie as tset_trie ..."
	g++ -o tset_trie st_main.o SetTrie.o

link_gram: mode_release lg_main.o LinkGrammar.o
	@echo "Making LinkGrammar as link_gram ..."
	g++ -o link_gram lg_main.o LinkGrammar.o

tlink_gram: mode_test lg_main.o LinkGrammar.o
	@echo "Making LinkGrammar as tlink_gram ..."
	g++ -o tlink_gram lg_main.o LinkGrammar.o

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
