/* SPDX-License-Identifier: GPL-2.0 */
#include <linux/bpf.h>
#include <bpf/bpf_helpers.h>
#include <linux/in.h>
#include <linux/if_ether.h>
#include <linux/ip.h>
#include <linux/udp.h>
#define SERVER1_PORT 5051
#define SERVER2_PORT 5052
#define SERVER3_PORT 5053
static __always_inline __u16
csum_fold_helper(__u64 csum)
{
    int i;
#pragma unroll
    for (i = 0; i < 4; i++)
    {
        if (csum >> 16)
            csum = (csum & 0xffff) + (csum >> 16);
    }
    return ~csum;
}

static __always_inline __u16
iph_csum(struct iphdr *iph)
{
    iph->check = 0;
    unsigned long long csum = bpf_csum_diff(0, 0, (unsigned int *)iph, sizeof(struct iphdr), 0);
    return csum_fold_helper(csum);
}
// store the number of busy threads on each server
struct
{
    __uint(type, BPF_MAP_TYPE_ARRAY);
    __uint(key_size, sizeof(__u32));
    __uint(value, sizeof(__u32));
    __uint(max_entries, 3);
} server_map SEC(".maps");

struct
{
    __uint(type, BPF_MAP_TYPE_QUEUE);
    __type(value, __u32);
    __uint(max_entries, 100);
} queue SEC(".maps");
int getFirst()
{
    int data = -1;
    if (bpf_map_peek_elem(&queue, &data) != 0)
        return -1;
    bpf_map_pop_elem(&queue, &data);
    return data;
}
SEC("xdp")
int xdp_filter(struct xdp_md *ctx)
{
    // Cast the numerical addresses to pointers for packet data access
    void *data = (void *)(long)ctx->data;
    void *data_end = (void *)(long)ctx->data_end;

    // Define a pointer to the Ethernet header at the start of the packet data
    struct ethhdr *eth = data;

    // Ensure the packet includes a full Ethernet header; if not, let it continue up the stack
    if (data + sizeof(struct ethhdr) > data_end)
    {
        return XDP_PASS;
    }

    // Check if the packet's protocol indicates it's an IP packet
    if (eth->h_proto != __constant_htons(ETH_P_IP))
    {
        // If not IP, continue with regular packet processing
        return XDP_PASS;
    }

    // Access the IP header positioned right after the Ethernet header
    struct iphdr *ip = data + sizeof(struct ethhdr);

    // Ensure the packet includes the full IP header; if not, pass it up the stack
    if (data + sizeof(struct ethhdr) + sizeof(struct iphdr) > data_end)
    {
        return XDP_PASS;
    }

    // Confirm the packet uses UDP by checking the protocol field in the IP header
    if (ip->protocol != IPPROTO_UDP)
    {
        return XDP_PASS;
    }

    // Locate the UDP header that follows the IP header
    struct udphdr *udp = data + sizeof(struct ethhdr) + sizeof(struct iphdr);

    // Validate that the packet is long enough to include the full UDP header
    if (data + sizeof(struct ethhdr) + sizeof(struct iphdr) + sizeof(struct udphdr) > data_end)
    {
        return XDP_PASS;
    }
    bpf_printk("----Recieved UDP packet: Source port %d, Destination port %d\n---", __constant_ntohs(udp->source), __constant_ntohs(udp->dest));
    
    //check if server 1 is trying to contact load balancer
    if (udp->source == __constant_htons(5051))
    {
        __u32 key = 0;
        __u32 *value;
        value = bpf_map_lookup_elem(&server_map, &key);
        if (value && *value > 0)
        {
            bpf_printk("Recieved response from server 1, current free threads %d port %d", 5 - *value, udp->source);
            int packet_data = getFirst();
            if (packet_data != -1 && *value <= 4)
            {
                bpf_printk("Queue isn't empty so sending first data back to server\n");
                if (data + sizeof(struct ethhdr) + sizeof(struct iphdr) + sizeof(struct udphdr) + 1 > data_end)
                {
                    return XDP_DROP;
                }
                void *payload = data + sizeof(struct ethhdr) + sizeof(struct iphdr) + sizeof(struct udphdr);
                *((__u8 *)payload) = packet_data;
                udp->source = udp->dest;
                udp->dest = __constant_htons(SERVER1_PORT);
                ip->saddr = ip->daddr;
                ip->daddr = ip->daddr;
                ip->check = iph_csum(ip);
                unsigned char tmp[ETH_ALEN];
                __builtin_memcpy(tmp, eth->h_source, ETH_ALEN);
                __builtin_memcpy(eth->h_source, eth->h_dest, ETH_ALEN);
                __builtin_memcpy(eth->h_dest, tmp, ETH_ALEN);
                return XDP_TX;
            }
            else
                (*value)--;
        }
        return XDP_PASS;
    }

    //check if server 2 is trying to contact load balancer
    else if (udp->source == __constant_htons(5052))
    {
        __u32 key = 1;
        __u32 *value;
        value = bpf_map_lookup_elem(&server_map, &key);
        if (value && *value > 0)
        {
            bpf_printk("Recieved response from server 2, current free threads %d", 5 - *value);

            int packet_data = getFirst();
            if (packet_data != -1 && *value <= 4)
            {
                bpf_printk("Queue isn't empty so sending first data back to server\n");
                if (data + sizeof(struct ethhdr) + sizeof(struct iphdr) + sizeof(struct udphdr) + 1 > data_end)
                {
                    return XDP_DROP;
                }
                void *payload = data + sizeof(struct ethhdr) + sizeof(struct iphdr) + sizeof(struct udphdr);
                *((__u8 *)payload) = packet_data;
                udp->source = udp->dest;

                udp->dest = __constant_htons(SERVER2_PORT);
                ip->saddr = ip->daddr;
                ip->daddr = ip->daddr;
                ip->check = iph_csum(ip);
                unsigned char tmp[ETH_ALEN];
                __builtin_memcpy(tmp, eth->h_source, ETH_ALEN);
                __builtin_memcpy(eth->h_source, eth->h_dest, ETH_ALEN);
                __builtin_memcpy(eth->h_dest, tmp, ETH_ALEN);

                return XDP_TX;
            }
            else
                (*value)--;
        }
        return XDP_PASS;
    }
    //check if server 3 is trying to contact load balancer
    else if (udp->source == __constant_htons(5053))
    {
        __u32 key = 2;
        __u32 *value;
        value = bpf_map_lookup_elem(&server_map, &key);
        if (value && *value > 0)
        {
            bpf_printk("Recieved response from server 3, current free threads %d", 5 - *value);
            int packet_data = getFirst();
            if (packet_data != -1 && *value <= 4)
            {
                bpf_printk("Queue isn't empty so sending first data back to server\n");
                if (data + sizeof(struct ethhdr) + sizeof(struct iphdr) + sizeof(struct udphdr) + 1 > data_end)
                {
                    return XDP_DROP;
                }
                void *payload = data + sizeof(struct ethhdr) + sizeof(struct iphdr) + sizeof(struct udphdr);
                *((__u8 *)payload) = packet_data;
                udp->source = udp->dest;
                udp->dest = __constant_htons(SERVER3_PORT);
                ip->saddr = ip->daddr;
                ip->daddr = ip->daddr;
                ip->check = iph_csum(ip);
                unsigned char tmp[ETH_ALEN];
                __builtin_memcpy(tmp, eth->h_source, ETH_ALEN);
                __builtin_memcpy(eth->h_source, eth->h_dest, ETH_ALEN);
                __builtin_memcpy(eth->h_dest, tmp, ETH_ALEN);
                return XDP_TX;
            }
            else
            {
                (*value)--;
                return XDP_PASS;
            }
        }
        else
            return XDP_PASS;
    }
    //Check if any client wants to connect to the server
    if (udp->dest == __constant_htons(8080))
    {
        if (data + sizeof(struct ethhdr) + sizeof(struct iphdr) + sizeof(struct udphdr) + 1 > data_end)
        {
            return XDP_PASS;
        }
        //get the udp data
        void *payload = data + sizeof(struct ethhdr) + sizeof(struct iphdr) + sizeof(struct udphdr);
        __u8 udp_data = *((__u8 *)payload);
        int port = -1;
        //Find the server with free threads
        for (int i = 0; i < 3; i++)
        {
            __u32 key = i;
            __u32 *value;
            value = bpf_map_lookup_elem(&server_map, &key);
            if (value && *value <= 4)
            {
                if (i == 0)
                    port = SERVER1_PORT;
                else if (i == 1)
                    port = SERVER2_PORT;
                else
                    port = SERVER3_PORT;
                (*value)++;
                break;
            }
        }

        if (port == -1)
        {
            bpf_printk("All servers are busy at this moment. Inserting this packet to queue");
            if (bpf_map_push_elem(&queue, &udp_data, BPF_EXIST) != 0)
                return XDP_DROP;
        }
        else
        {
            bpf_printk("Found free server with free thread on port %d", port);
            udp->dest = __constant_htons(port);
            ip->saddr = ip->daddr;
            ip->daddr = ip->daddr;
            ip->check = iph_csum(ip);
            unsigned char tmp[ETH_ALEN];
            __builtin_memcpy(tmp, eth->h_source, ETH_ALEN);
            __builtin_memcpy(eth->h_source, eth->h_dest, ETH_ALEN);
            __builtin_memcpy(eth->h_dest, tmp, ETH_ALEN);
            return XDP_TX;
        }
    }

    // Allow the packet to pass through
    return XDP_PASS;
}

char _license[] SEC("license") = "GPL";