
#include "tcp_client.h"

#include "helpers.h"
#include "lwip/err.h"
//#include "lwip/opt.h"
//#include "lwip/debug.h"
//#include "lwip/stats.h"
//#include "lwip/ip_addr.h"
#include "lwip/tcp.h"

#if LWIP_TCP && LWIP_CALLBACK_API

static struct tcp_pcb *tcp_client_raw_pcb;


err_t tcp_client_connected(void *arg, struct tcp_pcb *tpcb, err_t err)
{
	printf("tcp_client_connected:");
	return ERR_OK;
}

static err_t tcpecho_raw_sent(void *arg, struct tcp_pcb *tpcb, u16_t len)
{
	printf("tcp_client_raw_sent: called\n");
	printf("tcp_client_raw_sent: arg = 0x%x\n", arg);
	printf("tcp_client_raw_sent: tpcb = 0x%x\n", tpcb);
	printf("tcp_client_raw_sent: len = 0x%d\n", len);

}

void tcp_client_send(char *payload, u16_t payload_len)
{
	err_t error;

	printf("tcp_client_send called payload_len=%d\n", payload_len);
	error = tcp_write(tcp_client_raw_pcb, payload, payload_len, TCP_WRITE_FLAG_COPY);
	if(error == ERR_OK)
	{
		printf("tcp_client_send: tcp_write returned ERR_OK\n");
		error = tcp_output(tcp_client_raw_pcb);
		if(error == ERR_OK)
		{
			printf("tcp_client_send: tcp_output return ERR_OK\n");
			tcp_sent(tcp_client_raw_pcb, tcpecho_raw_sent);
		}
		else
		{
			printf("tcp_client_send: error calling tcp_output %d\n", error);
		}
	}
	else
	{
		printf("tcp_client_send: error calling tcp_write %d\n", error);
	}
}

void tcp_client_receive(void)
{

}

void tcp_client_connect(ip_addr_t connect_ip, u16_t connect_port)
{
	printf("tcp_client_connect: connect_ip=0x%x, connect_port=%d\n", connect_ip.addr, connect_port);
	tcp_client_raw_pcb = tcp_new();

	if(tcp_client_raw_pcb != NULL)
	{
		err_t error;

		error = tcp_bind(tcp_client_raw_pcb, NULL, connect_port);
		if(error == ERR_OK)
		{
			printf("tcp_client_raw_pcb->snd_nxt = %d\n", tcp_client_raw_pcb->snd_nxt);
			printf("tcp_client_raw_pcb->rtseq = %d\n", tcp_client_raw_pcb->rtseq);
			printf("tcp_client_raw_pcb->mss = %d\n", tcp_client_raw_pcb->mss);
			printf("tcp_client_raw_pcb->snd_wnd_max = %d\n", tcp_client_raw_pcb->snd_wnd_max);
			//tcp_client_raw_pcb->snd_wnd_max = 2000;
			printf("tcp_client_connect: tcp_client_raw_pcb->state=%d\n", tcp_client_raw_pcb->state);

			error = tcp_connect(tcp_client_raw_pcb, &connect_ip, connect_port, tcp_client_connected);
			printf("tcp_client_raw_pcb->mss = %d\n", tcp_client_raw_pcb->mss);
			printf("tcp_client_raw_pcb->snd_wnd_max = %d\n", tcp_client_raw_pcb->snd_wnd_max);
			if(error == ERR_OK)
			{
				printf("tcp_client_connect: successfully called tcp_connect\n");
				printf("tcp_client_connect: tcp_client_raw_pcb=0x%x\n", tcp_client_raw_pcb);
			}
			else
			{
				printf("tcp_client_connect: error calling tcp_connect: %d\n", error);
			}
		}
		else
		{
			printf("tcp_client_connect: error calling tcp_bind(), error=%d\n", error);
		}
	}
	else
	{
		printf("tcp_client_connect: error calling tcp_new()\n");
	}
}

void tcp_client_close(void)
{
	err_t error;
	printf("tcp_client_close\n");

	error = tcp_close(tcp_client_raw_pcb);

	printf("tcp_client_close: tcp_close returned %d\n", error);
}

#endif
