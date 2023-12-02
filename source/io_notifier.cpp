#include "io_notifier.hpp"

#include <bits/stdint-uintn.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <ratio>

using namespace std;
using namespace chrono;
using namespace easykey;

/**
 * The flags that will be set when creating the poll system
 */
constexpr static int32_t POLL_SYSTEM_FLAGS = 0;

namespace easykey
{
/**
 *  The associated file is available for read(2) operations.
 */
const EventType EventType::READ(EPOLLIN);

/**
 * The associated file is available for write(2) operations.
 */
const EventType EventType::WRITE(EPOLLOUT);

/**
 * Stream socket peer closed connection, or shut down writing half of
 * connection. (This flag is especially useful for writing simple code to detect
 * peer shutdown when using edge-triggered monitoring.)
 */
const EventType EventType::CLOSE_CONNECTION(EPOLLRDHUP);

};  // namespace easykey

Event::Event(int32_t file_descriptor) : file_descriptor(file_descriptor)
{
    flags = 0;
}

Event &Event::add(const EventType &type)
{
    flags |= type.flag;
    return *this;
}

int32_t Event::get_flags() const
{
    return flags;
}

bool Event::has(const EventType &type) const
{
    return type.flag & flags;
}

bool Event::is_for(const int32_t file_descriptor) const
{
    return this->file_descriptor == file_descriptor;
}

IONotifier::IONotifier()
{
    file_descriptor = epoll_create1(POLL_SYSTEM_FLAGS);
    if (file_descriptor < 0)
    {
        const auto msg = "Could not create epoll instance!";
        cerr << msg << endl;
        throw msg;
    }
}

IONotifier::~IONotifier()
{
    if (tracked_file_descriptors_count != 0)
    {
        cerr << "Not all registered file descriptors were closed in this IO "
                "Notifier! They will be forcelly closed now!"
             << endl;
    }
    close(file_descriptor);
}

bool IONotifier::add_event(const Event event)
{
    struct epoll_event epoll_e;
    memset(&epoll_e, 0, sizeof(struct epoll_event));
    epoll_e.events = event.get_flags();

    /**
     * We always will request for Edge-Triggered notifications instead of the
     * default behavior of Level-Triggered The big difference, is that
     * Edge-Triggered notifies that there is data to be read, when the kernel
     * writes to the buffer file descriptor buffer if we dont read, or do not
     * read everything, it will not notify again. Unless, new data is written
     * ...
     */
    epoll_e.events |= EPOLLET;

    epoll_e.data.fd = event.file_descriptor;

    const bool success = epoll_ctl(file_descriptor,
                                   EPOLL_CTL_ADD,
                                   event.file_descriptor,
                                   &epoll_e);

    if (success)
    {
        tracked_file_descriptors_count++;
    }
    return success;
}

bool IONotifier::delete_event(const Event event)
{
    const bool success = epoll_ctl(file_descriptor,
                                   EPOLL_CTL_DEL,
                                   event.file_descriptor,
                                   nullptr);
    if (success)
    {
        tracked_file_descriptors_count--;
    }
    return success;
}

const vector<Event> IONotifier::wait_for_events(
    chrono::duration<uint64_t, nano> timeout)
{
    // TODO: Should not be a static value size.
    // Should grow/shrink, while new file descriptors are added/removed ...
    struct epoll_event events_buffer[32];

    const auto total_events = epoll_wait(
        file_descriptor,
        events_buffer,
        sizeof(events_buffer) / sizeof(events_buffer[0]),
        chrono::duration_cast<chrono::milliseconds>(timeout).count());

    if (total_events < 0)
    {
        cerr << "Epoll wait error!" << endl;
        return {};
    }
    if (total_events == 0)
    {
        cout << "Epoll Timeout!" << endl;
        return {};
    }

    vector<Event> events;
    events.reserve(total_events);
    for (int32_t index = 0; index < total_events; index++)
    {
        const auto returned = events_buffer[index];

        Event event(returned.data.fd);

        if (returned.events & EPOLLRDHUP)
        {
            event.add(EventType::CLOSE_CONNECTION);
        }
        else if (returned.events & EPOLLIN)
        {
            event.add(EventType::READ);
        }
        else if (returned.events & EPOLLOUT)
        {
            event.add(EventType::WRITE);
        }
        events.push_back(event);
    }
    return events;
}