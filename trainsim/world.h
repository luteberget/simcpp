#ifndef __WORLD_H
#define __WORLD_H

#include <vector>
#include <unordered_map>
#include <memory>
using std::vector;
using std::unordered_map;
using std::unique_ptr;
using std::string;

#include "objects/infrastructure_object.h"
#include "objects/route.h"
#include "objects/train.h"
#include "history.h"

class World {
public:
  // Infrastructure
  vector<unique_ptr<ISObj>> objects;

  // Interlocking
  unordered_map<string, Route> routes;

  // Trains
  //vector<Train> trains;

  shared_ptr<Simulation> sim;
  unique_ptr<OutputWriter> output;

  static World Create(InfrastructureSpec &is, shared_ptr<Simulation> sim, unique_ptr<OutputWriter> output);
};

#endif