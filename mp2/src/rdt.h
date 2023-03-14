#ifndef __RDT_H__
#define __RDT_H__

#include <sys/time.h>

#define MTU          1500
#define IP_HEAD_LEN  20
#define UDP_HEAD_LEN 8
#define RDT_HEAD_LEN (sizeof(rdt_header_t))
#define DATA_LEN     (MTU - IP_HEAD_LEN - UDP_HEAD_LEN - RDT_HEAD_LEN)
#define SEQ_INIT     0
#define TIMEOUT      1000 /* 1 second */

/*
 * Status of reliable data transfer FSM
 *
 * SS: slow start
 * CA: congestion avoidance
 * FR: fast recovery
 * */
enum rdt_status_t {SS, CA, FR};

/* Timer */
typedef struct rdt_timer_t {
    struct timeval start; /* Start time of timer */
    int timeout;          /* Timeout value       */
} rdt_timer_t;

/*
 * Control structure for reliable data transfer FSM
 * */
typedef struct rdt_ctrl_info_t {
    rdt_timer_t timer;        /* Timer                                           */
    enum rdt_status_t status; /* Current status                                  */
    int seq;                  /* Current sequence number                         */
    int ack;                  /* Expected ACK number                             */
    int dupack;               /* Duplicate ACK number                            */
    int dupack_cnt;           /* Count of duplicate ACK                          */
    int rwnd;                 /* Receive window size (byte)                      */
    int max_bytes_inflight;   /* Maximum number of bytes in flight allowd (byte) */
    int last_byte_sent;       /* Last byte sent (byte)                           */
    int last_byte_acked;      /* Last byte ACKed (byte)                          */
    int cwnd_start;           /* Congestion window start (byte)                  */
    int cwnd_end;             /* Congestion window end (byte)                    */
    int ssthresh;             /* Slow start threshold (byte)                     */
    int bytes_remaining;      /* Number of bytes remaining to send (byte)        */
} rdt_ctrl_info_t;

/*
 * Extra information in each packet for reliable data transfer
 * */
typedef struct rdt_header_t {
    int seq;                  /* Sequence number               */
    int ack;                  /* ACK number                    */
    int rwnd;                 /* Receiver window size (byte)   */
    int data_len;             /* Data length (byte)            */
} rdt_header_t;

/*
 * Reliable data transfer packet
 * */
typedef struct rdt_packet_t {
    rdt_header_t header;      /* Header             */
    char *data;               /* First byte of data */
} rdt_packet_t;

#endif /* __RDT_H__ */
