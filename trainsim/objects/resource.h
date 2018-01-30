#ifndef __RESOURCE_H
#define __RESOURCE_H

#include "infrastructure_object.h"
#include "../simobj.h"
typedef shared_ptr<Simulation> Sim;

class Resource : protected EnvObj, public ISObj
{
  OBSERVABLE_PROPERTY(bool, allocated, false)
public:
  Resource(Sim s) : EnvObj(s) {}
};

#endif