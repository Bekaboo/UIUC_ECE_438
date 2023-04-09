#include "routing.hpp"


Graph::Graph() {
	for (int i = 0; i < MAX_NNODE; i++) {
		nodes[i] = false;
		for (int j = 0; j < MAX_NNODE; j++) {
			if (i == j) adj[i][j] = 0;
			else adj[i][j] = DISCONNECTED;
		}
	}
	clear_routing_info();
}


void Graph::clear_routing_info() {
	for (int i = 0; i < MAX_NNODE; i++) {
		for (int j = 0; j < MAX_NNODE; j++) {
			if (i == j) {
				next[i][j] = i;
				dist[i][j] = 0;
			} else {
				next[i][j] = DISCONNECTED;
				dist[i][j] = INF;
			}
		}
	}
}


void Graph::add_edge(int src, int dest, int cost) {
	if (!nodes[src]) {
		nodes[src] = true;
		nnode++;
	}
	if (!nodes[dest]) {
		nodes[dest] = true;
		nnode++;
	}
	adj[src][dest] = cost;
	adj[dest][src] = cost;
}


void Graph::write_rt(FILE* fp) {
	for (int i = 0; i < MAX_NNODE; i++) {
		if (nodes[i]) {
			for (int j = 0; j < MAX_NNODE; j++) {
				if (nodes[j]) {
					fprintf(fp, "%d %d %d\n", j, next[i][j], dist[i][j]);
				}
			}
		}
	}
}


void Graph::write_msg(FILE* fp, int src, int dest, char* content) {
	fprintf(fp, "from %d to %d cost %d hops ", src, dest, dist[src][dest]);
	int pos = src;
	while (pos != dest) {
		fprintf(fp, "%d ", pos);
		pos = next[pos][dest];
	}
	fprintf(fp, "message %s", content);
}

