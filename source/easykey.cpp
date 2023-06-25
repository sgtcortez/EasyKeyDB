#include "easykey.hpp"

#include <iostream>

using namespace easykey;
using namespace std;

void easykey::print_ok_message() {
  cout << "Hello, Easy Key Configured!" << endl;
}

int main() {
  print_ok_message();
  return 0;
}
