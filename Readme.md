# Sumamry 

- [Introduction](#Introduction)
- [Configuration](#Configuration)
    - [Clang](#Clang)
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


# References

Above, some references that helped to create this project.   

- # [Files per partition/directory](https://unix.stackexchange.com/questions/239146/linux-folder-size-limit#:~:text=ext4%3A-,Maximum%20number%20of%20files%3A%20232%20%2D%201%20(4%2C294%2C967%2C295),of%20files%20per%20directory%3A%20unlimited)
- # [sendfile zero copy](https://man7.org/linux/man-pages/man2/sendfile.2.html)

    I have an idea to store the contents(keys and values) at files.   
    So we can use fast retrieval using **zero copy** technique.

- # [Kafka Persistence](https://kafka.apache.org/documentation/#persistence)

    Kafka stores topic messages at disk, and still is very performant ...   
    So, their ideas can help me.
