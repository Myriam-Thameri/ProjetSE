# Base Ubuntu image
FROM ubuntu:22.04

# Avoid interactive prompts
ENV DEBIAN_FRONTEND=noninteractive

# Install C tools
RUN apt-get update && apt-get install -y \
    build-essential \
    gcc \
    g++ \
    make \
    gdb \
    nano \
    git \
    && apt-get clean

# Set working directory inside container
WORKDIR /projetSE

# Copy project files into the container
COPY . /projetSE

# Default command when container starts
CMD ["/bin/bash"]
