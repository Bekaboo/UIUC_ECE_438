#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "routing.hpp"


class GraphDV: public Graph {
private:
	void dijkstra(int root);
	void dijkstra_all();
public:
	void converge_and_report(FILE* fp, messages_t msgs);
};


void GraphDV::dijkstra(int root) {

	// memoization init
	dist[root][root] = 0;
	int mst[MAX_NNODE];
	bool visited[MAX_NNODE];
	for (int i = 0; i < MAX_NNODE; i++) {
		mst[i] = DISCONNECTED;
		visited[i] = false;
	}

	for (int i = 0; i < nnode; i++) {

		// find node closest to visited set
		int min = INF, min_idx;
		for (int j = 0; j < MAX_NNODE; j++) {
			if (nodes[j] && !visited[j] && dist[root][j] < min) {
				min = dist[root][j];
				min_idx = j;
			}
		}

		// visit and relax edges
		visited[min_idx] = true;
		for (int j = 0; j < MAX_NNODE; j++) {
			if (nodes[j] && !visited[j]
				&& adj[min_idx][j] != DISCONNECTED
				&& dist[root][min_idx] + adj[min_idx][j] < dist[root][j]) {
				dist[root][j] = dist[root][min_idx] + adj[min_idx][j];
				mst[j] = min_idx;
			}
		}
	}

	// backtrack MST to construct NEXT
	for (int i = 0; i < MAX_NNODE; i++) {
		if (nodes[i] && i != root) {
			int j = i;
			while (mst[j] != root) {
				j = mst[j];
			}
			next[root][i] = j;
		}
	}
}


void GraphDV::dijkstra_all() {
	for (int i = 0; i < MAX_NNODE; i++) {
		if (nodes[i]) dijkstra(i);
	}
}


void GraphDV::converge_and_report(FILE* fp, messages_t msgs) {
	dijkstra_all();
	write_rt(fp);
	for (int i = 0; i < msgs.num; i++) {
		write_msg(fp, msgs.entries[i].src,
			msgs.entries[i].dest, msgs.entries[i].data);
	}
}


int main(int argc, char** argv) {

	if (argc != 4) {
		printf("Usage: ./distvec topofile messagefile changesfile\n");
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
		graph.clear_routing_info();
		graph.converge_and_report(fpo, msgs);
	}


	fclose(fpt);
	fclose(fpm);
	fclose(fpc);
	fclose(fpo);
	return 0;
}

