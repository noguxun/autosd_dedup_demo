FROM fedora:42

# Install build dependencies
RUN dnf install -y gcc make


# Set working directory
WORKDIR /build

# Copy the source file
COPY libwaitinput.c ./


# Create target directory and compile the shared library
RUN mkdir -p _target && \
    gcc -fPIC -shared -o _target/libwaitinput.so libwaitinput.c

# Set up volume mount point for output
VOLUME ["/build/_target"]

# Default command - just keep container running so we can copy files
CMD ["sleep", "1"] 