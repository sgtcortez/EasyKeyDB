#include "easykey.hpp"
#include "know_nothing.hpp"
#include "server.hpp"
#include "database.hpp"

#include <bits/stdint-uintn.h>
#include <cstdint>
#include <iostream>
#include <unordered_map>
#include <string>
#include <vector>
#include <csignal>

using namespace easykey;
using namespace std;
using namespace knownothing;

/**
 * The signal handler 
 * man 7 signal for standard signals
*/
void signal_handler(int32_t signal);

void gracefully_termination();

Server* server_ptr = nullptr;

struct DemoEasyKeyDispatcher : public Dispatcher
{

  private: 
    Database database;

  public:

    string name() const
    {
      return "DemoEasyKeyDispatcher";
    }

    vector<uint8_t> exchange(vector<uint8_t> client_request)
    {
      auto request = RequestMessage::read(client_request);

      if (request == nullptr)
      {
          string output_message = "The message does not follow the KnowNothing Specification!";
          vector<uint8_t> output(output_message.begin(), output_message.end());
          return ResponseMessage{Protocol::V1, ResponseMessage::SERVER_ERROR, output}.write();        
      } 

      // If its a read operation, the value will be empty
      if (request->value.size() == 0) 
      {
        // Its a read operationn
        const auto result = database.read(request->key);
        if (result.empty())
        {
          string output_message = "Key \"" + request->key + "\" was not found!";
          vector<uint8_t> output(output_message.begin(), output_message.end());
          return ResponseMessage{Protocol::V1, ResponseMessage::CLIENT_ERROR, output}.write();
        }
        return  ResponseMessage{Protocol::V1, ResponseMessage::OK, result}.write();
      }
      else 
      {
        // its a write operation
        database.write(request->key, request->value);
        return ResponseMessage{Protocol::V1, ResponseMessage::OK, {'O', 'K', '!'}}.write();
      }
  }
};


int main() {

  auto dispatcher = DemoEasyKeyDispatcher();
  auto server = Server(9000, 10, dispatcher);
  server.start();

  server_ptr = &server;

  // https://en.cppreference.com/w/cpp/utility/program/signal
  /**
   * Register the signals to be handled!
  */
  signal(SIGTERM, signal_handler);
  signal(SIGINT, signal_handler);

  return 0;
}

void signal_handler(int32_t signal)
{
  switch (signal) {
    case SIGINT:
    case SIGTERM:
      gracefully_termination();
      break;
    default:
      throw "Signal: " + to_string(signal) + " catched is not being handled!";
  }
}

void gracefully_termination()
{
  server_ptr->stop();
}