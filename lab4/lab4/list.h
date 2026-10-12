#ifndef LIST_H
#define LIST_H

#include <stdio.h>

template <class Element, class Key> class list;

template <class Element, class Key> class list {
private:
	Element * my_head;
	unsigned long long my_num_ele;
public:
	list() {
		my_head = NULL;
		my_num_ele = 0;
	}
	void setup();
	unsigned num_ele() {
		return my_num_ele;
	}
	Element *head() {
		return my_head;
	}
	Element *lookup(Key the_key);
	void push(Element * e);
	Element *pop();
	void print(FILE * f = stdout);

	void cleanup();
};

template <class Element, class Key>
void
list<Element, Key>::setup() {
	my_head = NULL;
	my_num_ele = 0;
}

template <class Element, class Key>
void
list<Element, Key>::push(Element * e) {
	e->next = my_head;
	my_head = e;
	my_num_ele++;
}

template <class Element, class Key>
Element *
list<Element, Key>::pop() {
	Element *e;
	e = my_head;
	if (e) {
		my_head = e->next;
		my_num_ele--;
	}
	return e;
}

template <class Element, class Key>
void
list<Element, Key>::print(FILE * f) {
	Element *e_tmp = my_head;

	while (e_tmp) {
		e_tmp->print(f);
		e_tmp = e_tmp->next;
	}
}

template <class Element, class Key>
Element *
list<Element, Key>::lookup(Key the_key) {
	Element *e_tmp = my_head;

	while (e_tmp && (e_tmp->key() != the_key)) {
		e_tmp = e_tmp->next;
	}
	return e_tmp;
}

template <class Element, class Key>
void
list<Element, Key>::cleanup() {
	Element *e_tmp = my_head;
	Element *e_next;

	while (e_tmp) {
		e_next = e_tmp->next;
		delete e_tmp;
		e_tmp = e_next;
	}
	my_head = NULL;
	my_num_ele = 0;
}

#endif
