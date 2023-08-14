# Sumamry 

- [Introduction](#Introduction)
- [Configuration](#Configuration)
    - [Clang](#Clang)
- [Under the Hood](#Under-the-Hood)
- [Running](#Running)
- [References](#References)
    + [Files per partition/directory](#files-per-partitiondirectory)
    + [sendfile zero copy](#sendfile-zero-copy)
    + [Kafka Persistence](#kafka-persistence)

# Introduction

A simple key-value database.    
For now, this project is just for learning purposes.    

With this project, I want to make some efforts to really learn C++ programming language.  
Make some nice networking code and operating systems code.  

# Configuration

In the sections above, there are some explanations and mini tutorials in how to configure and run this project.  
Also, we follow the **Clang Format** coding standard ... So, if you write some code that does not follow this guideline, it will break in the **github action**.

- ## CMake

    I do prefer to use **Makefiles** to compile a simple C/C++ code ...   
    But, sometimes, when we want to make some portable code, it is not a good choice ...  

    So, to make this code portable, I choose **Cmake** for  this project ...  

    [Here](https://cmake.org/cmake/help/latest/guide/tutorial/index.html) is some tutorial provided by the Company.   
    [Here](https://github.com/ttroy50/cmake-examples) is some really nice examples

    For basic compile and run, you can use at the project root directory:  
    ```shell
    cmake -B build/ -DCMAKE_BUILD_TYPE=Debug  
    cmake --build build/
    ```

    This will build the project using debug flags.

- ## Clang

    I am a fan of LLVM tools, so, if you prefer clang over gcc or msvc, build this project will be easier.

    - [ClangD](https://releases.llvm.org/8.0.0/tools/clang/tools/extra/docs/clangd/index.html)

        So, here I am using **ClangD**.
        The file **compile_flags.txt** at the root directory tells the **clangd server** how the project is built.  

    - [Clang-Tidy](https://clang.llvm.org/extra/clang-tidy/)  

        We are using the clang-tidy to find bugs and typical programming errors.

    - [Clang-Format](https://clang.llvm.org/docs/ClangFormat.html)

        We have an github action which checks if the code follows the **LLVM** coding standard.    
        You can check by issuing the command: `clang-format -style=llvm <file>`


# Under the Hood

To understand how this program works, and how we can acomplish a reasonable response time, we need to undestand some aspects ...   

- Disks

    Hard Disk Drive and Solid State Disk are slower than main memory when we need to do a **Random Access(i.e: access some portion in the middle of the file)**, because it requires a lot of **disk seeks**.   
    So, accessing random portions is not performant ...   

    But, in hdd & ssd, they can contain much more memory(this is how they are built. Transistors and related stuff).  
    So, how we can use the benefits of the both worlds? **Page Cache**. 

- Page Cache

    In Linux, all the **main memory(RAM)** that is not being used, would be wasted ...   
    But, the operating system is smarter enough ...  
    To let we do this, so it makes the unused portion of **main memory** as the Page Cache ... 

    Page Cache, is just some portion of files(bytes) that are frequently in use.    
    So, instead of doing a **random access** to the file, we do this to the page cache ...   
    Which, in turns, turns out to be the **main memory** ...

- Input/Output Multiplexing

    Use one thread for every socket that comes in, can be wastefull and heavy ...   
    So, instead of creating a new thread every time, we multiplex the a *thread* which runs the **TCP server**, which will listen(man 2 listen) for incoming requests, messages and disconnections.    
    So, when the kernel signals us(a new client, we can read from a file descriptor), then, we process the request.  

    For linux, we are using the **Epoll Functions**.   

    So, basically, instead of using the one thread to execute the **accept**(to accept new tcp clients), and another "N"(depending of the number of clients(accepted) connected) threads to execute the **read** function, we can to both of them, with epoll.   

    So, we have an **infinite loop** which, waits for the **epoll** systemcall return, then, we process the events.   
    If there is a client awaiting to be accepted, we accept, if there is a client which have sent data, we read from it and so on ...

# Running

To run this project, since we havely use the file system, and not too much main memory.   
Would be nice to run in an enviroment with a lot of available ram(to make more and more **page caches**) and a huge fast filesystem and disk.   
With this, we can use the benefit of zero copy(sendfile) with **page caches**.   
We, as **Kafka** can be benefit from Page caches, since the most frequently used *pages* will be stored at memory ram, our time access reduces drastically.

# References

Above, some references that helped to create this project.   

- # [Files per partition/directory](https://unix.stackexchange.com/questions/239146/linux-folder-size-limit#:~:text=ext4%3A-,Maximum%20number%20of%20files%3A%20232%20%2D%201%20(4%2C294%2C967%2C295),of%20files%20per%20directory%3A%20unlimited)
- # [sendfile zero copy](https://man7.org/linux/man-pages/man2/sendfile.2.html)

    I have an idea to store the contents(keys and values) at files.   
    So we can use fast retrieval using **zero copy** technique.

- # [Kafka Persistence](https://kafka.apache.org/documentation/#persistence)

    Kafka stores topic messages at disk, and still is very performant ...   
    So, their ideas can help me.

- # [Why is Kafka so fast if it uses disk](https://andriymz.github.io/kafka/kafka-disk-write-performance/#)
