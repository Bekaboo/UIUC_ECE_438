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

#define RWND DATA_LEN * 4

struct sockaddr_in si_me, si_other;
int s, slen;

void diep(char *s) {
    perror(s);
    exit(1);
}


rdt_packet_t* make_ack(int acknum) {
    rdt_packet_t* ack = (rdt_packet_t*) malloc(RDT_HEAD_LEN);
    ack->header.seq = 0;
    ack->header.ack = acknum;
    ack->header.data_len = 0;
    ack->header.rwnd = RWND;
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

    int recv_len, next_expected = 0, can_terminate = 0;
    rdt_packet_t* pkt = (rdt_packet_t*) malloc(PACKET_LEN);

    while (1) {

        /* Receive */
        if ((recv_len = recvfrom(s, pkt, PACKET_LEN, 0, \
            (struct sockaddr *) &si_other, (socklen_t *) &slen)) == -1)
            diep("recvfrom");
        log(stdout, "Received packet %d with data length %d\n", \
            pkt->header.seq, pkt->header.data_len);

        if (pkt->header.seq == next_expected) {
            fwrite(pkt->data, 1, pkt->header.data_len, fptr);
            next_expected += pkt->header.data_len;
            can_terminate = pkt->header.last_pkg;
        }

        /* Send ACK */
        /* To keep things simple, we don't wait for 500ms */
        rdt_packet_t* ack = make_ack(next_expected);
        log(stdout, "Sending ACK %d\n", ack->header.ack);
        if (sendto(s, ack, RDT_HEAD_LEN, 0, \
            (struct sockaddr *) &si_other, slen) == -1)
            diep("sendto");

        /* Break at EOF */
        if (can_terminate) break;

    }

    fclose(fptr);
    free(pkt);

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

