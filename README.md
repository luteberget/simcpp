# SimCpp

Discrete event simulation framework for C++.
Aims to be a port of [SimPy](https://simpy.readthedocs.io/en/latest/).
Based on [Protothreads](http://dunkels.com/adam/pt/) and a [C++ port of Protothreads](https://github.com/benhoyt/protothreads-cpp).

## Minimal example
```c++
#include "simcpp.h"
using std::shared_ptr;

class Car : public Process {
public:
  Car(shared_ptr<Simulation> sim) : Process(sim) {}
  virtual bool Run() {
    PT_BEGIN();

    printf("Car running at %g.\n", sim->get_now());
    PROC_WAIT_FOR(sim->timeout(5.0));
    printf("Car running at %g.\n", sim->get_now());

    PT_END();
  }
};

int main() {
  auto sim = Simulation::create();
  sim->start_process<Car>();
  sim->run();

  return 0;
}
```
