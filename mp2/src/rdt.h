#ifndef __RDT_H__
#define __RDT_H__

#include <sys/time.h>

#define MTU          1500
#define IP_HEAD_LEN  20
#define UDP_HEAD_LEN 8
#define RDT_HEAD_LEN (sizeof(rdt_header_t))
#define DATA_LEN     (MTU - IP_HEAD_LEN - UDP_HEAD_LEN - RDT_HEAD_LEN)
#define TIMEOUT      1000 /* 1 second */

/*
 * States of reliable data transfer FSM
 *
 * IN: initial state (transmit first packet)
 * SS: slow start
 * CA: congestion avoidance
 * FR: fast recovery
 * */
typedef enum rdt_sender_state_t {IN, SS, CA, FR} rdt_sender_state_t;

/* Timer */
typedef struct rdt_timer_t {
    struct timeval start; /* Start time of timer */
    int timeout;          /* Timeout value       */
} rdt_timer_t;

/*
 * Control structure for reliable data transfer FSM (sender side)
 * */
typedef struct rdt_sender_ctrl_info_t {
    rdt_timer_t timer;        /* Timer                                    */
    rdt_sender_state_t state; /* Current state                            */
    int seq;                  /* Current sequence number                  */
    int expack;               /* Expected ACK number                      */
    int dupack;               /* Duplicate ACK number                     */
    int dupack_cnt;           /* Count of duplicate ACK                   */
    int rwnd;                 /* Self's receive window size (byte)        */
    int cwnd;                 /* Congestion window size (byte)            */
    int ssthresh;             /* Slow start threshold (byte)              */
    int bytes_remaining;      /* Number of bytes remaining to send (byte) */
} rdt_sender_ctrl_info_t;

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
    rdt_header_t header;      /* Header                                   */
    char data[];              /* Data buffer, length = data_len in header */
} rdt_packet_t;

#endif /* __RDT_H__ */
