il: il.cpp simcpp.h simobj.h
	g++ -Wall -std=c++11 il.cpp -o il

all: example-minimal example-twocars
.PHONY: all

example-minimal: example-minimal.cpp simcpp.h
	g++ -Wall -std=c++11 example-minimal.cpp -o example-minimal

example-twocars: example-twocars.cpp simcpp.h
	g++ -Wall -std=c++11 example-twocars.cpp -o example-twocars
