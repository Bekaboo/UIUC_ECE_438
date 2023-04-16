#include "routing.hpp"


class GraphLS: public Graph {
private:
	void dijkstra(int root);
	void dijkstra_all();
public:
	GraphLS();
	GraphLS(messages_t* msgs, changes_t* chgs, char** argv);
	void converge_and_report(FILE* fp, messages_t msgs);
};

GraphLS::GraphLS() {}

GraphLS::GraphLS(messages_t *msgs, changes_t *chgs, char **argv)
	: Graph(msgs, chgs, argv) {}

void GraphLS::dijkstra(int root) {

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


void GraphLS::dijkstra_all() {
	for (int i = 0; i < MAX_NNODE; i++) {
		if (nodes[i]) dijkstra(i);
	}
}


void GraphLS::converge_and_report(FILE* fp, messages_t msgs) {
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

	messages_t msgs;
	changes_t chgs;
	GraphLS graph = GraphLS(&msgs, &chgs, argv);

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

