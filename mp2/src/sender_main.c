#include "rdt.h"
#include "alg.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/stat.h>
#include <signal.h>
#include <string.h>
#include <sys/time.h>
#include <fcntl.h>
#include <errno.h>

struct sockaddr_in si_other;
int s, slen;

void diep(char *s) {
    perror(s);
    exit(1);
}

/*
 * Initialize the control structure for the RDT FSM
 *
 * Input: bytesToTransfer - number of bytes to transfer
 * Return: pointer to the control structure if successful,
 * NULL otherwise
 * */
rdt_sender_ctrl_info_t *rdt_sender_ctrl_init(int bytesToTransfer) {
    rdt_sender_ctrl_info_t *rdt_ctrl
        = (rdt_sender_ctrl_info_t *) malloc(sizeof(rdt_sender_ctrl_info_t));
    if (rdt_ctrl == NULL) {
        return NULL;
    }

    rdt_ctrl->state = IN;
    rdt_ctrl->seq = 0;
    rdt_ctrl->expack = 0;
    rdt_ctrl->dupack = 0;
    rdt_ctrl->dupack_cnt = 0;
    rdt_ctrl->rwnd = DATA_LEN;
    rdt_ctrl->cwnd = DATA_LEN;
    rdt_ctrl->ssthresh = 64 * 1024;
    rdt_ctrl->bytes_remaining = bytesToTransfer;

    return rdt_ctrl;
}

/*
 * Make packet with given data
 *
 * Input: data - pointer to the data to be sent
 *        len  - length of the data
 *        ctrl - pointer to the control structure
 * Return: pointer to the packet if successful, exit otherwise
 * */
rdt_packet_t* rdt_sender_make_packet(char *data, int len,
                                 rdt_sender_ctrl_info_t* ctrl) {
    rdt_packet_t *pkt = (rdt_packet_t *) malloc(sizeof(rdt_header_t) + len);
    if (pkt == NULL) {
        fprintf(stderr, "Failed to make packet %d\n", ctrl->seq);
        exit(1);
    }

    pkt->header.seq = ctrl->seq;
    pkt->header.ack = 0;    /* Sender packet, ack not used */
    pkt->header.rwnd = ctrl->rwnd;
    pkt->header.data_len = len;
    memcpy(pkt->data, data, len);

    return pkt;
}

/*
 * Send a packet, exit if failed
 *
 * Input: ctrl - pointer to the control structure
 *        pkt - pointer to the packet
 *        len - length of the data, EXCLUDING RDT HEADER
 * Output: packet_sent - 1 if packet is sent; 0 otherwise
 * */
int rdt_sender_send_packet(rdt_sender_ctrl_info_t *ctrl,
                            rdt_packet_t *pkt, int len) {
    /* Congestion control and flow control */
    if (ctrl->seq + len - ctrl->expack > min(ctrl->rwnd, ctrl->cwnd)) {
        return 0;
    }

    if (sendto(s, pkt, RDT_HEAD_LEN + len, 0,
               (struct sockaddr *) &si_other, slen) == -1) {
        free(pkt);
        diep("sendto()");
    }
    free(pkt);
    return 1;
}

/*
 * Start the timer
 *
 * Input: timer - pointer to the timer
 *        timeout - timeout value
 * Return: None
 * */
void timer_start(rdt_timer_t *timer, int timeout) {
    timer->timeout = timeout;
    gettimeofday(&timer->start, NULL);
}

/*
 * Check if the timer has timed out
 *
 * Input: timer - pointer to the timer
 * Return: 1 if timed out, 0 otherwise
 * */
int timer_timeout(rdt_timer_t *timer) {
    struct timeval now;
    gettimeofday(&now, NULL);

    /* Check if timed out */
    if ((now.tv_sec - timer->start.tv_sec) * 1000
            + (now.tv_usec - timer->start.tv_usec) > timer->timeout) {
        return 1;
    } else {
        return 0;
    }
}

/*
 * Retransmit a packet
 *
 * Input: ctrl - pointer to the control structure
 *        sendbuf - send (data) buffer
 * Return: None
 * */
void rdt_sender_act_retransmit(rdt_sender_ctrl_info_t *ctrl, char* sendbuf) {
    int bytes_to_send = min(DATA_LEN, ctrl->bytes_remaining);
    int pkt_seq = max(ctrl->seq - DATA_LEN, 0);
    rdt_packet_t *pkt = rdt_sender_make_packet(&sendbuf[pkt_seq],
                                               bytes_to_send, ctrl);

    if (rdt_sender_send_packet(ctrl, pkt, bytes_to_send)) {
        printf("Retransmit packet %d\n", pkt_seq);
        timer_start(&ctrl->timer, TIMEOUT);
    } else {
        fprintf(stderr, "Failed to retransmit packet %d\n", pkt_seq);
    }
}

/*
 * Transmit a new packet
 *
 * Input: ctrl - pointer to the control structure
 *        sendbuf - send (data) buffer
 * Return: None
 * */
void rdt_sender_act_transmit(rdt_sender_ctrl_info_t *ctrl, char* sendbuf) {
    if (ctrl->bytes_remaining <= 0) {
        return;
    }

    int bytes_to_send = min(DATA_LEN, ctrl->bytes_remaining);
    rdt_packet_t *pkt = rdt_sender_make_packet(
        &sendbuf[ctrl->seq], bytes_to_send, ctrl);

    if (rdt_sender_send_packet(ctrl, pkt, bytes_to_send)) {
        printf("Transmit packet %d\n", ctrl->seq);

        /* Update control structure */
        ctrl->seq += bytes_to_send;

        /* Start timer */
        timer_start(&ctrl->timer, TIMEOUT);
    } else {
        fprintf(stderr, "Failed to transmit packet %d\n", ctrl->seq);
    }
}

/*
 * Action taken by the sender triggered by timed out event
 *
 * Input: ctrl - pointer to the control structure
 *        sendbuf - send (data) buffer
 * Return: timeout - 1 if the timer has timed out, 0 otherwise
 * Side effects: change control structure
 * */
int rdt_sender_event_timeout(rdt_sender_ctrl_info_t *ctrl, char *sendbuf) {
    if (!timer_timeout(&ctrl->timer)) {
        return 0;
    }

    printf("Timeout detected\n");

    /* Update control structure */
    ctrl->ssthresh = ctrl->cwnd / 2;
    ctrl->cwnd = DATA_LEN;
    ctrl->dupack_cnt = 0;
    ctrl->state = SS;

    rdt_sender_act_retransmit(ctrl, sendbuf);
    return 1;
}

/*
 * Action taken by the sender triggered by received ACK event
 *
 * Input: ctrl - pointer to the control structure
 *        sendbuf - send (data) buffer
 *        recvbuf - receive buffer
 * Return: ackrecved - 1 if received ack, 0 otherwise
 * Side effects:
 * */
int rdt_sender_event_handleack(rdt_sender_ctrl_info_t *ctrl,
                           char *sendbuf, char *recvbuf) {
    /* Check if there are packets arrive */
    int recvlen = recvfrom(s, recvbuf, DATA_LEN + RDT_HEAD_LEN, 0,
                           (struct sockaddr *) &si_other, (socklen_t *) &slen);

    /* No packet received */
    if (errno == EAGAIN || errno == EWOULDBLOCK) {
        return 0;
    }

    if (recvlen <= 0) {
        diep("recvfrom()");     // Error
    } else if (recvlen > 0) {   // Received data (ACK)
                                // TODO: what if ACK is corrupted?
        rdt_packet_t *recvpkt = (rdt_packet_t *) recvbuf;
        if (recvpkt->header.ack == ctrl->dupack) {          // Duplicate ACK
            printf("Received duplicate ACK %d\n", recvpkt->header.ack);
            if (ctrl->state == FR) {
                ctrl->cwnd += DATA_LEN;
            } else {
                ctrl->dupack_cnt++;
            }
        } else if (recvpkt->header.ack < ctrl->expack) {    // Old ACK
                                                            // (1st dup ACK)
            printf("Received duplicate ACK %d\n", recvpkt->header.ack);
            if (ctrl->state == FR) {
                ctrl->cwnd += DATA_LEN;
                rdt_sender_act_transmit(ctrl, sendbuf);
            } else {
                ctrl->dupack = recvpkt->header.ack;
                ctrl->dupack_cnt = 1;
            }
        } else {                                            // New ACK
            printf("Received new ACK %d\n", recvpkt->header.ack);
            switch (ctrl->state) {
                case SS:
                    /* Update control structure */
                    ctrl->seq = recvpkt->header.ack;
                    ctrl->bytes_remaining -= min(DATA_LEN,
                                                 ctrl->bytes_remaining);
                    ctrl->cwnd += DATA_LEN;
                    ctrl->dupack_cnt = 0;
                    ctrl->expack = recvpkt->header.ack
                                    + min(DATA_LEN, ctrl->bytes_remaining);

                    /* Transmit a new packet */
                    rdt_sender_act_transmit(ctrl, sendbuf);
                    break;

                case CA:
                    ctrl->seq = recvpkt->header.ack;
                    ctrl->bytes_remaining -= min(DATA_LEN,
                                                 ctrl->bytes_remaining);
                    ctrl->cwnd += DATA_LEN  * DATA_LEN / ctrl->cwnd;
                    ctrl->dupack_cnt = 0;
                    ctrl->expack = recvpkt->header.ack
                                    + min(DATA_LEN, ctrl->bytes_remaining);

                    /* Transmit a new packet */
                    rdt_sender_act_transmit(ctrl, sendbuf);
                    break;

                case FR:
                    ctrl->seq = recvpkt->header.ack;
                    ctrl->bytes_remaining -= min(DATA_LEN,
                                                 ctrl->bytes_remaining);
                    ctrl->cwnd = ctrl->ssthresh;
                    ctrl->dupack = 0;
                    ctrl->state = CA;
                    ctrl->expack = recvpkt->header.ack
                                    + min(DATA_LEN, ctrl->bytes_remaining);
                    break;

                default: break;
            }
        }
    }

    return 1;
}

/*
 * Action taken by the sender triggered by duplicate ACK count event
 *
 * Input: ctrl - pointer to the control structure
 *        sendbuf - send (data) buffer
 * Return: 1 if dupack >= 3, 0 otherwise
 * */
int rdt_sender_event_dupackcount(rdt_sender_ctrl_info_t *ctrl, char *sendbuf) {
    if (ctrl->dupack_cnt < 3) {
        return 0;
    }

    switch (ctrl->state) {
        case SS:
        case CA:
            /* Update control structure */
            ctrl->ssthresh = ctrl->cwnd / 2;
            ctrl->cwnd = ctrl->ssthresh + 3 * DATA_LEN;

            rdt_sender_act_retransmit(ctrl, sendbuf);

            /* Switch to state FR */
            ctrl->state = FR;
            break;

        default: break;
    }

    return 1;
}

/*
 * Reliable data transfer sender, IN state
 *
 * Input: ctrl - pointer to the control structure
 *        sendbuf - send (data) buffer
 * Return: None
 * Side effects: recvbuf and control structure modified
 * */
void rdt_sender_state_in(rdt_sender_ctrl_info_t *ctrl, char *sendbuf) {
    /* Send the first packet */
    rdt_sender_act_transmit(ctrl, sendbuf);

    /* Switch to SS */
    ctrl->state = SS;
    return;
}

/*
 * Reliable data transfer sender, SS state
 *
 * Input: ctrl - pointer to the control structure
 *        sendbuf - send (data) buffer
 *        recvbuf - receive buffer
 * Return: None
 * Side effects: recvbuf and control structure modified
 * */
void rdt_sender_state_ss(rdt_sender_ctrl_info_t *ctrl,
                      char *sendbuf, char *recvbuf) {
    /* Action taken for received ACK */
    if (rdt_sender_event_handleack(ctrl, sendbuf, recvbuf)) {
        return;
    }

    /* Action taken to check duplicate ACK count */
    if (rdt_sender_event_dupackcount(ctrl, sendbuf)) {
        return;
    }

    /* Switch to state CA (congestion avoidance) if cwnd is larger than
     * ssthresh */
    if (ctrl->cwnd >= ctrl->ssthresh) {
        ctrl->state = CA;
        return;
    }

    /* Action taken when timed out */
    if (rdt_sender_event_timeout(ctrl, sendbuf)) {
        return;
    }

    /* Transmit a new packet */
    rdt_sender_act_transmit(ctrl, sendbuf);
    return;
}

/*
 * Reliable data transfer sender, CA state
 *
 * Input: ctrl - pointer to the control structure
 *        sendbuf - send (data) buffer
 *        recvbuf - receive buffer
 * Return: None
 * Side effects: recvbuf and control structure modified
 * */
void rdt_sender_state_ca(rdt_sender_ctrl_info_t *ctrl,
                      char *sendbuf, char *recvbuf) {
    if (rdt_sender_event_handleack(ctrl, sendbuf, recvbuf)) {
        return;
    }

    if (rdt_sender_event_dupackcount(ctrl, sendbuf)) {
        return;
    }

    if (rdt_sender_event_timeout(ctrl, sendbuf)) {
        return;
    }

    /* Transmit a new packet */
    rdt_sender_act_transmit(ctrl, sendbuf);
    return;
}

/*
 * Reliable data transfer sender, FR state
 *
 * Input: ctrl - pointer to the control structure
 *        sendbuf - send (data) buffer
 *        recvbuf - receive buffer
 * Return: None
 * Side effects: recvbuf and control structure modified
 * */
void rdt_sender_state_fr(rdt_sender_ctrl_info_t *ctrl,
                      char *sendbuf, char *recvbuf) {
    if (rdt_sender_event_handleack(ctrl, sendbuf, recvbuf)) {
        return;
    }

    if (rdt_sender_event_timeout(ctrl, sendbuf)) {
        return;
    }

    /* Transmit a new packet */
    rdt_sender_act_transmit(ctrl, sendbuf);
    return;
}

/*
 * Reliably transfer 'bytesToTransfer' bytes from 'filename' to host
 * at 'hostname' over UDP 'hostUDPport'
 *
 * Input: hostname - hostname of the receiver
 *        hostUDPport - UDP port number of the receiver
 *        filename - name of the file to transfer
 *        bytesToTransfer - number of bytes to transfer
 * Return: None
 * */
void reliablyTransfer(char* hostname, unsigned short int hostUDPport, char* filename, unsigned long long int bytesToTransfer) {
    //Open the file
    FILE *fp;
    fp = fopen(filename, "rb");
    if (fp == NULL) {
        printf("Could not open file to send.");
        exit(1);
    }

	/* Determine how many bytes to transfer */

    /* Get the length of the socket address struct */
    slen = sizeof (si_other);

    /* Create UDP socket, exit if failed to create one */
    if ((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
        diep("socket");
    /* Set socket to non-blocking */
    int s_flags = fcntl(s, F_GETFL, 0);
    fcntl(s, F_SETFL, s_flags | O_NONBLOCK);

    /* Initialize the socket address struct */
    memset((char *) &si_other, 0, sizeof (si_other));
    si_other.sin_family = AF_INET;
    si_other.sin_port = htons(hostUDPport);
    /* ?? */
    if (inet_aton(hostname, &si_other.sin_addr) == 0) {
        fprintf(stderr, "inet_aton() failed\n");
        exit(1);
    }


	/* Send data and receive acknowledgements on s*/

    /* Initialize buffers for sending and receiving messages */
    char recvbuf[DATA_LEN + RDT_HEAD_LEN];  // Only holds one packet at a time
    char sendbuf[bytesToTransfer];
    memset(recvbuf, 0, DATA_LEN + RDT_HEAD_LEN);
    int bytesRead = fread(sendbuf, 1, bytesToTransfer, fp);
    if (bytesToTransfer != bytesRead) {
        fprintf(stderr, "Warning: "
                "bytesToTransfer set to %llu but only %d bytes read\n",
                bytesToTransfer, bytesRead);
    }

    /* Initialize control structure for new RDT FSM */
    rdt_sender_ctrl_info_t *ctrl = rdt_sender_ctrl_init(bytesRead);
    if (ctrl == NULL) {
        fprintf(stderr, "Failed to initialize control structure for RDT\n");
        exit(1);
    }

    /* Start transmission */
    while (ctrl->bytes_remaining > 0) {
        switch (ctrl->state) {
            case IN: rdt_sender_state_in(ctrl, sendbuf); break;
            case SS: rdt_sender_state_ss(ctrl, sendbuf, recvbuf); break;
            case CA: rdt_sender_state_ca(ctrl, sendbuf, recvbuf); break;
            case FR: rdt_sender_state_fr(ctrl, sendbuf, recvbuf); break;
        };
    }

    /* Close the socket */
    printf("Closing the socket\n");
    close(s);
    /* Release memory occupied by the RDT control struct */
    free(ctrl);
    return;

}

/*
 * 
 */
int main(int argc, char** argv) {

    unsigned short int udpPort;
    unsigned long long int numBytes;

    if (argc != 5) {
        fprintf(stderr, "usage: %s receiver_hostname receiver_port filename_to_xfer bytes_to_xfer\n\n", argv[0]);
        exit(1);
    }
    udpPort = (unsigned short int) atoi(argv[2]);
    numBytes = atoll(argv[4]);



    reliablyTransfer(argv[1], udpPort, argv[3], numBytes);


    return (EXIT_SUCCESS);
}


