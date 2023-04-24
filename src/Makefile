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
clean:
	@echo "Cleaning up all files not stored in the repo ..."
	@rm -f *.o settrie_cli settrie_test mode_* errors.log .coverage
	@rm -f settrie/*.so settrie/*.o
	@rm -f logs/*
	@find . | grep __pycache__ | xargs rm -rf
	@find . | grep .ipynb_checkpoints | xargs rm -rf
	@rm -rf ../reference ../build ../dist ../mercury_settrie.egg-info ../src/mercury_settrie.egg-info

.PHONY	: doc_cpp
doc_cpp:
	@-doxygen 2>errors.log 1>/dev/null
	@cat errors.log

.PHONY	: doc_py
doc_py:
	@-mkdocs build 2>errors.log 1>/dev/null
	@cat errors.log | grep -vE 'No type or annotation for'

.PHONY	: package
package: mode_release
	g++ -c -fpic -O3 -std=c++11 -Isettrie -DNDEBUG -o settrie.o settrie/settrie.cpp
	cd settrie && swig -python -o py_settrie_wrap.cpp py_settrie.i && mv py_settrie.py __init__.py && cat imports.in >>__init__.py
	g++ -c -fpic -O3 settrie/py_settrie_wrap.cpp -Dpython -I/usr/include/python3.8 -I/usr/include/python3.10
	g++ -shared settrie.o py_settrie_wrap.o -o settrie/_py_settrie.so
	@printf "\nPython 3.x package was built locally in the folder './settrie'.\n"
	@printf "\nYou can run 'import settrie' for here or ./test.sh to test it!\n"
