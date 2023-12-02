#pragma once

#include <cstdint>
#include <vector>
#include <chrono>

namespace easykey
{

struct EventType
{
  private:
    EventType(const std::int32_t flag):flag(flag){};
  public:    
    const std::int32_t flag;

    const static EventType READ;
    const static EventType WRITE;
    const static EventType CLOSE_CONNECTION;

};

struct Event
{
  Event(const std::int32_t file_descriptor);
  const std::int32_t file_descriptor;
  public:

    /**
    * Add an event type
    */
    Event& add(const EventType& type);
 
    /**
     * Gets the current flags set
    */
    std::int32_t get_flags() const;

    /**
     * Checks if a flag is set
    */
    bool has(const EventType& type) const;

    /**
      * Checks if the event happened for a specific file descriptor
    */
    bool is_for(const std::int32_t file_descriptor) const;
  private:
    std::int32_t flags;

};

class IONotifier
{
  public:
    IONotifier();
    ~IONotifier();
    bool add_event(const Event event);
    bool delete_event(const Event event);
    const std::vector<Event> wait_for_events(std::chrono::duration<std::uint64_t, std::nano> timeout);
  private:
    std::uint32_t file_descriptor;

    /**
     * How many file descriptors are registered
    */
    std::uint32_t tracked_file_descriptors_count;
};

};