#include "il_inputparser.h"
#include <iostream>

int main() {
  try {
  auto s = parse_simulatorinput(std::cin);
  printf("got %lu objects.\n", s.infrastructure.driveGraph.size());
  printf("got %lu routes.\n", s.infrastructure.routes.size());
  printf("got %lu plan items.\n", s.plan.items.size());
  } catch (string s){
    printf("Parse failed: %s\n", s.c_str());
  }
}
