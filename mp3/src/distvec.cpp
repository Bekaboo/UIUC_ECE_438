#include "routing.hpp"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

class GraphDV : public Graph {
private:
	void bellman_ford(int root, int neighbor);
	void bellman_ford_all();

public:
	GraphDV();
	bool signal[MAX_NNODE][MAX_NNODE]; // signal sent to notify an update
	void converge_and_report(FILE *fp, messages_t msgs);
};

GraphDV::GraphDV() {
	for (int i = 0; i < MAX_NNODE; i++) {
		for (int j = 0; j < MAX_NNODE; j++) {
			signal[i][j] = false;
		}
	}
}

/*
 * Bellman-Ford algorithm to update the shortest distance from root
 * to other nodes using the distance vector of the neighbor
 *
 * Input: int root - root node
 *        int neighbor - neighbor node
 * Output: void
 * Side effects: updates dist vector of root
 * */
void GraphDV::bellman_ford(int root, int neighbor) {
	bool updated = false;	// If root has updated its routing table

	/* Using Bellman-Ford equation to update min dist from root
	 * to other nodes */
	for (int dest = 0; dest < MAX_NNODE; dest++) {
		if (!nodes[dest] || root == dest) continue;
		int alternative_dist = adj[root][neighbor] + dist[neighbor][dest];
		if (alternative_dist < dist[root][neighbor] ||
				(alternative_dist == dist[root][neighbor] &&
				 neighbor < next[root][dest])) {
			dist[root][dest] = alternative_dist;
			next[root][dest] = neighbor;
			updated = true;
		}
	}

	/* Send signal to all neighbors */
	if (updated) {
		for (int neighbor = 0; neighbor < MAX_NNODE; neighbor++) {
			if (root == neighbor || adj[root][neighbor] == DISCONNECTED)
				continue;
			signal[root][neighbor] = true;
		}
	}
}

/*
 * Run Bellman-Ford algorithm on all nodes until no update is needed
 *
 * Input: void
 * Output: void
 * Side effects: updates dist and next vectors
 * */
void GraphDV::bellman_ford_all() {
	/* Initialize matrix 'dist', 'next', and 'signal' */
	for (int node = 0; node < MAX_NNODE; node++) {
		if (!nodes[node]) continue;
		for (int neighbor = 0; neighbor < MAX_NNODE; neighbor++) {
			if (node == neighbor || adj[node][neighbor] == DISCONNECTED)
				continue;
			dist[node][neighbor] = adj[node][neighbor];
			next[node][neighbor] = neighbor;
			signal[node][neighbor] = true;
		}
	}

	/* Keep updating the network until we walked all nodes without update */
	int last_updated = 0;
	int current = 0;
	do {
		/* Check for an incoming signals */
		for (int sigsender = 0; sigsender < MAX_NNODE; sigsender++) {
			if (signal[sigsender][current]) {
				bellman_ford(current, sigsender);
				last_updated = current;
			}
		}
		current = (current + 1) % MAX_NNODE;
	} while (last_updated != current);
}

/*
 * Run Bellman-Ford algorithm on all nodes until no update is needed
 * and write the routing table to file
 *
 * Input: FILE* fp - file pointer to output file
 * Output: void
 * Side effects: updates dist and next vectors
 * */
void GraphDV::converge_and_report(FILE* fp, messages_t msgs) {
	bellman_ford_all();
	write_rt(fp);
	for (int i = 0; i < msgs.num; i++) {
		write_msg(fp, msgs.entries[i].src,
			msgs.entries[i].dest, msgs.entries[i].data);
	}
}

int main(int argc, char **argv) {
	if (argc != 4) {
		printf("Usage: ./linkstate topofile messagefile changesfile\n");
		return -1;
	}

	GraphDV graph;
	messages_t msgs;
	changes_t chgs;

	read_input(&graph, &msgs, &chgs, argv);

	FILE *fpo;
	fpo = fopen("output.txt", "w");
	if (!fpo) {
		printf("Error: cannot open output file\n");
		exit(1);
	}

	graph.converge_and_report(fpo, msgs);

	for (int i = 0; i < chgs.num; i++) {
		graph.add_edge(
			chgs.entries[i].src,
			chgs.entries[i].dest,
			chgs.entries[i].cost);
		graph.clear_routing_info();
		graph.converge_and_report(fpo, msgs);
	}

	fclose(fpo);
	return 0;
}
