#include "parser.h"
#include <iostream>
#include <string>
using std::string;

string parse_string(std::istream& input) {
  string x = ""; input >> x;
  if(input.fail() || x == "") throw string("Expected string.\n");
  return x;
}

bool parse_string_optional(std::istream& input, string& s) {
  string x = ""; input >> x;
  if(input.fail() || x == "") return false;
  s = x;
  return true;
}

int parse_int(std::istream& input) {
  int x; input >> x;
  if(input.fail()) throw string("Expected integer.\n");
  return x;
}

size_t parse_size(std::istream& input) {
  int x; input >> x;
  if(input.fail() || x < 0 ) throw string("Expected positive integer.\n");
  return (size_t)x;
}

double parse_double(std::istream& input) {
  double x; input >> x;
  if(input.fail()) throw string("Expected number.\n");
  return x;
}

void expect(std::istream& input, string s) {
  string x; input >> x; 
  if(x != s) throw (string("Expected ") + s + string (" -- got ") + x);
}

Direction parse_dir(std::istream& input) {
  string dir = parse_string(input);
  if(dir == "up") return Direction::Up;
  if(dir == "down") return Direction::Down;
  throw string("Direction parse error");
}

SwitchState parse_switchstate(std::istream& input) {
  string state = parse_string(input);
  if(state == "left") return SwitchState::Left;
  if(state == "right") return SwitchState::Right;
  throw string("Switch state parse error");
}

LinkSpec parse_link(std::istream& input) { return { parse_size(input), parse_double(input) }; }
SignalSpec parse_signal(std::istream& input) { return { parse_dir(input), parse_size(input) }; }
SightSpec parse_sight(std::istream& input) { return { parse_size(input), parse_double(input) }; }
DetectorSpec parse_detector(std::istream& input) { return { parse_int(input), parse_int(input) }; }
SwitchSpec parse_switch(std::istream& input) { return { parse_switchstate(input) }; }

ISObjSpec parse_obj(std::istream& input) {
  ISObjSpec obj;
  obj.name = parse_string(input);
  obj.n_up = parse_size(input);
  for(size_t i = 0; i < obj.n_up; i++) obj.up[i] = parse_link(input);
  obj.n_down = parse_size(input);
  for(size_t i = 0; i < obj.n_down; i++) obj.down[i] = parse_link(input);
  string type = parse_string(input);
  printf("object type %s\n", type.c_str());
  if(type == "signal")  {
    obj.type = ISObjSpec::ISObjType::Signal;
    obj.signal = parse_signal(input); }
  else if(type == "detector") {
    obj.type = ISObjSpec::ISObjType::Detector;
    obj.detector = parse_detector(input); }
  else if(type == "sight") {
    obj.type = ISObjSpec::ISObjType::Sight;
    obj.sight = parse_sight(input); }
  else if(type == "switch") {
    obj.type = ISObjSpec::ISObjType::Switch;
    obj.sw = parse_switch(input); }

  else if(type == "boundary") obj.type = ISObjSpec::ISObjType::Boundary;
  else if(type == "stop") obj.type = ISObjSpec::ISObjType::Stop;
  else if(type == "tvd") obj.type = ISObjSpec::ISObjType::TVD;
 
  else throw string("Unknown object type: ") + type;

  return obj;
}

pair<string, RouteSpec> parse_route(std::istream& input) {
  string name = parse_string(input);
  size_t signal = parse_size(input);
  double length = parse_double(input);
  size_t n_tvds = parse_size(input);
  vector<size_t> tvds;
  for(size_t i = 0; i < n_tvds; i++) {
    tvds.push_back(parse_size(input));
  }
  size_t n_switches = parse_size(input);
  vector<pair<size_t, SwitchState>> switches;
  for(size_t i = 0; i < n_switches; i++) {
    switches.push_back({ parse_size(input), parse_switchstate(input) });
  }

  return {name, { signal, tvds, switches, {}, length}};
}

pair<string, ReleaseSpec> parse_release(std::istream& input) {
  string name = parse_string(input);
  size_t trigger = parse_size(input);
  size_t n_resources = parse_size(input);
  vector<size_t> resources;
  for(size_t i = 0; i < n_resources; i++) resources.push_back(parse_size(input));
  return { name, { trigger, resources }};
}

InfrastructureSpec parse_infrastructure(std::istream& input) {
  InfrastructureSpec is;

  string type;
  while(parse_string_optional(input, type)) {
    if(type == "obj") is.driveGraph.push_back(parse_obj(input));
    else if(type == "route") { 
      auto route = parse_route(input);
      is.routes.insert({route.first,route.second});
    }
    else if(type == "release") {
      auto release = parse_release(input);
      is.routes[release.first].releases.push_back(release.second);
    } else if(type == "plan") {
      break;
    }
    else throw string("Unexpected: ") + type;
  }
  return is;
}

TrainRunSpec parse_traindata(std::istream& input) {
  // train 100.0 train1
  //   acc brk vel len
  //   dir auth
  //   boundary_id
  return { { parse_double(input), parse_double(input), 
             parse_double(input), parse_double(input) }, // LinearTrainParam
           parse_dir(input), parse_double(input), 
           { parse_size(input), 0.0 }, {} };
}

Plan parse_plan(std::istream& input) {
  Plan plan;

  string type;
  while(parse_string_optional(input, type)) {
    if(type == "setroute") {
      plan.items.push_back( { Plan::ItemType::Route, parse_double(input), parse_string(input) } );
    }
    else if(type == "train") {
      plan.items.push_back( { Plan::ItemType::Train, parse_double(input), 
                                   parse_string(input), parse_traindata(input) } );
      auto& item = plan.items[plan.items.size()-1];
      printf("Parsed train l=%g, auth=%g, startnode=%lu\n", 
        item.trainData.params.length, 
        item.trainData.startAuthorityLength,
        item.trainData.startLoc.obj);
    }
    else throw string("Unexpected: ") + type;
  }
  return plan;
}

SimulatorInput parse_simulatorinput(std::istream& input) {
  return { parse_infrastructure(input), parse_plan(input) };
}


