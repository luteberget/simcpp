#ifndef __TVD_H
#define __TVD_H

#include "infrastructure_object.h"
#include "resource.h"
#include "../simobj.h"
typedef shared_ptr<Simulation> Sim;

class TVD : public Resource {
  OBSERVABLE_PROPERTY(bool, occupied, false)
public:
  TVD(Sim s) : Resource(s) {}
};

#endif