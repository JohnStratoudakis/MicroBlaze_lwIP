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

void udpecho_raw_send(char *data, u16_t datalen)
{
    int i = 0;

    printf("\nudpecho_raw_send\n");

    XGpio_DiscreteWrite(&gpio_2, 2, 0xBBBB0001);
    XGpio_DiscreteWrite(&gpio_2, 2, datalen);

    for ( i = 0; i < datalen; i++)
    {
        XGpio_DiscreteWrite(&gpio_2, 2, data[i]);
        printf("0x%02x, ", (unsigned char)data[i]);
        if( (i+1) % 8 == 0)
            printf("\n");
    }

    struct pbuf p;
    p.len = datalen;
    p.next = NULL;
    p.payload = data;
    XGpio_DiscreteWrite(&gpio_2, 2, 0xBBBB0002);
    udp_send(udpecho_raw_pcb, &p);

    XGpio_DiscreteWrite(&gpio_2, 2, 0xBBBB0003);
    printf("\nudpecho_raw_send().end\n");
}

static void udpecho_raw_recv(void *arg, struct udp_pcb *upcb, struct pbuf *p,
                             const ip_addr_t *addr, u16_t port)
{
    printf("udpecho_raw_recv()\n  ");
    LWIP_UNUSED_ARG(arg);
    if (p != NULL)
    {
        // Send the payload out of Fifo #1
        char *data;
        data = (char *)(p->payload);
        u16_t tot_len = p->tot_len;
        if (tot_len > 0)
        {
            if (XLlFifo_iTxVacancy(&fifo_2))
            {
                int i = 0;
                XGpio_DiscreteWrite(&gpio_2, 2, 0xE001);
                for ( i = 0; i < tot_len; i++)
                {
                    XGpio_DiscreteWrite(&gpio_2, 2, data[i]);
                    printf("0x%02x, ", (unsigned char)data[i]);
                    if( (i+1) % 8 == 0)
                        printf("\n");
                }

                XLlFifo_Write(&fifo_2, data, tot_len);

                XLlFifo_iTxSetLen(&fifo_2, tot_len);
            }
        }

        // Send it back to the sender as quick check
        udp_sendto(upcb, p, addr, port);

        /* free the pbuf */
        pbuf_free(p);
    }
}

void udpecho_raw_init(void)
{
    printf("tcpecho_raw_init: IPADDR_ANY, port 35312");
    udpecho_raw_pcb = udp_new();

    if (udpecho_raw_pcb != NULL)
    {
        err_t err;

        err = udp_bind(udpecho_raw_pcb, IPADDR_ANY, 35312);
        if (err == ERR_OK)
        {
            printf("WHY is it calling udp_recv here??");
            udp_recv(udpecho_raw_pcb, udpecho_raw_recv, NULL);
            printf("Does reaching this line require UDP data to have been received?");
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
