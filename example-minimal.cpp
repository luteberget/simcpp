// Copyright (c) 2021 Bjørnar Steinnes Luteberget
// Licensed under the MIT license. See the LICENSE file for details.

#include <cstdio>

#include "simcpp.h"

class Car : public simcpp::Process {
public:
  explicit Car(simcpp::SimulationPtr sim) : Process(sim) {}

  bool Run() override {
    auto sim = this->sim.lock();

    PT_BEGIN();

    printf("Car running at %g.\n", sim->get_now());
    PROC_WAIT_FOR(sim->timeout(5.0));
    printf("Car running at %g.\n", sim->get_now());

    PT_END();
  }
};

int main() {
  auto sim = simcpp::Simulation::create();
  sim->start_process<Car>();
  sim->run();

  return 0;
}
