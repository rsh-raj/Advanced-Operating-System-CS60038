FROM ubuntu:latest
RUN apt-get update && \
apt-get install -y build-essential git cmake
RUN mkdir /src
WORKDIR /src

COPY ./client.c /src/client.c

RUN gcc /src/client.c -o /src/client

CMD ["/bin/bash"]
