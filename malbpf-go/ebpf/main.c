#include <linux/kconfig.h>
#define KBUILD_MODNAME "program"

#include "include/bpf.h"
#include "include/bpf_helpers.h"

#include <linux/udp.h>
#include <linux/ip.h>
#include <linux/in.h>


// Define map for counting how many times we've redirected a src IP
// Table is percpu to guarantee atomicity of the counters and 256 bits to account for IPv4 space
BPF_TABLE("percpu_array", uint32_t, long, ip_counter, 256);


SEC("xdp")
int mal_xdp(struct xdp_md *ctx)
{
	char src_addr[16];
	char dest_addr[16];

	void *data = (void *)(long)ctx->data;
	void *data_end = (void *)(long)ctx->data_end;
	struct ethhdr *eth = data;
	
	int pkt_sz = data_end - data;
	bpf_printk("packet size: %d", pkt_sz);

	if ((void*)eth + sizeof(*eth) <= data_end) {
		struct iphdr *ip = data + sizeof(*eth);
		if ((void*)ip + sizeof(*ip) <= data_end) {
			// Retrieve number of redirects from map
			long *count = ip_counter.lookup(htons(ip->saddr));

			// Convert IPs to dotted-decimal strings
			snprintf(src_addr, 16, "%pI4", &ip->saddr);
			snprintf(dest_addr, 16, "%pI4", &ip->saddr);
			switch (ip->protocol) {
				// Don't handle TCP for now
				case IPPROTO_TCP:
					return XDP_PASS;
				case IPPROTO_UDP:
					struct udphdr *udp = ip + sizeof(*ip);
					if ((void*)udp + sizeof(*ip) <= data_end) {
						int src_port = htons(udp->source);
						int dest_port = htons(udp->dest);

						// Send packet data if 
						if (strcmp(dest_addr,"172.217.3.238") == 0) {
							// Redirect dest IP
							*dest_addr = "10.0.2.15";

							// Lookup and update IP counter
							*count += 1;
							bpf_printk("sent out IP %s %s times", dest_addr, *count);
						}
					}
				default:
					return XDP_ABORTED;
			}
		}
	}

	return XDP_PASS;
}

char _license[] SEC("license") = "GPL";
__u32 _version SEC("version") = 0xFFFFFFFE;
