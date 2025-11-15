#include <iostream>
#include <fstream>
#include <string>
#include "swtor_parser.h"

int main(int argc, char** argv) {
  using namespace swtor;
  if (argc < 2) {
    std::cerr << "Usage: demo <combat_log.txt>\n";
    return 1;
  }
  std::ifstream in(argv[1], std::ios::binary);
  if (!in) {
    std::cerr << "Failed to open file: " << argv[1] << "\n";
    return 1;
  }

  std::string line;
  PrintOptions opt{};
  size_t n=0, ok=0;
  while (std::getline(in, line)) {
    CombatLine cl{};
    if (parse_line(line, cl)) {
      ++ok;
      std::cout << to_text(cl, opt) << "\n";
    }
    ++n;
  }
  std::cerr << "Parsed " << ok << " / " << n << " lines\n";
  return 0;
}
