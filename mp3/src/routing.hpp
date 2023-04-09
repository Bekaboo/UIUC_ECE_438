#ifndef ROUTING_HPP
#define ROUTING_HPP


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_NNODE 256
#define MAX_NMSG  1024
#define MAX_NCHG  1024
#define MAX_LLINE 128
#define DISCONNECTED -999
#define INF 0x7FFFFFFF


typedef struct message_t {
	int src;
	int dest;
	char data[MAX_LLINE];
} message_t;

typedef struct messages_t {
	int num = 0;
	message_t entries[MAX_NMSG];
} messages_t;

typedef struct change_t {
	int src;
	int dest;
	int cost;
} change_t;

typedef struct changes_t {
	int num = 0;
	change_t entries[MAX_NCHG];
} changes_t;


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


void read_input(Graph* graph, messages_t* msgs, changes_t* chgs, char** argv);


#endif
