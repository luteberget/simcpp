#include "simcpp.h"

/* Simulation::QueuedEvent */

Simulation::QueuedEvent::QueuedEvent(double time, size_t id,
                                     shared_ptr<Event> event)
    : time(time), id(id), event(event) {}

bool Simulation::QueuedEvent::operator<(const QueuedEvent &other) const {
  if (time != other.time) {
    return time > other.time;
  }

  return id > other.id;
}

/* Simulation */

shared_ptr<Simulation> Simulation::create() {
  return std::make_shared<Simulation>();
}

shared_ptr<Process> Simulation::run_process(shared_ptr<Process> process) {
  auto event = std::make_shared<Event>(shared_from_this());
  event->add_handler(process);
  schedule(event);
  return process;
}

shared_ptr<Event> Simulation::timeout(double delay) {
  auto event = std::make_shared<Event>(shared_from_this());
  schedule(event, delay);
  return event;
}

shared_ptr<Event> Simulation::schedule(shared_ptr<Event> event,
                                       double delay /* = 0.0 */) {
  queued_events.emplace(now + delay, next_id++, event);
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
  // TODO Does this keep the shared pointer in scope, making the
  // automatic memory management useless?
  // TODO Is this whole approach limited by the stack size?
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

void Simulation::advance_to(shared_ptr<Process> process) {
  while (!process->is_triggered() && has_next()) {
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

/* Event */

Event::Event(shared_ptr<Simulation> sim)
    : listeners(new vector<shared_ptr<Process>>()), sim(sim) {}

bool Event::add_handler(shared_ptr<Process> process) {
  if (is_processed()) {
    return false;
  }

  listeners->push_back(process);
  return true;
}

bool Event::is_processed() { return listeners == nullptr; }

int Event::get_value() { return value; }

void Event::fire() {
  auto listeners = std::move(this->listeners);

  this->listeners = nullptr;
  value = 0;

  for (auto proc : *listeners) {
    proc->resume();
  }
}

bool Event::is_triggered() { return value != -1; }

bool Event::is_success() { return value == 1; }

bool Event::is_failed() { return value == 2; }

/* Process */

Process::Process(shared_ptr<Simulation> sim) : Event(sim), Protothread() {}

void Process::resume() {
  // Is the process already finished?
  if (is_triggered()) {
    return;
  }

  bool run = Run();

  // Did the process finish now?
  if (!run) {
    // Process finished
    value = 1;
    sim->schedule(shared_from_this());
  }
}

void Process::abort() {
  value = 2; //?
  Aborted();
}

void Process::Aborted(){};

/* AnyOf */

AnyOf::AnyOf(shared_ptr<Simulation> sim, vector<shared_ptr<Event>> events)
    : Process(sim) {
  for (auto &event : events) {
    event->add_handler(shared_from_this());
  }
}

bool AnyOf::Run() {
  PT_BEGIN();
  PT_YIELD();
  // Any time we get called back, we are finished.
  PT_END();
}

/* AllOf */

AllOf::AllOf(shared_ptr<Simulation> sim, vector<shared_ptr<Event>> events)
    : Process(sim), events(events) {}

bool AllOf::Run() {
  PT_BEGIN();
  while (i < events.size()) {
    if (!events[i]->is_triggered()) {
      PROC_WAIT_FOR(events[i]);
      i++;
    }
  }
  PT_END();
}
