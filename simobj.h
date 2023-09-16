#ifndef SIMOBJ_H_
#define SIMOBJ_H_

#include "simcpp.h"

#define OBSERVABLE_PROPERTY(TYP, NAM, VAL)                                     \
private:                                                                       \
  TYP NAM = VAL;                                                               \
                                                                               \
public:                                                                        \
  simcpp::EventPtr NAM##_event{std::make_shared<simcpp::Event>(env)};          \
  TYP get_##NAM() const { return this->NAM; }                                  \
  void set_##NAM(TYP v) {                                                      \
    this->NAM = v;                                                             \
    this->env->schedule(this->NAM##_event);                                    \
    this->NAM##_event = std::make_shared<simcpp::Event>(env);                  \
  }


namespace simcpp {

class EnvObj {
protected:
    SimulationPtr env;

public:
    explicit EnvObj(const SimulationPtr& s) : env(s) {}
    virtual ~EnvObj() = default;
};

}


#endif
