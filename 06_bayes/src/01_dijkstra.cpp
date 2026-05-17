#pragma GCC optimize("Ofast")
#include <bits/stdc++.h>
using namespace std;

#include "lib/constants.hpp"
#include "lib/path_graph.hpp"

int main() {
  Graph graph(GRID_SIZE);

  // すべての辺長を 5000 と仮定して固定する。
  vector<double> fixed_cost(NUM_EDGES, INITIAL_EDGE_COST);
  graph.setCost(fixed_cost);

  for (int i = 0; i < TOTAL_QUERIES; i++) {
    int si, sj, ti, tj;
    cin >> si >> sj >> ti >> tj;
    int vs = si * GRID_SIZE + sj;
    int vg = ti * GRID_SIZE + tj;

    const auto &[vpath, _] = graph.findShortestPath(vs, vg);
    output(vpath);

    // 観測値はこの実装では使わないが、インタラクティブ入出力のため受け取る。
    double observed_distance;
    cin >> observed_distance;
  }
  return 0;
}
