#include "il_inputparser.h"
#include <iostream>

Direction parse_dir(std::istream& input) {
  string dir; input >> dir;
  if(dir == "up") return Direction::Up;
  if(dir == "down") return Direction::Down;
  throw "Direction parse error";
}

SwitchState parse_switchstate(std::istream& input) {
  string state; input >> state;
  if(state == "left") return SwitchState::Left;
  if(state == "right") return SwitchState::Right;
  throw "Switch state parse error";
}

LinkSpec parse_link(std::istream& input) {
  size_t idx; input >> idx;
  double length; input >> length;
  return { idx, length };
}

SignalSpec parse_signal(std::istream& input) {
  auto dir = parse_dir(input);
  size_t detector; input >> detector;
  return { dir, detector };
}

SightSpec parse_sight(std::istream& input) {
  size_t sig; input >> sig;
  double dist; input >> dist;
  return { sig, dist };
}

DetectorSpec parse_detector(std::istream& input) {
  int upTVD; input >> upTVD;
  int downTVD; input >> downTVD;
  return { upTVD, downTVD };
}

SwitchSpec parse_switch(std::istream& input) {
  return { parse_switchstate(input) };
}

pair<size_t, RouteSpec> parse_route(std::istream& input) {
  RouteSpec r;
  return std::make_pair(0, r);
}

ISObjSpec parse_obj(std::istream& input) {
  ISObjSpec obj;
  input >> obj.name; 
  string type; input >> type;
  int n_up; input >> n_up; obj.n_up = n_up;
  int n_down; input >> n_down; obj.n_down = n_down;
  for(size_t i = 0; i < obj.n_up; i++) obj.up[i] = parse_link(input);
  for(size_t i = 0; i < obj.n_down; i++) obj.down[i] = parse_link(input);
  if(type == "signal") obj.signal = parse_signal(input);
  if(type == "detector") obj.detector = parse_detector(input);
  if(type == "sight") obj.sight = parse_sight(input);
  if(type == "switch") obj.sw = parse_switch(input);
  if(type == "boundary") obj.type = ISObjSpec::ISObjType::Boundary;
  if(type == "stop") obj.type = ISObjSpec::ISObjType::Stop;
  if(type == "tvd") obj.type = ISObjSpec::ISObjType::TVD;
  return obj;
}

InfrastructureSpec parse_infrastructure(std::istream& input) {
  InfrastructureSpec is;

  string type = "";
  do {
    type = ""; input >> type;
    if(type == "obj") is.driveGraph.push_back(parse_obj(input));
    if(type == "route") { auto x = parse_route(input); is.routes[x.first] = x.second; }
  } while (type != "");

  if(input.fail() || !input.eof()) throw "InfrastructureSpec parse error";
  return is;
}

