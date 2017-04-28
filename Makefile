CPP = g++ -O2 -Wno-deprecated
CPPDEBUG = g++ -O2 -Wno-deprecated -g



sketch_sampling:  relation.o xis.o sketches.o sampling.o sketch_sampling.o
	$(CPP) -lgsl -lgslcblas -o sketch_sampling.out relation.o xis.o sketches.o sampling.o sketch_sampling.o

sketch_sampling.o : sketch_sampling.cpp
	$(CPP) -c sketch_sampling.cpp


sketch_compare:  relation.o xis.o sketches.o sketch_compare.o
	$(CPP) -o sketch_compare.out relation.o xis.o sketches.o sketch_compare.o

sketch_compare.o : sketch_compare.cpp
	$(CPP) -c sketch_compare.cpp



relation.o : relation.h relation.cpp data_generator.h
	$(CPP) -c relation.cpp

xis.o : xis.h xis.cpp gen_scheme.h range_sum.h RM7_range_sum.h
	$(CPP) -c xis.cpp

sketches.o : sketches.h sketches.cpp
	$(CPP) -c sketches.cpp

sampling.o : sampling.h sampling.cpp
	$(CPP) -c sampling.cpp



clean:
	rm -f *.out
	rm -f *.o
	rm -f *~

