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

  const auto message = RequestMessage::read(byte_array);
  cout << message.key << endl;

  vector<uint8_t> temp;
  for (int index = 0; index < 66000; index++) {
    temp.push_back(index);
  }

  const auto responseMessage =
      ResponseMessage{Protocol::V1, ResponseMessage::StatusCode::OK, temp};

  const auto responseBytes = responseMessage.write();
  for (unsigned int index = 0; index < responseBytes.size(); index++) {
    uint8_t v = responseBytes[index];

    cout << "Index: [" << to_string(index) << "]"
         << " - " << to_string(v) << endl;
  }
  cout << endl;
  return 0;
}