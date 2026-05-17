#pragma GCC optimize("Ofast")
#include <bits/stdc++.h>
using namespace std;

#include "lib/constants.hpp"  // 定数をまとめたコード
#include "lib/path_graph.hpp" // グラフを管理するクラス
#include "lib/kalman.hpp"     // カルマンフィルタの状態を管理するクラス

// 進捗に応じて、序盤は急減し終盤は緩やかに減少する補間値を返す
double exponentialDecrease(double start, double end, double progress)
{
    return pow(start, 1.0 - progress) * pow(end, progress);
}

int main()
{
    Graph graph(GRID_SIZE);

    // M=1に対応した多変量正規分布に近似したモデルを初期化
    KalmanState kalman_state1(getPriorMean(), getPriorSegmentCovarianceModel1());
    // M=2に対応した多変量正規分布に近似したモデルを初期化
    KalmanState kalman_state2(getPriorMean(), getPriorSegmentCovarianceModel2());

    for (int i = 0; i < TOTAL_QUERIES; i++)
    {
        int si, sj, ti, tj;
        cin >> si >> sj >> ti >> tj;
        int vs = si * GRID_SIZE + sj;
        int vg = ti * GRID_SIZE + tj;

        bool is_model2 =
            kalman_state2.log_likelihood >= kalman_state1.log_likelihood;

        double progress = i / double(TOTAL_QUERIES - 1);
        double lcb_value = exponentialDecrease(LCB_START, LCB_END, progress);
        vector<double> mixed_cost(NUM_EDGES, 0.0);
        const KalmanState& kalman = is_model2 ? kalman_state2 : kalman_state1;
        for (int edge_index = 0; edge_index < NUM_EDGES; edge_index++)
        {
            double kalman_mean = kalman.edgeMean(edge_index);
            double kalman_variance = kalman.edgeVariance(edge_index);
            mixed_cost[edge_index] =
                max(0.0, kalman_mean - lcb_value * sqrt(kalman_variance));
        }
        graph.setCost(mixed_cost);

        const auto& [vpath, epath] = graph.findShortestPath(vs, vg);
        vector<PathEdge> path_edges = buildPathEdges(epath);
        output(vpath);

        double observed_distance;
        cin >> observed_distance;

        // 観測値を元に事後分布を更新
        kalman_state1.observe(path_edges, observed_distance);
        kalman_state2.observe(path_edges, observed_distance);
    }
    return 0;
}
