/* SPDX-License-Identifier: GPL-2.0 */
#include <linux/bpf.h>
#include <bpf/bpf_helpers.h>
#include <linux/in.h>
#include <linux/if_ether.h>
#include <linux/ip.h>
#include <linux/udp.h>

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

    // Check if the destination port is the server's port
    if (udp->dest == __constant_htons(20000))
    {
        // Log the packet

        // Extract numeric data from the UDP payload
        if (data + sizeof(struct ethhdr) + sizeof(struct iphdr) + sizeof(struct udphdr) + 1 > data_end)
        {
            return XDP_PASS;
        }
        void *payload = data + sizeof(struct ethhdr) + sizeof(struct iphdr) + sizeof(struct udphdr);
        __u8 numeric_data = *((__u8 *)payload);

        // Check if the numeric data parity is even
        if (numeric_data % 2 == 0)
        {
            bpf_printk("Dropping UDP packet with data %d\n", numeric_data);
            // Drop the packet
            // bpf_trace_printk("Dropping UDP packet with even numeric data\n");
            return XDP_DROP;
        }
    }

    // Allow the packet to pass through
    return XDP_PASS;
}

char _license[] SEC("license") = "GPL";