HEADER=simcpp.h protothread.h
EXE=example-minimal example-twocars

.PHONY: clean

all: $(EXE)

%: %.cpp $(HEADER)
	g++ -Wall -std=c++11 $< -o $@

clean:
	rm $(EXE)
