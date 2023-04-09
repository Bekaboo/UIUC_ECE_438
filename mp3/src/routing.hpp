#ifndef ROUTING_HPP
#define ROUTING_HPP


#include <stdio.h>

#define MAX_NNODE 256
#define MAX_NMSG  1024
#define MAX_LMSG  128
#define DISCONNECTED -999
#define INF 0x7FFFFFFF


typedef struct message_t {
	int src;
	int dest;
	char data[MAX_LMSG];
} message_t;

typedef struct messages_t {
	int num;
	message_t entries[MAX_NMSG];
} messages_t;


class Graph {
public:
	int nnode;
	bool nodes[MAX_NNODE];
	int adj [MAX_NNODE][MAX_NNODE]; // adjacency matrix
	int next[MAX_NNODE][MAX_NNODE]; // next hop if going from i to j
	int dist[MAX_NNODE][MAX_NNODE]; // minimum distance from i to j

	Graph();
	void clear_routing_info();
	void add_edge(int src, int dest, int cost);
	void write_rt(FILE* fp);
	void write_msg(FILE* fp, int src, int dest, char* content);
};


#endif
