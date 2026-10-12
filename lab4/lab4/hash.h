#ifndef HASH_H
#define HASH_H

#include <stdio.h>
#include "list.h"

#define HASH_INDEX(_addr,_size_mask) (((_addr) >> 2) & (_size_mask))

template <class Element, class Key> class hash;

template <class Element, class Key> class hash {
private:
	unsigned my_size_log;
	unsigned my_size;
	unsigned my_size_mask;
	list<Element, Key> *entries;

public:
	void setup(unsigned the_size_log = 5);
	unsigned size();
	void insert(Element * e);
	Element *lookup(Key the_key);
	void print(FILE * f = stdout);
	void reset();
	void cleanup();
	list<Element, Key> *get_list(unsigned the_idx);
};

template <class Element, class Key>
void
hash<Element, Key>::setup(unsigned the_size_log) {
	my_size_log = the_size_log;
	my_size = 1 << my_size_log;
	my_size_mask = (1 << my_size_log) - 1;
	entries = new list<Element, Key>[my_size];
}

template <class Element, class Key>
unsigned
hash<Element, Key>::size() {
	return my_size;
}

template <class Element, class Key>
list<Element, Key> *
hash<Element, Key>::get_list(unsigned the_idx) {
	if (the_idx >= my_size) {
		fprintf(stderr, "hash::get_list(): idx out of range!\n");
		exit(1);
	}
	return &entries[the_idx];
}

template <class Element, class Key>
Element *
hash<Element, Key>::lookup(Key the_key) {
	list<Element, Key> *l;

	l = &entries[HASH_INDEX(the_key, my_size_mask)];
	return l->lookup(the_key);
}

template <class Element, class Key>
void
hash<Element, Key>::print(FILE * f) {
	for (unsigned i = 0; i < my_size; i++) {
		entries[i].print(f);
	}
}

template <class Element, class Key>
void
hash<Element, Key>::reset() {
	for (unsigned i = 0; i < my_size; i++) {
		entries[i].cleanup();
	}
}

template <class Element, class Key>
void
hash<Element, Key>::cleanup() {
	reset();
	delete[]entries;
}

template <class Element, class Key>
void
hash<Element, Key>::insert(Element * e) {
	entries[HASH_INDEX(e->key(), my_size_mask)].push(e);
}

#endif
