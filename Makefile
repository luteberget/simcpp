HEADER=simcpp.h protothread.h
SOURCE=simcpp.cpp
EXE=example-minimal example-twocars

.PHONY: clean

all: $(EXE)

%: %.cpp $(HEADER) $(SOURCE)
	g++ -Wall -std=c++11 $< $(SOURCE) -o $@

clean:
	rm $(EXE)
