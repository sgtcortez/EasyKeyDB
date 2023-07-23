# This is the build stage, just used to build the application.
FROM alpine:3.18.0 AS build_stage

# https://pkgs.alpinelinux.org/packages?branch=v3.18&repo=&arch=x86_64
RUN apk update && \
    apk add --no-cache cmake build-base clang-dev && \
    ln -sf /usr/bin/clang /usr/bin/cc && \
    ln -sf /usr/bin/clang++ /usr/bin/c++     

# Define our Easy Key Directory
WORKDIR /easykey

# Copy the necessary contents
COPY include ./include
COPY source ./source
COPY main.cpp .
COPY CMakeLists.txt .

# Compile our code with release flags
RUN cmake -B build/ -DCMAKE_BUILD_TYPE=Release && cmake --build build/ 

# This is the runtime stage, the image has few libraries, so it will have  a smaller size
FROM alpine:3.18.0 AS runtime_stage

# Installs standard c++ library
RUN apk update && apk add --no-cache libstdc++

# Add group, add user and specify it to the group that we created before
RUN addgroup -S ramboindustries && adduser -S easykey -G ramboindustries

# Set as running with user 
USER easykey

# Copy the binary output from the previous build
COPY --chown=ramboindustries:easykey --from=build_stage \
    ./easykey/build/easykeydb \
    ./easykey

# Define our entrypoint
ENTRYPOINT ["./easykey"]