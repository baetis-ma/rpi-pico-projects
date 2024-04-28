#include "pico/cyw43_arch.h"

#include "lwip/pbuf.h"
#include "lwip/tcp.h"

#define TCP_PORT 80
#define DEBUG_printf printf
#define BUF_SIZE 2048
#define POLL_TIME_S 1

#include <malloc.h>
int heap, lastheap = 0;
uint32_t getTotalHeap(void) {
   extern char __StackLimit, __bss_end__;
   return &__StackLimit  - &__bss_end__;
}

uint32_t getFreeHeap(void) {
   struct mallinfo m = mallinfo();
   return getTotalHeap() - m.uordblks;
}

//reads index.html file into string - must be on three lines (stays in flash rom?)
const char http[6000] = 
#include "./index.html" 
;
int pktsize = 1460;    //rcp write will transmit when writes equal 1460 or close
int mesgsent = 0; 
//char tcpresponse[2048];

err_t tcp_server_send_data(void *arg, struct tcp_pcb *tpcb);
typedef struct TCP_SERVER_T_ {
    struct tcp_pcb *server_pcb;
    struct tcp_pcb *client_pcb;
    bool complete;
    uint8_t buffer_sent[BUF_SIZE];
    uint8_t buffer_recv[BUF_SIZE];
    int sent_len;
    int recv_len;
    int run_count;
} TCP_SERVER_T;


static TCP_SERVER_T* tcp_server_init(void) {
    //printf("init\n");
    TCP_SERVER_T *state = calloc(1, sizeof(TCP_SERVER_T));
    //lastheap = heap; printf("------ %6d   %6d\n", heap = getFreeHeap(), heap - lastheap);
    if (!state) { DEBUG_printf("failed to allocate state\n"); return NULL; }
    return state;
}

static err_t tcp_server_close(void *arg) {
    //printf("close\n");
    //lastheap = heap; printf("------ %6d   %6d\n", heap = getFreeHeap(), heap - lastheap);
    TCP_SERVER_T *state = (TCP_SERVER_T*)arg;
    err_t err = ERR_OK;
    if (state->client_pcb != NULL) {
        tcp_arg(state->client_pcb, NULL);
        tcp_poll(state->client_pcb, NULL, 0);
        tcp_sent(state->client_pcb, NULL);
        tcp_recv(state->client_pcb, NULL);
        tcp_err(state->client_pcb, NULL);
        err = tcp_close(state->client_pcb);
        if (err != ERR_OK) {
            DEBUG_printf("close failed %d, calling abort\n", err);
            tcp_abort(state->client_pcb);
            err = ERR_ABRT;
        }
        state->client_pcb = NULL;
    }
    if (state->server_pcb) {
        tcp_arg(state->server_pcb, NULL);
        tcp_close(state->server_pcb);
        state->server_pcb = NULL;
    }
    ////lastheap = heap; printf("------ %6d   %6d\n", heap = getFreeHeap(), heap - lastheap);
    return err;
}

static err_t tcp_server_result(void *arg, int status) {
    //printf("result\n");
    TCP_SERVER_T *state = (TCP_SERVER_T*)arg;
    //if (status == 0) { DEBUG_printf("test success\n"); } else { DEBUG_printf("test failed %d\n", status); }
    state->complete = true;

    return tcp_server_close(arg);

}

static err_t tcp_server_sent(void *arg, struct tcp_pcb *tpcb, u16_t len) {
    //printf("sent\n");
    TCP_SERVER_T *state = (TCP_SERVER_T*)arg;
    state->sent_len += len;
    //handles multiple packets from index.html type of GETs iresponce always < packet
    if((strcmp(tcpresponse, "index.html")==0) && mesgsent < strlen(http)) {
       tcp_server_send_data(arg, state->client_pcb);
    }

    if (state->sent_len >= strlen(http)) {
        state->recv_len = 0;
        //DEBUG_printf("---------Waiting for buffer from client\n");
    }

    return ERR_OK;
}

err_t tcp_server_send_data(void *arg, struct tcp_pcb *tpcb)
{
    //printf("data\n");
    TCP_SERVER_T *state = (TCP_SERVER_T*)arg;

    state->sent_len = 0;
    cyw43_arch_lwip_check();

    err_t err;
    int packet = 0;
    if(strcmp(tcpresponse, "index.html")==0) {
       if (strlen(http) - mesgsent >= pktsize) packet = pktsize; else packet = strlen(http) - mesgsent;
       //printf("INDEX.HTML\ntotal message = %d packet =  %d mesg start ptr = %d\n", strlen(http), packet, mesgsent);
       strncpy(state->buffer_sent, mesgsent + http, packet); 
       printf("MESGSENT = %d PACKET = %d\n", mesgsent, packet);
       //printf("============== http fragment sent  = %d bytes\n%s\n", strlen(state->buffer_sent), state->buffer_sent);
       err = tcp_write(tpcb, state->buffer_sent, packet, TCP_WRITE_FLAG_COPY);
       mesgsent = mesgsent + packet;
    } 
    else if(strstr(tcpresponse, "getData")!=0) {
       char temp[32];
       if(end_ptr - start_ptr > 100){ 
           start_ptr = end_ptr - 100;
           printf("skipped\n");
       }
       sprintf(outstring, "0,%d,%d,%.1f,%.1f", start_ptr, end_ptr, pulse, spO2);
       //printf("start_ptr = %d end_ptr =  %d\n", start_ptr, end_ptr);
       while(start_ptr < end_ptr) {
           sprintf(temp, ",%d",result_array[start_ptr%ARRAY_LEN]);
           ++start_ptr;
           strcat(outstring, temp);
       }
       if (strlen(outstring) - mesgsent >= pktsize) packet = pktsize; else packet = strlen(outstring) - mesgsent;
       //packet = strlen(outstring);
       strncpy(state->buffer_sent, mesgsent + outstring, packet); 
       printf("============== getDa outstring sent  = %d bytes\n", strlen(state->buffer_sent));
       err = tcp_write(tpcb, state->buffer_sent, packet, TCP_WRITE_FLAG_COPY);
       //err = tcp_write(tpcb, state->buffer_sent, packet, TCP_WRITE_FLAG_COPY);
       mesgsent = mesgsent + packet;
    }
    else {
       //if (strlen(outstring) - mesgsent >= pktsize) packet = pktsize; else packet = strlen(outstring) - mesgsent;
       //strncpy(state->buffer_sent, mesgsent + outstring, packet); 
       //strcpy(state->buffer_sent, "empty\0"); 
       //printf("============== blank buffer_sent = %d bytes\n", strlen(state->buffer_sent));
       err = tcp_write(tpcb, state->buffer_sent, packet, TCP_WRITE_FLAG_COPY);
       //mesgsent = mesgsent + packet;
    }
    if (err != ERR_OK) {
        DEBUG_printf("Failed to write data %d\n", err);
        return tcp_server_result(arg, -1);
    }
    return ERR_OK;
}

err_t tcp_server_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err) {
    //printf("recv\n");
    TCP_SERVER_T *state = (TCP_SERVER_T*)arg;
//printf("complete %d sent %d recv %d cnt %d\n", state->complete, state->sent_len, state->recv_len, state->run_count);
    if (!p) { return tcp_server_result(arg, -1); }
    cyw43_arch_lwip_check();
    if (p->tot_len > 0) {
        const uint16_t buffer_left = BUF_SIZE - state->recv_len;
        state->recv_len += pbuf_copy_partial(p, state->buffer_recv + state->recv_len,
                                             p->tot_len > buffer_left ? buffer_left : p->tot_len, 0);
        //DEBUG_printf("==============received %dbytes from client\n", p->tot_len);
        //DEBUG_printf("==============received %dbytes from client\n%s\n===> EOP\n", p->tot_len, state->buffer_recv);
        tcp_recved(tpcb, p->tot_len);
	sscanf(state->buffer_recv, "GET /%s", tcpresponse);
	printf("    >>>>> tcpresponce = %d/%s\n", strlen(tcpresponse), tcpresponse); 

        tcp_server_send_data(arg, state->client_pcb);
    }
    pbuf_free(p);
    return ERR_OK;
}

static void tcp_server_err(void *arg, err_t err) {
    //printf("err\n");
    if (err != ERR_ABRT) {
        DEBUG_printf("tcp_client_err_fn %d\n", err);
        tcp_server_result(arg, err);
    }
}

static err_t tcp_server_poll(void *arg, struct tcp_pcb *tpcb) {
    //printf("poll\n");
    //DEBUG_printf("tcp_server_poll_fn\n");
    return tcp_server_result(arg, -1); // no response is an error?
}

static err_t tcp_server_accept(void *arg, struct tcp_pcb *client_pcb, err_t err) {
    //printf("accept\n");
    sleep_ms(50);
    TCP_SERVER_T *state = (TCP_SERVER_T*)arg;
//printf("complete %d sent %d recv %d cnt %d\n", state->complete, state->sent_len, state->sent_len, state->run_count);
    if (err != ERR_OK || client_pcb == NULL) { 
        tcp_server_result(arg, err); return ERR_VAL; }

    state->client_pcb = client_pcb;
    tcp_arg(client_pcb, state);
    tcp_sent(client_pcb, tcp_server_sent);
    tcp_recv(client_pcb, tcp_server_recv);
    tcp_poll(client_pcb, tcp_server_poll, POLL_TIME_S * 2);
    tcp_err(client_pcb, tcp_server_err);

    return tcp_server_send_data(arg, state->client_pcb);
}

static bool tcp_server_open(void *arg) {
    //printf("open\n");
    TCP_SERVER_T *state = (TCP_SERVER_T*)arg;
//printf("complete %d sent %d recv %d cnt %d\n", state->complete, state->sent_len, state->sent_len, state->run_count);
    //lastheap = heap; printf("------ %6d   %6d\n", heap = getFreeHeap(), heap - lastheap);
    mesgsent = 0;
 
    //DEBUG_printf("Starting server at %s on port %u\n", 
    //        ip4addr_ntoa(netif_ip4_addr(netif_list)), TCP_PORT);

    struct tcp_pcb *pcb = tcp_new_ip_type(IPADDR_TYPE_ANY);

    if (!pcb) { 
	   // DEBUG_printf("failed to create pcb  %d\n", pcb); 
	    free(state); return false; }

    err_t err = tcp_bind(pcb, NULL, TCP_PORT);
    if (err) { 
       DEBUG_printf("failed to bind to port %d\n", TCP_PORT); 
       free(state);
       return false; }

    state->server_pcb = tcp_listen_with_backlog(pcb, 1);
    if (!state->server_pcb) { DEBUG_printf("failed to listen\n"); if (pcb) { tcp_close(pcb); } return false; }

    tcp_arg(state->server_pcb, state);
    tcp_accept(state->server_pcb, tcp_server_accept);

    return true;
}

void run_tcp_server_start(void) {
    printf("START\n");
    //lastheap = heap; printf("   ------ totalheap %6d  new since last close %6d\n", heap = getFreeHeap(), heap - lastheap);
    tcpresponse[0] = '\0';
    TCP_SERVER_T *state = tcp_server_init();
//printf("complete %d sent %d recv %d cnt %d\n", state->complete, state->sent_len, state->sent_len, state->run_count);
    if (!state) { return; }
    if (!tcp_server_open(state)) { tcp_server_result(state, -1); return; } 
    //some of these sleeps are needed
    while(!state->complete) {  
        cyw43_arch_poll();
	cyw43_arch_wait_for_work_until(make_timeout_time_ms(1000));
        //sleep_ms(500);
    }
    printf("server stop\n");
    free(state);
}

