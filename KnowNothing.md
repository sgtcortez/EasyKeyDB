# Summary

- [KnowNothing Protocol](#KnowNothing-Protocol)
- [Introduction](#Introduction)
- [Specification](#Specification)
    - [Version 1](#version-1)
- [References](#References)

# KnowNothing Protocol

***KnowNothing/KN Protocol*** is a **Know Nothing** about the *"data"*.    
The name is pretty simple, we do not know nothing about the data, so, we do not impose limits, validations(at protocol level) and so on.   
It is just a raw binary protocol.  
Note: We do not impose any limit at the protocol level, but, validations, limits can easily be included in messages. So, it always depends on your usage.

So, want to send utf-8?, utf-16?, ascii, integers? No problem, just send it. 

# Introduction

The protocol is quite simple.  
The messages are always in *"byte array"*.

The protocol aims to provide a fast, easy, scalable and flexible protocol, which can grow, have user extensions and so on ...

# Specification

At first, we provide a easy way to exchange data.   
This protocol is meant to be used as client-server.  
So, the client sends a request, the servers reply with an appropriate response.

By convencience, and aiming to create something really flexible, the first byte will always be the protocol version.  
So, we can always modify the entire protocol message structure, without breaking everything, or doing something too complex in the client and server parsers.

For now, there is only one avaialbe version.   
| Version | Value | Description |
| :-: | :-: | :-: |
| 1 | `0x01` | The first version of the protocol, can exchange only messages |

The protocol follows the **Little Endian byte storage**. This means that the least significant bytes(LSB) are stored from right to the left.

As an example, there is this [**gist**](https://gist.github.com/sgtcortez/b51df067e864c2003b8243491bff21cb) 
which describes how to store a variant size number(1 byte, 2 bytes, 4bytes, 8bytes ...) into a byte array, than restore it.   

In **C**, we can use the bitwise arithmetic   
```c
void serialize(uint8_t *array, uint64_t number, uint8_t number_size) 
{
    for (uint8_t index = 0; index < number_size; index++)
    {
        const int8_t shift_by = index * 8;
        // Since we use only the least significant 8 bits of the value(we do this by just using uint8_t)
        // there is no need to do (number >> shift_by) & 0xFF to get the last 8 bits
        const uint8_t result = number >> shift_by ;
        array[index] = result;
    }
}
```

And to restore it:   
```c
uint64_t deserialize(uint8_t *array, uint8_t number_size)
{
    uint64_t value = 0;
    for (uint8_t index = 0; index < number_size; index++)
    {
        const int8_t shift_by = index * 8;
 
        // need to store it as uint64_t ... 
        // otherwise the next shift operation could overlflow if the shift_by is too big
        uint64_t byte_value = array[index];
        uint64_t result = byte_value << shift_by;
        value |= result;
    }
    return value;
}
```

- ## Version 1

    The first version, allows the exchange of messages.  

    **"Byte array schema definition"**   
    | Byte | Description |
    | :-: | :- |
    | **`0x00`** | Is always 0x01. Specify the protocol version |
    | **`0x01-0x05`** ... N | The number of bytes used by the messages. Maximum message size: 2²²-1(4MiB) |

    This is quite good, because there is no overhead to add control characteres ... 
    And also, this allows us to create the buffers with the dedicated size for them.   
    So, we can 
    It's just a *"byte array"*.

    So for example, the following byte array message:    
    `0x01 0x02 0x00 0x00 0x00 0x05 0x00 0x00 0x00 0x30 0x31 0x40 0x41 0x42 0x43 0x44`    
    
    The message goes until the last byte of the stream.

    The meaning is:  
    | Index | Byte | Description |
    | :-: | :-: | :- |
    | 0 | **`0x01`** | The "protocol" version |
    | 1 | **`0x02`** | The *"first"* byte of the four byte size of the first message |     
    | 2 | **`0x00`** | The *"second"* byte of the four byte size of the first message |     
    | 3 | **`0x00`** | The *"third"* byte of the four byte size of the first message |     
    | 4 | **`0x00`** | The *"fourth"* byte of the four byte size of the first message |     
    | 5 | **`0x05`** | The *"first"* byte of the four byte size of the second message |     
    | 6 | **`0x00`** | The *"second"* byte of the four byte size of the second message |     
    | 7 | **`0x00`** | The *"third"* byte of the four byte size of the second message |     
    | 8 | **`0x00`** | The *"fourth"* byte of the four byte size of the second message |     
    | 9 | **`0x30`** | The *"first byte"* of *"two"* bytes from the *"first"* the message |
    | 10 | **`0x31`** | The *"second byte"* of *"two"* bytes from the *"first"* message |
    | 11 | **`0x40`** | the *"first byte"* of *"five"* bytes from the *"second"* message |
    | 12 | **`0x41`** | the *"second byte"* of *"five"* bytes from the *"second"* message |
    | 13 | **`0x42`** | the *"third byte"* of *"five"* bytes from the *"second"* message |
    | 14 | **`0x43`** | the *"fourth byte"* of *"five"* bytes from the *"second"* message |
    | 15 | **`0x44`** | the *"fifth byte"* of *"five"* bytes from the *"second"* message |

    You can send a batch of messages, and also, just one message. It always depends on your usage.

# References

Learning and inspirations

- [Redis Protocol](https://redis.io/docs/reference/protocol-spec/)
- [Endianness](https://en.wikipedia.org/wiki/Endianness)
- [MessagePack](https://github.com/msgpack/msgpack/blob/master/spec.md)
- [BSON](https://bsonspec.org/spec.html)
- [Protocol Buffers](https://github.com/protocolbuffers/protobuf)