# SimCpp

SimCpp is a discrete event simulation framework for C++.
It aims to be a port of [SimPy](https://simpy.readthedocs.io/en/latest/).
It is based on [Protothreads](http://dunkels.com/adam/pt) and a [C++ port of Protothreads](https://github.com/benhoyt/protothreads-cpp).

## Minimal Example

```c++
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
```

This example can be compiled with `g++ -Wall -std=c++11 example-minimal.cpp simcpp.cpp -o example.minimal`.
When executed with `./example-minimal`, it produces the following output:

```text
Car running at 0.
Car running at 5.
```

## Installation

To use SimCpp, you need the files `simcpp.cpp`, `simcpp.h`, and `protothread.h`.
Whenever you want to use SimCpp in a file, include it with `#include "simcpp.h"`.
When compiling your program, you have to include the `simcpp.cpp` file.

## Getting Started

A SimCpp simulation is created by calling `simcpp::Simulation::create();`.
This returns a shared pointer to an instance of `simcpp::Simulation` to simplify memory management.
In this simulation, one or more processes have to be started to actually do things.
This is done with `sim->start_process<MyProcessClass>()`.

Each process is modeled as a subclass of `simcpp::Process`.
The behavior of a process is implemented in its `Run` method.
The `Run` method should start with (optional) variable declarations, followed by `PT_BEGIN();`, the behavior code, and finally `PT_END();`.

In between `PT_BEGIN();` and `PT_END();` you can use `PROC_WAIT_FOR(...);` to wait for events.
Events can be created with `sim->event()` or `sim->timeout(...)`.
These methods will be explained later.
Processes are events as well, so you can start new processes and wait for their completion using `PROC_WAIT_FOR(...);` too.

The `sim` class attribute is a weak pointer to the simulation instance to prevent cyclic references.
Never permanently store shared pointers to the simulation inside your processes!
The local variables of the `Run` method are reset after (most) calls to `PROC_WAIT_FOR`, so keeping a shared pointer to the simulation instance in the local variable `sim` is acceptable (this is done via `auto sim = this->sim.lock()` in the above example).
This also means that you have to use class attributes instead of local variables to keep state across calls to `PROC_WAIT_FOR`.

As stated before, `auto event = sim->event()` can be used to create new events.
Again, this returns a shared pointer to an instance of `simcpp::Event` to simplify memory management.
Initially, the returned event is pending.
When calling `PROC_WAIT_FOR(event);` with a pending event, the process is paused.

By calling `event->trigger()`, the event is triggered.
This means that all processes waiting for the event are resumed.
When calling `PROC_WAIT_FOR(event);` with a triggered event, nothing special happens.

When calling `auto event = sim->timeout(5)`, a pending event is returned too.
However, this event is automatically triggered after 5 timesteps.
Thus, if you write `PROC_WAIT_FOR(sim->timeout(5))`, the process is paused for 5 timesteps.

To check the current simulation time, you can use `sim->get_now()`.
Initially, it will return 0, but it will increase when the simulation is run.
To run the simulation, use `sim->run()`.
This runs the simulation until no further events are scheduled to be triggered.
It is also possible to run the simulation for a specific duration with `sim->advance_by(100)` (in this case, the simulation is advanced by 100 timesteps).

Now you should know all the basics to write your discrete event simulation with SimCpp.
Continue reading to learn about all features.
If you have any questions or problems, please file an issue on GitHub: <https://github.com/luteberget/simcpp/issues>.
If you want to improve SimCpp, feel free to submit a pull request.

## API

### Creating the simulation

```c++
auto sim = simcpp::Simulation::create();
```

### Starting processes

Construct the `Car` process with two additional arguments and run it:

*The `Car` instance is returned.*

```c++
auto car = sim->start_process<Car>(arg1, arg2);
```

Construct the `Car` process with two additional arguments and run it after the given delay:

*The `Car` instance is returned.*

```c++
auto car = sim->start_process_delayed<Car>(delay, arg1, arg2);
```

Manually construct the `Car` process using `std::make_shared` and run it:

```c++
auto car = std::make_shared<Car>(sim, arg1, arg2);
sim->run_process(car);
```

Manually construct the `Car` process using `std::make_shared` and run it after the given delay:

```c++
auto car = std::make_shared<Car>(sim, arg1, arg2);
sim->run_process(car, delay);
```

### Creating events

Create an event:

```c++
auto event = sim->event();
```

Create a timeout event which is scheduled to be processed after the given delay:

```c++
auto event = sim->timeout(delay);
```

Create an event which is triggered when any of the given events is processed:

```c++
auto event = sim->any_of({ event1, event2 });
```

Create an event which is triggered when all of the given events are processed:

```c++
auto event = sim->all_of({ event1, event2 });
```

### Running the simulation

Run the simulation until no scheduled events are left:

```c++
sim->run();
```

Advance the simulation by the given duration:

```c++
sim->advance_by(duration);
```

Advance the simulation until the given event is triggered:

*Returns `true` if the event was triggered and `false` is the simulation stopped because the event was aborted or no scheduled events are left.*

```c++
bool ok = sim->advance_to(event);
```

Process the next scheduled event:

*Returns `true` if there was a scheduled event to be processed and `false` otherwise.*

```c++
bool ok = sim->step();
```

### Checking the simulation state

Get the current simulation time:

```c++
double now = sim->get_now();
```

Check whether a scheduled event is left:

```c++
bool has_next = sim->has_next();
```

Get the time at which the next event is scheduled:

*If no events are scheduled, this method throws an exception.*

```c++
double time = sim->peek_next_time();
```

### Changing the event state

Schedule the event to be processed:

*If the event was not pending, nothing is done.
Returns `true` if the event was pending, `false` otherwise.*

```c++
bool ok = event->trigger();
```

Schedule the event to be processed after the given delay:

*If the event was not pending, nothing is done.
Returns `true` if the event was pending, `false` otherwise.*

```c++
bool ok = event->trigger(delay);
```

Abort the event:

*If the event was not pending, nothing is done.
Otherwise, calls the `Abort` callback of the event / process.
Returns `true` if the event was pending, `false` otherwise.*

```c++
bool ok = event->abort();
```

### Checking the event state

Check whether the event is pending:

```c++
bool pending = event->is_pending();
```

Check whether the event is triggered:

*This also returns `true` if the event is already processed.*

```c++
bool triggered = event->is_triggered();
```

Check whether the event is processed:

```c++
bool processed = event->is_processed();
```

Check whether the event is aborted:

```c++
bool aborted = event->is_aborted();
```

Get the state of an event:

*The state is one of `{Pending, Triggered, Processed, Aborted}`.*

```c++
simcpp::Event::State state = event->get_state();
```

### Waiting for events

Add a callback to be called when the event is processed:

*If the event was not pending, nothing is done.
Returns `false` if the event is triggered, `true` otherwise.*

```c++
bool ok = event->add_handler(callback);
```

Pause the process until the event is processed:

*If the event is aborted, the process is paused indefinitely.
If the event is triggered or processed, the process is not paused.*

```c++
PROC_WAIT_FOR(handler);
```

### Subclassing `simcpp::Event`

The `simcpp::Event` class can be subclassed to create custom event classes.
Custom events can take additonal arguments in their constructor, store attributes, offer methods, and override the `Aborted` callback.
Note that the `sim` attribute is only a weak pointer to the simulation instance.
It must be converted to a shared pointer first to use it (`sim.lock()`).

```c++
class MyEvent : simcpp::Event {
  MyEvent(simcpp::SimulationPtr sim, int arg1) : Event(sim) {}

  void Aborted() override {
    // ...
  }
}
```

### Subclassing `simcpp::Process`

The `simcpp::Process` class can be subclassed to create process classes.
Custom events can take additonal arguments in their constructor, store attributes, offer methods, and override the `Run` method and `Aborted` callback.
Note that the `sim` attribute is only a weak pointer to the simulation instance.
It must be converted to a shared pointer first to use it (`sim.lock()`).

```c++
class MyProcess : simcpp::Process {
  MyProcess(simcpp::SimulationPtr sim, int arg1) : Process(sim) {}

  bool Run() override {
    auto sim = this->sim.lock();

    PT_BEGIN();
    // ...
    PT_END();
  }

  void Aborted() override {
    // ...
  }
}
```

## Copyright and License

Copyright © 2021 Bjørnar Steinnes Luteberget.

Licensed under the MIT License.
See the LICENSE file for details.
