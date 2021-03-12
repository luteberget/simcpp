#include "simcpp.h"

#include <cstdio>

using std::shared_ptr;

class Car : public Process {
public:
  Car(shared_ptr<Simulation> sim) : Process(sim) {}

  virtual bool Run() override {
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
