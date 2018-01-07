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
  std::shared_ptr<Simulation> sim = std::make_shared<Simulation>();
  sim->start_process(std::make_shared<Car>(sim));
  sim->run();

  return 0;
}
