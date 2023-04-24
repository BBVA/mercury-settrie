#include <iostream>
#include <fstream>
#include <sstream>
#include <string.h>


#define CATCH_CONFIG_MAIN		//This tells Catch2 to provide a main() - has no effect when CATCH_TEST is not defined

#include "st_main.h"

#if !defined TEST

using namespace std;


int main(int argc, char* argv[]) {

	char fn [128];

	SetTrie ST;

	String doc;

	for (int i = 1; i <= 50000; i++) {
		sprintf(fn, "./settrie_data/%i.txt", i);

		std::ifstream fh (fn);

		if (!fh.is_open()) {
			std::cout << "Not found:" << fn << endl << endl;
			std::cout << "That is just a test dataset. You can create any text files named 1.txt to 50000.txt" << endl;
			std::cout << "and copy them to a folder named /settrie_data to try this out." << endl;
			return false;
		}
		getline(fh, doc);
		fh.close();

		StringSet str;
		std::stringstream ss(doc);

		String elem;
		while (std::getline(ss, elem, ' '))
			str.push_back(elem);

		ST.insert(str, String(fn));
	}
	std::cout << "Loaded." << endl;

	for (int i = 1; i <= 50000; i++) {
		sprintf(fn, "./settrie_data/%i.txt", i);

		std::ifstream fh (fn);

		if (!fh.is_open()) {
			std::cout << "Not found:" << fn << endl;
			return false;
		}
		getline(fh, doc);
		fh.close();

		StringSet str;
		std::stringstream ss(doc);

		String elem;
		while (std::getline(ss, elem, ' '))
			str.push_back(elem);

		std:string key = ST.find(str);

		if (strcmp(fn, key.c_str()) != 0)
			printf("Failed: %s -- %s\n", fn, key.c_str());
	}
	std::cout << "Verified." << endl;
};

#endif
