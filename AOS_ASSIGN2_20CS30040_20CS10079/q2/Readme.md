# Without docker

## Compiling and running the server and LB

`cd server`

`gcc server.c && ./a.out 5051`

`gcc server.c && ./a.out 5052`

`gcc server.c && ./a.out 5053`

`clang -O2 -g -Wall -target bpf -c xdp.c -o xdp.o`

`mount -t bpf none /sys/fs/bpf`

`xdp-loader load -m skb -s xdp eth0 xdp.o`

## Compiling the client

`cd client`

`gcc client.c && ./a.out 127.0.0.1`

# With docker

## Running the server and lb

`docker build -t server .`

`docker create network mynet`

`docker run -it --name server1 --network mynet --privileged server`

`mount -t bpf none /sys/fs/bpf`

`xdp-loader load -m skb -s xdp eth0 xdp.o`

Spawn three instance of terminal of container server 1 using this command

`docker exec -it server1 bash`

Run these in each of them

`gcc server.c && ./a.out 5051`

`gcc server.c && ./a.out 5052`

`gcc server.c && ./a.out 5053`

## Running the client

`docker build -t client .`  

`docker run -it --name client1 --network mynet --privileged client`

Run docker inspect mynet to get the server ip and run client with 

`./client <server_ip>`

# Testing
Everything along with some additional info for better debugging has been printed. Also performed the testing with high sleep times and low sleep times. There is macro named SLEEP_FACTOR on line 17 in server.c which can be used to control the sleep duration.
