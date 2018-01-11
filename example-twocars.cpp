#include "simcpp.h"
#include <string>
using std::shared_ptr;
using std::string;

class Car : public Process {
private:
  bool finished = false;
  double target_time;
  string name;

public:
  Car(shared_ptr<Simulation> sim, string name)
      : Process(sim), target_time(sim->get_now() + 100.0), name(name) {}
  virtual bool Run() {
    PT_BEGIN();
    while (!this->finished) {
      PROC_WAIT_FOR(sim->timeout(5.0));
      printf("Car %s running at %g.\n", this->name.c_str(), sim->get_now());
      if (sim->get_now() >= this->target_time)
        this->finished = true;
    }
    PT_END();
  }
};

class TwoCars : public Process {
public:
  TwoCars(shared_ptr<Simulation> sim) : Process(sim) {}
  virtual bool Run() {
    PT_BEGIN();

    printf("Starting car C1.\n");
    PROC_WAIT_FOR(sim->start_process<Car>("C1"));
    printf("Finished car C1.\n");

    printf("Starting car C2.\n");
    PROC_WAIT_FOR(sim->start_process<Car>("C2"));
    printf("Finished car C2.\n");

    PT_END();
  }
};

int main() {
  auto sim = Simulation::create();
  sim->start_process<TwoCars>();
  sim->advance_by(10000);

  return 0;
}
