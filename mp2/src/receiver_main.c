#include "utils.h"
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

#include "rdt.h"

#define MAX_BUFFERED_PACKETS 1024

struct sockaddr_in si_me, si_other;
int s, slen;

void diep(char *s) {
    perror(s);
    exit(1);
}


rdt_packet_t* make_ack(int acknum, int nslot) {
    rdt_packet_t* ack = (rdt_packet_t*) malloc(RDT_HEAD_LEN);
    ack->header.seq = 0;
    ack->header.ack = acknum;
    ack->header.data_len = 0;
    ack->header.rwnd = nslot * DATA_LEN;
    return ack;
}


void reliablyReceive(unsigned short int myUDPport, char* destinationFile) {
    
    slen = sizeof (si_other);


    if ((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
        diep("socket");

    memset((char *) &si_me, 0, sizeof (si_me));
    si_me.sin_family = AF_INET;
    si_me.sin_port = htons(myUDPport);
    si_me.sin_addr.s_addr = htonl(INADDR_ANY);
    log(stdout, "Now binding\n");
    if (bind(s, (struct sockaddr*) &si_me, sizeof (si_me)) == -1)
        diep("bind");


	/* Now receive data and send acknowledgements */    

    FILE* fptr = fopen(destinationFile, "w");
    if (fptr == NULL)
        diep("fopen");

    rdt_packet_t* pktbuf = (rdt_packet_t*) \
        malloc(PACKET_LEN * MAX_BUFFERED_PACKETS);
    memset(pktbuf, 0, PACKET_LEN * MAX_BUFFERED_PACKETS);
    int present[MAX_BUFFERED_PACKETS] = {0};
    int head = 0, next = 0, tail = 0;

    /*
        PKTBUF: | * | * |   | * |   | * | ... [Empty]
                  ^       ^               ^
                head    next            tail

        [0,    head) - Packets written to file and removed from buffer
        [head, next) - Contiguous packets *followed by a gap*, written to file
        [next, tail) - All fragmented gaps appear here
        [tail, head + MAX_BUFFERED_PACKETS) - Empty, available slots
        [head + MAX_BUFFERED_PACKETS, ...)  - Out of range
    */

    while (1) {

        /* Receive */
        rdt_packet_t* pkt = (rdt_packet_t*) malloc(PACKET_LEN);
        int recv_len;
        if ((recv_len = recvfrom(s, pkt, PACKET_LEN, 0, \
            (struct sockaddr *) &si_other, (socklen_t *) &slen)) == -1)
            diep("recvfrom");
        log(stdout, "Received packet %d with data length %d from %s:%d\n", \
            pkt->header.seq, pkt->header.data_len, \
            inet_ntoa(si_other.sin_addr), ntohs(si_other.sin_port));

        /* Buffer the packet */
        int pktnum = pkt->header.seq / DATA_LEN;
        if (pktnum >= next         // otherwise this must be a duplicate packet
            && pktnum < head + MAX_BUFFERED_PACKETS) {      // can buffer
            memcpy(pktbuf_idx(pktnum - head), pkt, RDT_HEAD_LEN + pkt->header.data_len);
            present[pktnum - head] = 1;
            if (tail <= pktnum) tail = pktnum + 1;

            /* Update "next" ptr */
            while (present[next - head]) {
                fwrite(pktbuf_idx(next - head) + 1, 1, \
                    pktbuf_idx(next - head)->header.data_len, fptr);
                present[next - head] = 0;
                next++;
            }

            /* Update "head" ptr */
            /* Good news: there's no need to copy the packets around */
            if (next == tail) {
                memset(present, 0, sizeof (present));       // clear the markers
                head = tail;                                // and start over
            }
        }

        /* Send ACK */
        /* To keep things simple, we don't wait for 500ms */
        rdt_packet_t* ack = make_ack(next * DATA_LEN, \
            MAX_BUFFERED_PACKETS - (tail - head));      // nslot = "tail" ~ MAX_BUFFERED_PACKETS
        log(stdout, "Sending ACK %d, rwnd = %d, head = %d, next = %d, tail = %d\n", \
            ack->header.ack, ack->header.rwnd, head, next, tail);
        if (sendto(s, ack, RDT_HEAD_LEN, 0, \
            (struct sockaddr *) &si_other, slen) == -1)
            diep("sendto");

        /* Break at EOF */
        if (pkt->header.last_pkg && head == tail) break;

    }

    fclose(fptr);

    /* Done */

    close(s);
	log(stdout, "Data written to file %s\n", destinationFile);
    return;
}

/*
 * 
 */
int main(int argc, char** argv) {

    unsigned short int udpPort;

    if (argc != 3) {
        log(stderr, "usage: %s UDP_port filename_to_write\n\n", argv[0]);
        exit(1);
    }

    udpPort = (unsigned short int) atoi(argv[1]);

    reliablyReceive(udpPort, argv[2]);
}

