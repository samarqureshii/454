CXX      = g++
CXXFLAGS = -Wall -O3 -std=gnu++17 -pthread
LDFLAGS  =
LDLIBS   = -pthread

HDRS = defs.h hash.h list.h

all: randtrack randtrack_parallel \
     randtrack_global_lock randtrack_list_lock \
     randtrack_element_lock randtrack_reduction

# serial (correct output)
randtrack: randtrack.cc $(HDRS)
	$(CXX) $(CXXFLAGS) randtrack.cc -o $@ $(LDFLAGS) $(LDLIBS)

# basic pthread-parallel version without synchonirzation
randtrack_parallel: randtrack_parallel.cc $(HDRS)
	$(CXX) $(CXXFLAGS) randtrack_parallel.cc -o $@ $(LDFLAGS) $(LDLIBS)

randtrack_global_lock: randtrack_global_lock.cc $(HDRS)
	$(CXX) $(CXXFLAGS) randtrack_global_lock.cc -o $@ $(LDFLAGS) $(LDLIBS)

randtrack_list_lock: randtrack_list_lock.cc $(HDRS)
	$(CXX) $(CXXFLAGS) randtrack_list_lock.cc -o $@ $(LDFLAGS) $(LDLIBS)

randtrack_element_lock: randtrack_element_lock.cc $(HDRS)
	$(CXX) $(CXXFLAGS) randtrack_element_lock.cc -o $@ $(LDFLAGS) $(LDLIBS)

randtrack_reduction: randtrack_reduction.cc $(HDRS)
	$(CXX) $(CXXFLAGS) randtrack_reduction.cc -o $@ $(LDFLAGS) $(LDLIBS)

.PHONY: clean
clean:
	rm -f *.out *.time *.o randtrack randtrack_parallel \
	      randtrack_global_lock randtrack_list_lock \
	      randtrack_element_lock randtrack_reduction
