/*
 * Copyright (c) 2001-2004 Swedish Institute of Computer Science.
 * All rights reserved.
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
 * Christiaan Simons rewrote this file to get a more stable echo example.
 */

/**
 * @file
 * TCP echo server example using raw API.
 *
 * Echos all bytes sent by connecting client,
 * and passively closes when client is done.
 *
 */

#include "lwip/opt.h"
#include "lwip/debug.h"
#include "lwip/stats.h"
#include "lwip/ip_addr.h"
#include "lwip/tcp.h"
#include "tcpecho_raw.h"

#include "helpers.h"

#if LWIP_TCP && LWIP_CALLBACK_API

static struct tcp_pcb *tcpecho_raw_pcb;

static ip_addr_t local_addr;
static u16_t local_port;

enum tcpecho_raw_states
{
    ES_NONE = 0,
    ES_ACCEPTED,
    ES_RECEIVED,
    ES_CLOSING
};

struct tcpecho_raw_state
{
    u8_t state;
    u8_t retries;
    struct tcp_pcb *pcb;
    /* pbuf (chain) to recycle */
    struct pbuf *p;
};

static void tcpecho_raw_free(struct tcpecho_raw_state *es)
{
    if (es != NULL)
    {
        if (es->p)
        {
            /* free the buffer chain if present */
            pbuf_free(es->p);
        }
        mem_free(es);
    }
}

static void tcpecho_raw_close(struct tcp_pcb *tpcb, struct tcpecho_raw_state *es)
{
    printf("tcpecho_raw_close()\n");

    tcp_arg(tpcb, NULL);
    tcp_sent(tpcb, NULL);
    tcp_recv(tpcb, NULL);
    tcp_err(tpcb, NULL);
    tcp_poll(tpcb, NULL, 0);
    tcpecho_raw_free(es);

    tcp_close(tpcb);
}

static void tcpecho_raw_send(struct tcp_pcb *tpcb, struct tcpecho_raw_state *es)
{
    struct pbuf *ptr;
    err_t wr_err = ERR_OK;

    while ((wr_err == ERR_OK) &&
            (es->p != NULL) &&
            (es->p->len <= tcp_sndbuf(tpcb)))
    {
        ptr = es->p;

        /* enqueue data for transmission */
        wr_err = tcp_write(tpcb, ptr->payload, ptr->len, 1);
        if (wr_err == ERR_OK)
        {
        	printf("tcpecho_raw_send - write okay\n");
            u16_t plen;

            plen = ptr->len;
            /* continue with next pbuf in chain (if any) */
            es->p = ptr->next;
            if(es->p != NULL)
            {
                /* new reference! */
                pbuf_ref(es->p);
            }
            /* chop first pbuf from chain */
            pbuf_free(ptr);
            /* we can read more data now */
            tcp_recved(tpcb, plen);
        }
        else
        if(wr_err == ERR_MEM)
        {
            /* we are low on memory, try later / harder, defer to poll */
        	printf("tcpecho_raw_send - low memory\n");
            es->p = ptr;
        }
        else
        {
            /* other problem ?? */
        	printf("tcpecho_raw_send - other error (%d)\n", wr_err);
        }
    }
}

static void tcpecho_raw_error(void *arg, err_t err)
{
    struct tcpecho_raw_state *es;

    LWIP_UNUSED_ARG(err);

    es = (struct tcpecho_raw_state *)arg;

    tcpecho_raw_free(es);
}

static err_t tcpecho_raw_poll(void *arg, struct tcp_pcb *tpcb)
{
    err_t ret_err;
    struct tcpecho_raw_state *es;

    es = (struct tcpecho_raw_state *)arg;
    if (es != NULL)
    {
        if (es->p != NULL)
        {
            /* there is a remaining pbuf (chain)  */
            tcpecho_raw_send(tpcb, es);
        }
        else
        {
            /* no remaining pbuf (chain)  */
            if(es->state == ES_CLOSING)
            {
                tcpecho_raw_close(tpcb, es);
            }
        }
        ret_err = ERR_OK;
    }
    else
    {
        /* nothing to be done */
        tcp_abort(tpcb);
        ret_err = ERR_ABRT;
    }
    return ret_err;
}

static err_t tcpecho_raw_sent(void *arg, struct tcp_pcb *tpcb, u16_t len)
{
    struct tcpecho_raw_state *es;

    LWIP_UNUSED_ARG(len);

    es = (struct tcpecho_raw_state *)arg;
    es->retries = 0;

    if(es->p != NULL)
    {
        /* still got pbufs to send */
        tcp_sent(tpcb, tcpecho_raw_sent);
        tcpecho_raw_send(tpcb, es);
    }
    else
    {
        /* no more pbufs to send */
        if(es->state == ES_CLOSING)
        {
            tcpecho_raw_close(tpcb, es);
        }
    }
    return ERR_OK;
}

static err_t tcpecho_raw_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err)
{
    struct tcpecho_raw_state *es;
    err_t ret_err;

    printf("tcpecho_raw_recv()\n");
//    LWIP_ASSERT("arg != NULL",arg != NULL);
    es = (struct tcpecho_raw_state *)arg;
    if (p == NULL)
    {
    	printf("tcpecho_raw_recv - REMOTE HOST CLOSED\n");
        /* remote host closed connection */
        es->state = ES_CLOSING;
        if(es->p == NULL)
        {
            /* we're done sending, close it */
            tcpecho_raw_close(tpcb, es);
        }
        else
        {
            /* we're not done yet */
            tcpecho_raw_send(tpcb, es);
        }
        ret_err = ERR_OK;
    }
    else
    if(err != ERR_OK)
    {
    	printf("tcpecho_raw_recv - != ERR_OK\n");
        /* cleanup, for unknown reason */
        if (p != NULL)
        {
            pbuf_free(p);
        }
        ret_err = err;
    }
    else
    if(es->state == ES_ACCEPTED)
    {
    	printf("tcpecho_raw_recv - ES_ACCEPTED\n");
        /* first data chunk in p->payload */
        es->state = ES_RECEIVED;
        /* store reference to incoming pbuf (chain) */
        es->p = p;
        tcpecho_raw_send(tpcb, es);
        ret_err = ERR_OK;
    }
    else
    if (es->state == ES_RECEIVED)
    {
        /* read some more data */
        if(es->p == NULL)
        {
        	printf("tcpecho_raw_recv - ES_RECEIVED-1\n");
            es->p = p;
            printf("Received %d bytes of data\n", es->p->len);
            char *inPayload = (char *)es->p->payload;
            for(int i=0; i<es->p->len; i++) {
            	printf("0x%02x, ", inPayload[i]);
				if( (i+1) % 8 == 0)
					printf("\n");
            }
//            tcpecho_raw_send(tpcb, es);
        }
        else
        {
            struct pbuf *ptr;
            printf("tcpecho_raw_recv - ES_RECEIVED-2\n");
            /* chain pbufs to the end of what we recv'ed previously  */
            ptr = es->p;
            pbuf_cat(ptr,p);
        }
        ret_err = ERR_OK;
    }
    else
    {
    	printf("tcpecho_raw_recv - TRASH_DATA\n");
        /* unkown es->state, trash data  */
        tcp_recved(tpcb, p->tot_len);
        pbuf_free(p);
        ret_err = ERR_OK;
    }

    return ret_err;
}

static err_t tcpecho_raw_accept(void *arg, struct tcp_pcb *newpcb, err_t err)
{
    err_t ret_err;
    struct tcpecho_raw_state *es;

    printf("tcpecho_raw_accept()\n");

    LWIP_UNUSED_ARG(arg);
    if ((err != ERR_OK) || (newpcb == NULL))
    {
        return ERR_VAL;
    }

    /* Unless this pcb should have NORMAL priority, set its priority now.
       When running out of pcbs, low priority pcbs can be aborted to create
       new pcbs of higher priority. */
    tcp_setprio(newpcb, TCP_PRIO_MIN);

    es = (struct tcpecho_raw_state *)mem_malloc(sizeof(struct tcpecho_raw_state));
    if (es != NULL)
    {
    	printf("tcpecho_raw_accept - es != NULL\n");
        es->state = ES_ACCEPTED;
        es->pcb = newpcb;
        es->retries = 0;
        es->p = NULL;
        /* pass newly allocated es to our callbacks */
        tcp_arg(newpcb, es);
        tcp_recv(newpcb, tcpecho_raw_recv);
        tcp_err(newpcb, tcpecho_raw_error);
        tcp_poll(newpcb, tcpecho_raw_poll, 0);
        tcp_sent(newpcb, tcpecho_raw_sent);
        ret_err = ERR_OK;
    }
    else
    {
    	printf("tcpecho_raw_accept - es == NULL\n");
        ret_err = ERR_MEM;
    }
    return ret_err;
}

void tcpecho_raw_user_send(ip_addr_t destIp, u16 destPort, char *payload, u16_t payload_len)
{
	printf("\ntcpecho_raw_user_send() called\n");
	printf("destination address: %d.%d.%d.%d\n",
				(destIp.addr >>  0) & 0xFF, (destIp.addr >>  8) & 0xFF,
				(destIp.addr >> 16) & 0xFF, (destIp.addr >> 24) & 0xFF);
	printf("destPort %d\n", destPort);
	printf("payload_len = %d (0x%x)\n", payload_len, payload_len);
	for (int i = 0; i < payload_len; i++)
	{
		printf("0x%02x, ", (unsigned char)payload[i]);
		if( (i+1) % 8 == 0)
			printf("\n");
	}
	printf("\n");

	struct tcpecho_raw_state *es;
	es = (struct tcpecho_raw_state *)tcpecho_raw_pcb->callback_arg;

	struct pbuf *p;
	p = pbuf_alloc(PBUF_TRANSPORT, payload_len, PBUF_RAW);
	char *outPayload = (char *)p->payload;
	for(int i=0; i<payload_len; i++) {
		*(outPayload + i) = payload[i];
	}

//	es = (struct tcpecho_raw_state *)malloc(sizeof(struct tcpecho_raw_state));
	es->p = p;
//	es->state = ES_ACCEPTED;
//	es->retries = 5;
//	es->pcb = tcpecho_raw_pcb;
	tcpecho_raw_send(tcpecho_raw_pcb, es);
	//            tcpecho_raw_send(tcpecho_raw_pcb, es);
}

void tcpecho_raw_init(u16_t listen_port)
{
    printf("tcpecho_raw_init: IPADDR_ANY, port %d\n", listen_port);
    tcpecho_raw_pcb = tcp_new();

    if (tcpecho_raw_pcb != NULL)
    {
        err_t err;

        local_port = listen_port;
        err = tcp_bind(tcpecho_raw_pcb, IPADDR_ANY, local_port);
        if (err == ERR_OK)
        {
        	printf("Calling tcp_listen and tcp_accept to set up callback function\n");
        	local_addr = tcpecho_raw_pcb->local_ip;
			local_port = tcpecho_raw_pcb->local_port;
			printf("local_port %d\n", local_port);
			printf("local_addr: 0x%x\n", local_addr.addr);
			printf("local_addr: %d.%d.%d.%d\n",
					(local_addr.addr >>  0) & 0xFF, (local_addr.addr >>  8) & 0xFF,
					(local_addr.addr >> 16) & 0xFF, (local_addr.addr >> 24) & 0xFF);
            tcpecho_raw_pcb = tcp_listen(tcpecho_raw_pcb);
            tcp_accept(tcpecho_raw_pcb, tcpecho_raw_accept);
        }
        else
        {
            /* abort? output diagnostic? */
        	printf("ERROR: tcp_bind() failed with code: %d\n", err);
        }
    }
    else
    {
        /* abort? output diagnostic? */
    	printf("ERROR: tcp_new() failed\n");
    }
}

#endif /* LWIP_TCP && LWIP_CALLBACK_API */
