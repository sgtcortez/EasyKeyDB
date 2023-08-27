#!/usr/bin/python3

import os
from os.path import join, getsize
from sys import exit

from_directory = input("Directory to traverse and get files: ")
easykeyv1_write_bin = "./cli-write.out"

files_to_import = {}
index = 0

for root, directories, files in os.walk(from_directory):
    for file in files:
        index += 1

        # It will traverse recursively the directory tree 
        # The root is the current directory, files are the files found in this path
        pid = os.fork() 
        if pid > 0:
            # In the parent process
            # Just wait for the child process to finish
            pid, status = os.waitpid(pid, 0)
            if status != 0:
                print(f"An error occured with child pid: {pid}, status: {status}")
                exit(1)
        else:
            # In the child process
            key = "file_" + str(index)
            path = "{}/{}".format(root, file)

            # https://docs.python.org/3/library/os.html#os.execv
            print("Execute execl: {} {} {} {}".format(easykeyv1_write_bin, key, path, "--file=yes"))
            os.execl(easykeyv1_write_bin, easykeyv1_write_bin, key, path, "--file=yes")