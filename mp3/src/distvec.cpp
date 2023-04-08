#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_NNODE 256
#define MAX_NMSG  1024
#define MAX_LMSG  128
#define DISCONNECTED -999
#define INF 0x7FFFFFFF


class Graph {


private:
	int nnode;
	bool nodes[MAX_NNODE];
	int adj[MAX_NNODE][MAX_NNODE];
	int next[MAX_NNODE][MAX_NNODE];
	int dist[MAX_NNODE][MAX_NNODE];


	void dijkstra(int root) {

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


public:
	Graph() {
		for (int i = 0; i < MAX_NNODE; i++) {
			nodes[i] = false;
			for (int j = 0; j < MAX_NNODE; j++) {
				if (i == j) {
					adj[i][j] = 0;
					next[i][j] = i;
					dist[i][j] = 0;
				} else {
					adj[i][j] = DISCONNECTED;
					next[i][j] = DISCONNECTED;
					dist[i][j] = INF;
				}
			}
		}
	}


	void add_edge(int src, int dest, int cost) {
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


	void dijkstra_all() {
		for (int i = 0; i < MAX_NNODE; i++) {
			if (nodes[i]) dijkstra(i);
		}
	}


	void print() {
		printf("nnode = %d\n", nnode);
		printf("[adj]\n");
		for (int i = 0; i < 16; i++) {
			for (int j = 0; j < 16; j++) {
				if (adj[i][j] == DISCONNECTED)  printf("X ");
				else printf("%d ", adj[i][j]);
			}
			printf("\n");
		}
		printf("[next]\n");
		for (int i = 0; i < 16; i++) {
			for (int j = 0; j < 16; j++) {
				if (next[i][j] == DISCONNECTED)  printf("X ");
				else printf("%d ", next[i][j]);
			}
			printf("\n");
		}
		printf("[dist]\n");
		for (int i = 0; i < 16; i++) {
			for (int j = 0; j < 16; j++) {
				if (dist[i][j] == INF)  printf("X ");
				else printf("%d ", dist[i][j]);
			}
			printf("\n");
		}
	}
};


typedef struct message_t {
	int src;
	int dest;
	char data[MAX_LMSG];
} message_t;


int main(int argc, char** argv) {

	if (argc != 4) {
		printf("Usage: ./distvec topofile message_tfile changesfile\n");
		return -1;
	}

	Graph graph;
	FILE *fpt, *fpm, *fpc, *fpo;
	char buf[MAX_LMSG], hdata[MAX_LMSG];

	fpt = fopen(argv[1], "r");
	while (1) {
		if (!fgets(buf, MAX_LMSG, fpt)) break;
		int src, dest, cost;
		sscanf(buf, "%d %d %d", &src, &dest, &cost);
		graph.add_edge(src, dest, cost);
	}

	graph.dijkstra_all();
	graph.print();

	int nm = 0;
	message_t message_ts[MAX_NMSG];
	fpm = fopen(argv[2], "r");
	while (1) {
		if (!fgets(buf, MAX_LMSG, fpm)) break;
		sscanf(buf, "%d %d %s", &message_ts[nm].src,
			&message_ts[nm].dest, hdata);
		char* hptr = strstr(buf, hdata);
		strcpy(message_ts[nm].data, hptr);
		nm++;
	}

	for (int i = 0; i < nm; i++) {
		printf("%d %d %s",
			message_ts[i].src,
			message_ts[i].dest,
			message_ts[i].data);
	}

	// fpc = fopen(argv[3], "r");


	// fpo = fopen("output.txt", "w");
	// fclose(fpo);

	return 0;
}

