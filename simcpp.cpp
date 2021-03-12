#include "simcpp.h"

#include <utility>

namespace simcpp {

/* Simulation */

SimulationPtr Simulation::create() { return std::make_shared<Simulation>(); }

ProcessPtr Simulation::run_process(ProcessPtr process) {
  auto event = this->event();
  event->add_handler(process);
  schedule(event);
  return process;
}

EventPtr Simulation::event() {
  return std::make_shared<Event>(shared_from_this());
}

EventPtr Simulation::timeout(double delay) {
  auto event = this->event();
  schedule(event, delay);
  return event;
}

EventPtr Simulation::any_of(std::initializer_list<EventPtr> events) {
  int n = 1;
  for (auto &event : events) {
    if (event->is_triggered()) {
      n = 0;
      break;
    }
  }

  auto process = start_process<Condition>(n);
  for (auto &event : events) {
    event->add_handler(process);
  }
  return process;
}

EventPtr Simulation::all_of(std::initializer_list<EventPtr> events) {
  int n = 0;
  for (auto &event : events) {
    if (!event->is_triggered()) {
      ++n;
    }
  }

  auto process = start_process<Condition>(n);
  for (auto &event : events) {
    event->add_handler(process);
  }
  return process;
}

EventPtr Simulation::schedule(EventPtr event, double delay /* = 0.0 */) {
  queued_events.emplace(now + delay, next_id, event);
  ++next_id;
  return event;
}

bool Simulation::step() {
  if (queued_events.empty()) {
    return false;
  }

  auto queued_event = queued_events.top();
  queued_events.pop();
  now = queued_event.time;
  auto event = queued_event.event;
  event->fire();
  return true;
}

void Simulation::advance_by(double duration) {
  double target = now + duration;
  while (has_next() && peek_next_time() <= target) {
    step();
  }
  now = target;
}

void Simulation::advance_to(EventPtr event) {
  // TODO how to handle event abort?
  while (event->is_pending() && has_next()) {
    step();
  }
}

void Simulation::run() {
  while (step()) {
  }
}

double Simulation::get_now() { return now; }

bool Simulation::has_next() { return !queued_events.empty(); }

double Simulation::peek_next_time() { return queued_events.top().time; }

/* Simulation::QueuedEvent */

Simulation::QueuedEvent::QueuedEvent(double time, size_t id, EventPtr event)
    : time(time), id(id), event(event) {}

bool Simulation::QueuedEvent::operator<(const QueuedEvent &other) const {
  if (time != other.time) {
    return time > other.time;
  }

  return id > other.id;
}

/* Event */

Event::Event(SimulationPtr sim)
    : sim(sim), listeners(new std::vector<ProcessPtr>()) {}

bool Event::add_handler(ProcessPtr process) {
  if (is_triggered()) {
    return false;
  }

  if (is_pending()) {
    listeners->push_back(process);
  }

  return true;
}

void Event::trigger() {
  if (!is_pending()) {
    // TODO handle differently?
    return;
  }

  sim->schedule(shared_from_this());
  state = State::Triggered;
}

void Event::abort() {
  if (!is_pending()) {
    // TODO handle differently?
    return;
  }

  state = State::Aborted;
  Aborted();
}

void Event::fire() {
  if (is_aborted() || is_processed()) {
    return;
  }

  auto listeners = std::move(this->listeners);

  this->listeners = nullptr;
  state = State::Triggered;

  for (auto proc : *listeners) {
    proc->resume();
  }
}

bool Event::is_pending() { return state == State::Pending; }

bool Event::is_triggered() { return state == State::Triggered; }

bool Event::is_processed() { return listeners == nullptr; }

bool Event::is_aborted() { return state == State::Aborted; }

Event::State Event::get_state() { return state; }

void Event::Aborted() {}

/* Process */

Process::Process(SimulationPtr sim) : Event(sim), Protothread() {}

void Process::resume() {
  // Is the process already finished?
  if (!is_pending()) {
    return;
  }

  bool still_running = Run();

  // Did the process finish now?
  if (!still_running) {
    // Process finished
    trigger();
  }
}

ProcessPtr Process::shared_from_this() {
  return std::static_pointer_cast<Process>(Event::shared_from_this());
}

/* Condition */

Condition::Condition(SimulationPtr sim, int n) : Process(sim), n(n) {}

bool Condition::Run() {
  PT_BEGIN();
  while (n > 0) {
    PT_YIELD();
    --n;
  }
  PT_END();
}

} // namespace simcpp
