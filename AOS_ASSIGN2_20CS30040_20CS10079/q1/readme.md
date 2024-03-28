# Without docker
`cd server`

`gcc server.c && ./a.out`

`mount -t bpf none /sys/fs/bpf`

`xdp-loader load -m skb -s xdp eth0 xdp.o`

`cd client`

`gcc client.c ./a.out`

# With docker

## Running the server
`docker build -t server .`

`docker network create mynet`

`docker run -it --name server1 --network mynet --privileged server`

`mount -t bpf none /sys/fs/bpf`

`xdp-loader load -m skb -s xdp eth0 xdp.o`

`Run server with ./server`
## Running the client
`docker build -t client .`

`docker run -it --name client1 --network mynet --privileged client`

`Run docker inspect mynet to get the server ip and run client with ./client <server_ip>`


