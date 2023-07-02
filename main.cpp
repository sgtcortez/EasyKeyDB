#include "easykey.hpp"
#include "know_nothing.hpp"

#include <bits/stdint-uintn.h>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

using namespace easykey;
using namespace std;
using namespace knownothing;

int main() {

  // Client sends this data
  uint8_t byte_array[] = {
      0x01, // knownothing protocol version
      0x02, // The number of messages in the content
      0x04, // The first message size first byte
      0x00, // The first message size second byte
      0x00, // The first message size third byte
      0x00, // The first message size fourth byte
      'n',  // The first message first byte
      'a',  // the first message second byte
      'm',  // The first message third byte
      'e',  // The first message fourth byte
      0x07, // The second message size first byte
      0x00, // The second message size second byte
      0x00, // The second message size third byte
      0x00, // The second message size fourth byte
      'M',  // The second message first byte
      'a',  // The second message second byte
      't',  // The second message third byte
      'h',  // The second message fourth byte
      'e',  // The second message fifth byte
      'u',  // The second message sixth byte
      's'   // The second message seventh byte
  };

  vector<uint8_t> vec(
      byte_array, byte_array + (sizeof(byte_array) / sizeof(byte_array[0])));

  const auto message = RequestMessage::read(vec);
  cout << message.key << endl;

  return 0;
}