/*
 * Copyright (c) 2016 Stephan Linz <linz@li-pro.net>, Li-Pro.Net
 * All rights reserved.
 *
 * Based on examples provided by
 * Iwan Budi Kusnanto <ibk@labhijau.net> (https://gist.github.com/iwanbk/1399729)
 * Juri Haberland <juri@sapienti-sat.org> (https://lists.gnu.org/archive/html/lwip-users/2007-06/msg00078.html)
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 *
 * This file is part of and a contribution to the lwIP TCP/IP stack.
 *
 * Credits go to Adam Dunkels (and the current maintainers) of this software.
 *
 * Stephan Linz rewrote this file to get a basic echo example.
 */

/**
 * @file
 * UDP echo server example using raw API.
 *
 * Echos all bytes sent by connecting client,
 * and passively closes when client is done.
 *
 */

#include "lwip/opt.h"
#include "lwip/debug.h"
#include "lwip/stats.h"
#include "lwip/udp.h"
#include "udpecho_raw.h"

#include "helpers.h"

#if LWIP_UDP

static struct udp_pcb *udpecho_raw_pcb;

static ip_addr_t local_addr;
static u16_t local_port;

void udpecho_raw_send(ip_addr_t destIp, u16 destPort, char *payload, u16_t payload_len)
{
    printf("\nudpecho_raw_send\n");

//    for ( i = 0; i < datalen; i++)
//    {
//        printf("0x%02x, ", (unsigned char)data[i]);
//        if( (i+1) % 8 == 0)
//            printf("\n");
//    }

    printf("destination address: %d.%d.%d.%d\n",
			(destIp.addr >>  0) & 0xFF, (destIp.addr >>  8) & 0xFF,
			(destIp.addr >> 16) & 0xFF, (destIp.addr >> 24) & 0xFF);
    printf("payload_len = %d %x\n", payload_len, payload_len);

    struct pbuf *p;
    //p = pbuf_alloc(PBUF_RAW, payload_len, PBUF_RAM);
    p = pbuf_alloc(PBUF_TRANSPORT, payload_len, PBUF_RAM);

    char *outPayload = (char *) p->payload;
    for(int i=0; i<payload_len; i++){
    	int index = 0;
    	printf("Setting index %d to %d (0x%x)\n", (index + i), payload[i], payload[i]);
    	*(outPayload + index + i) = payload[i];
    }

    err_t status = udp_sendto(udpecho_raw_pcb, p, &destIp, destPort);

    u8_t free_status = pbuf_free(p);
    printf("udp_sendto() -> free_status: %d %i\n", free_status, free_status);
    printf("udp_sendto() -> status: %d %i\n", status, status);
}

static void udpecho_raw_recv(void *arg, struct udp_pcb *upcb, struct pbuf *p,
                             ip_addr_t *addr, u16_t port)
{
    printf("udpecho_raw_recv()\n");
//    LWIP_UNUSED_ARG(arg);
    if (p != NULL)
    {
        // Send the payload out of Fifo #1
        char *data;
        data = (char *)(p->payload);
        u16_t tot_len = p->tot_len;
        printf("udpecho_raw_recv - p->tot_len = %d\n", tot_len);
        if (tot_len > 0)
        {
            if (XLlFifo_iTxVacancy(&fifo_2))
            {
            	// Generate a small/custom header
            	u32 header_len = 16;
            	char header[16];
            	int i=0;

            	for(i=0; i<16; i++) {
            		header[i] = 0;
            	}
            	header[ 0] = 1; // Session #1
            	header[ 1] = (addr->addr >>   0) & 0xFF; // IP Address 0
            	header[ 2] = (addr->addr >>   8) & 0xFF; // IP Address 1
            	header[ 3] = (addr->addr >>  16) & 0xFF; // IP Address 2
            	header[ 4] = (addr->addr >>  24) & 0xFF; // IP Address 3
            	header[ 5] = port; // Port 0
            	header[ 6] = port; // Port 1
            	header[ 7] = (local_addr.addr >>   0) & 0xFF; // IP Address 0
				header[ 8] = (local_addr.addr >>   8) & 0xFF; // IP Address 1
				header[ 9] = (local_addr.addr >>  16) & 0xFF; // IP Address 2
				header[10] = (local_addr.addr >>  24) & 0xFF; // IP Address 3
				header[11] = local_port; // Port 0
				header[12] = local_port; // Port 1

            	printf("RX_PACKET - Spot #1\n");

            	// Write header length
            	XLlFifo_Write(&fifo_2, &header_len, 4);
            	// Write header
            	XLlFifo_Write(&fifo_2, header, header_len);

            	u32 payload_len = tot_len;
            	// Write payload
            	static char payload[MAX_FRAME_SIZE];

            	for(i=0; i<payload_len; i++) {
            		payload[i] = data[i];
            	}
            	if(payload_len % 4 != 0) {
					u32 padding = 0;
					padding = (4 - (payload_len %4));
					payload_len += padding;
					for(i=tot_len; i<payload_len; i++) {
						payload[i] = '\0';
					}
					printf("Adding padding %d bytes\n", padding);
				}
            	// Write payload length
				XLlFifo_Write(&fifo_2, &payload_len, 4);
				XLlFifo_Write(&fifo_2, payload, payload_len);

            	// Append payload length, followed by payload
            	//u32 entire_len = (4 + header_len) + (4 + payload_len);
            	u32 entire_len = (4 + header_len) + (4 + payload_len);

            	XLlFifo_iTxSetLen(&fifo_2, entire_len);

            	printf("udpecho_raw_recv - header_len=%lu\n", header_len);
            	printf("udpecho_raw_recv - entire_len=%lu\n", entire_len);
            	printf("udpecho_raw_recv - payload_len=%lu\n", payload_len);

//                int i = 0;
//                for ( i = 0; i < tot_len; i++)
//                {
//                    printf("0x%02x, ", (unsigned char)data[i]);
//                    if( (i+1) % 8 == 0)
//                        printf("\n");
//                }
//                printf("\n");

                // Dump port - source port
                printf("port %d\n", port);
//                printf("port 0x%x\n", port);
//                u16_t revPort = ((port & 0xFF00) >> 8) | ((port & 0xFF) << 8);
//                printf("port (Byte-Order Reversed) %u\n", revPort);
//                printf("port (Byte-Order Reversed) 0x%x\n", revPort);

                // Dump addr
                // 10.0.1.100
                printf("addr: 0x%x\n", addr->addr);
                printf("addr: %d.%d.%d.%d\n",
                		(addr->addr >>  0) & 0xFF, (addr->addr >>  8) & 0xFF,
						(addr->addr >> 16) & 0xFF, (addr->addr >> 24) & 0xFF);
//                printf("upcb->local_ip.addr: %d.%d.%d.%d\n",
//                        (upcb->local_ip.addr >>  0) & 0xFF, (upcb->local_ip.addr >>  8) & 0xFF,
//                		(upcb->local_ip.addr >> 16) & 0xFF, (upcb->local_ip.addr >> 24) & 0xFF);
//                printf("upcb->remote_ip.addr: %d.%d.%d.%d\n",
//                        (upcb->remote_ip.addr >>  0) & 0xFF, (upcb->remote_ip.addr >>  8) & 0xFF,
//                        (upcb->remote_ip.addr >> 16) & 0xFF, (upcb->remote_ip.addr >> 24) & 0xFF);

				//IPADDR2_COPY(&hdr->dipaddr, &hdr->sipaddr);
//                udp_sendto(upcb, p, addr, revPort);
            }
        } else {
        	printf("tot_len == 0\n");
        }

        /* free the pbuf */
        pbuf_free(p);
    }
}

void udpecho_raw_init(void)
{
    printf("udpecho_raw_init: IPADDR_ANY, port 35312\n");
    udpecho_raw_pcb = udp_new();

    if (udpecho_raw_pcb != NULL)
    {
        err_t err;

        local_port = 35312;
        err = udp_bind(udpecho_raw_pcb, IPADDR_ANY, local_port);
        if (err == ERR_OK)
        {
        	printf("Calling udp_recv to set up callback function\n");
        	local_addr = udpecho_raw_pcb->local_ip;
			local_port = udpecho_raw_pcb->local_port;
			printf("local_port %d\n", local_port);
			printf("local_addr: 0x%x\n", local_addr.addr);
			printf("local_addr: %d.%d.%d.%d\n",
					(local_addr.addr >>  0) & 0xFF, (local_addr.addr >>  8) & 0xFF,
					(local_addr.addr >> 16) & 0xFF, (local_addr.addr >> 24) & 0xFF);
            udp_recv(udpecho_raw_pcb, udpecho_raw_recv, NULL);
        }
        else
        {
            /* abort? output diagnostic? */
            printf("ERROR: udpecho_raw_init in udp_bind\n");
        }
    }
    else
    {
        /* abort? output diagnostic? */
        printf("ERROR: udpecho_raw_init in udp_new()\n");
    }
    printf(" - udpecho_raw_init.end\n");
}

#endif /* LWIP_UDP */
