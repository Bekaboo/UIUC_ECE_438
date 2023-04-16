#include "routing.hpp"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

class GraphDV : public Graph {
private:
	bool bellman_ford(int root, int neighbor);
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
 * Output: bool updated - true if root has updated its routing table
 * Side effects: updates dist vector of root
 * */
bool GraphDV::bellman_ford(int root, int neighbor) {
	bool updated = false;	// If root has updated its routing table

	/* Using Bellman-Ford equation to update min dist from root
	 * to other nodes */
	for (int dest = 0; dest < MAX_NNODE; dest++) {
		if (!nodes[dest] || root == dest || neighbor == dest) continue;
		int alternative_dist
			= dist[neighbor][dest] == INF
				? INF : dist[neighbor][dest] + adj[root][neighbor];
		if (alternative_dist < dist[root][dest] ||
				(alternative_dist == dist[root][dest] &&
				 neighbor < next[root][dest])) {
			dist[root][dest] = alternative_dist;
			next[root][dest] = neighbor;
			updated = true;
		}
	}

	/* Send signal to all neighbors */
	if (updated) {
		for (int neighbor = 0; neighbor < MAX_NNODE; neighbor++) {
			if (!nodes[neighbor] || root == neighbor ||
					adj[root][neighbor] == DISCONNECTED)
				continue;
			signal[root][neighbor] = true;
		}
	}

	return updated;
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
				last_updated = bellman_ford(current, sigsender) ? current : last_updated;
				signal[sigsender][current] = false;
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
	int src, dest, cost;
	char buf[MAX_LMSG], hdata[MAX_LMSG];

	FILE *fpt, *fpm, *fpc, *fpo;
	fpo = fopen("output.txt", "w");
	fpt = fopen(argv[1], "r");
	fpm = fopen(argv[2], "r");
	fpc = fopen(argv[3], "r");
	if (!fpt || !fpm || !fpc || !fpo) {
		printf("Error: cannot open file\n");
		return -1;
	}


	while (1) {
		if (!fgets(buf, MAX_LMSG, fpt)) break;
		sscanf(buf, "%d %d %d", &src, &dest, &cost);
		graph.add_edge(src, dest, cost);
	}

	while (1) {
		if (!fgets(buf, MAX_LMSG, fpm)) break;
		sscanf(buf, "%d %d %s", &msgs.entries[msgs.num].src,
			&msgs.entries[msgs.num].dest, hdata);
		char* hptr = strstr(buf, hdata);
		strcpy(msgs.entries[msgs.num].data, hptr);
		msgs.num++;
	}

	graph.converge_and_report(fpo, msgs);

	while (1) {
		if (!fgets(buf, MAX_LMSG, fpc)) break;
		sscanf(buf, "%d %d %d", &src, &dest, &cost);
		graph.add_edge(src, dest, cost);
		graph.reset();
		graph.converge_and_report(fpo, msgs);
	}


	fclose(fpt);
	fclose(fpm);
	fclose(fpc);
	fclose(fpo);
	return 0;
}
