
FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    build-essential \
    gcc \
    g++ \
    make \
    gdb \
    nano \
    git \
    && apt-get clean

WORKDIR /projetSE

COPY . /projetSE

CMD ["/bin/bash"]
