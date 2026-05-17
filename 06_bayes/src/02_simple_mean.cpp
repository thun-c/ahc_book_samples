#pragma GCC optimize("Ofast")
#include <bits/stdc++.h>
using namespace std;

#include "lib/constants.hpp"
#include "lib/path_graph.hpp"

int main()
{
    Graph graph(GRID_SIZE);

    // 初期状態では全辺 5000 とみなす。
    vector<double> estimated_cost(NUM_EDGES, INITIAL_EDGE_COST);
    vector<int> observed_count(NUM_EDGES, 0);

    for (int i = 0; i < TOTAL_QUERIES; i++)
    {
        int si, sj, ti, tj;
        cin >> si >> sj >> ti >> tj;
        int vs = si * GRID_SIZE + sj;
        int vg = ti * GRID_SIZE + tj;

        graph.setCost(estimated_cost);
        const auto& [vpath, epath] = graph.findShortestPath(vs, vg);
        output(vpath);

        double observed_distance;
        cin >> observed_distance;

        if (epath.empty())
            continue;

        // 観測経路長を辺数で割った値を「その経路の1辺あたり長さ」とみなす。
        double path_edge_mean =
            observed_distance / static_cast<double>(epath.size());

        // 各通過辺について、現在観測した値と観測済み値の重み付き和を計算して更新する。
        for (int edge_index : epath)
        {
            double now_weight = 1.0 / (observed_count[edge_index] + 1);

            estimated_cost[edge_index] =
                now_weight * path_edge_mean + (1 - now_weight) * estimated_cost[edge_index];
            observed_count[edge_index]++;
        }
    }
    return 0;
}
