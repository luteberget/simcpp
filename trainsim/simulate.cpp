#include "simulate.h"
#include "simcpp.h"
#include "world.h"

History simulate(InfrastructureSpec& infrastructure, Plan& plan) {
    History output;
    auto sim = Simulation::create();
    auto world = World::Create(sim, infrastructure);
    return output;
}
