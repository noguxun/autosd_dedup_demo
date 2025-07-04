# Stage 1: Build stage
FROM fedora:42 AS builder

# Install build dependencies including GCC and static libraries
RUN dnf install -y gcc make

# Set working directory
WORKDIR /build

# Copy source files
RUN mkdir -p _target
COPY mem_chk_map.c Makefile  ./
COPY _target/libwaitinput.so ./_target/

# Build the executable only, do not build the library
RUN make exe


# Stage 2: Runtime stage - minimal image
FROM fedora:42 AS runtime

WORKDIR /app

COPY --from=builder /build/_target/libwaitinput.so ./

ARG EXEC
ENV EXEC=${EXEC}
COPY --from=builder /build/_target/${EXEC} ./

# Make sure the executable has proper permissions
RUN chmod +x ./${EXEC}

ENV LD_LIBRARY_PATH=.
CMD ["sh", "-c", "./${EXEC}"]
