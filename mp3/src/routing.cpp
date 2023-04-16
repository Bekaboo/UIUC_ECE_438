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
					if (dist[i][j] == INF) continue;	// unreachable
					fprintf(fp, "%d %d %d\n", j, next[i][j], dist[i][j]);
				}
			}
		}
	}
}


void Graph::write_msg(FILE* fp, int src, int dest, char* content) {
	if (dist[src][dest] == INF) {
		fprintf(fp, "from %d to %d cost infinite hops unreachable message %s", src, dest, content);
		return;
	}
	fprintf(fp, "from %d to %d cost %d hops ", src, dest, dist[src][dest]);
	int pos = src;
	while (pos != dest) {
		fprintf(fp, "%d ", pos);
		pos = next[pos][dest];
	}
	fprintf(fp, "message %s", content);
}


void read_input(Graph* graph, messages_t* msgs, changes_t* chgs, char** argv) {

	FILE *fpt, *fpm, *fpc;
	fpt = fopen(argv[1], "r");
	fpm = fopen(argv[2], "r");
	fpc = fopen(argv[3], "r");
	if (!fpt || !fpm || !fpc) {
		printf("Error: cannot open input file\n");
		exit(1);
	}

	int src, dest, cost;
	char buf[MAX_LLINE], hdata[MAX_LLINE], *first;

	while (1) {
		first = fgets(buf, MAX_LLINE, fpt);
		if (!first) break;
		if (*first == '\n' || *first == '\r') continue;
		sscanf(buf, "%d%d%d", &src, &dest, &cost);
		graph->add_edge(src, dest, cost);
	}

	while (1) {
		first = fgets(buf, MAX_LLINE, fpm);
		if (!first) break;
		if (*first == '\n' || *first == '\r') continue;
		sscanf(buf, "%d%d%s",
			&msgs->entries[msgs->num].src,
			&msgs->entries[msgs->num].dest,
			hdata);
		char* hptr = strstr(buf, hdata);
		strcpy(msgs->entries[msgs->num].data, hptr);
		msgs->num++;
	}

	while (1) {
		first = fgets(buf, MAX_LLINE, fpc);
		if (!first) break;
		if (*first == '\n' || *first == '\r') continue;
		sscanf(buf, "%d%d%d",
			&chgs->entries[chgs->num].src,
			&chgs->entries[chgs->num].dest,
			&chgs->entries[chgs->num].cost);
		chgs->num++;
	}

	fclose(fpt);
	fclose(fpm);
	fclose(fpc);
}

