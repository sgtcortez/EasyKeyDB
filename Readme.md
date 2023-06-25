# Sumamry 

- [Introduction](#Introduction)
- [References](#References)
    + [Files per partition/directory](#files-per-partitiondirectory)
    + [sendfile zero copy](#sendfile-zero-copy)
    + [Kafka Persistence](#kafka-persistence)

# Introduction

A simple key-value database.    
For now, this project is just for learning purposes.    

With this project, I want to make some efforts to really learn C++ programming language.  
Make some nice networking code and operating systems code.  

# References

Above, some references that helped to create this project.   

- # [Files per partition/directory](https://unix.stackexchange.com/questions/239146/linux-folder-size-limit#:~:text=ext4%3A-,Maximum%20number%20of%20files%3A%20232%20%2D%201%20(4%2C294%2C967%2C295),of%20files%20per%20directory%3A%20unlimited)
- # [sendfile zero copy](https://man7.org/linux/man-pages/man2/sendfile.2.html)

    I have an idea to store the contents(keys and values) at files.   
    So we can use fast retrieval using **zero copy** technique.

- # [Kafka Persistence](https://kafka.apache.org/documentation/#persistence)

    Kafka stores topic messages at disk, and still is very performant ...   
    So, their ideas can help me.