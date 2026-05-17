#pragma GCC optimize("Ofast")
#include <bits/stdc++.h>
using namespace std;

#include "lib/constants.hpp"  // 定数をまとめたコード
#include "lib/kalman.hpp"     // カルマンフィルタの状態を管理するクラス
#include "lib/mcmc.hpp"       // MCMC法の状態を管理するクラス
#include "lib/path_graph.hpp" // グラフを管理するクラス

// 進捗に応じて、序盤は急減し終盤は緩やかに減少する補間値を返す
double exponentialDecrease(double start, double end, double progress)
{
    return pow(start, 1.0 - progress) * pow(end, progress);
}

double calculateMixedVariance(double kalman_mean, double kalman_variance,
                              double mcmc_mean, double mcmc_variance,
                              double edge_kalman_weight,
                              double query_mcmc_weight, double mixed_mean)
{
    // モデル独立を仮定した近似的なモーメント補間。
    double mixed_second_moment =
        edge_kalman_weight * (kalman_variance + kalman_mean * kalman_mean) +
        query_mcmc_weight * (mcmc_variance + mcmc_mean * mcmc_mean);
    double mixed_variance = mixed_second_moment - mixed_mean * mixed_mean;
    return max(0.0, mixed_variance);
}

int main()
{
    Graph graph(GRID_SIZE);

    KalmanState kalman_state1(getPriorMean(), getPriorSegmentCovarianceModel1());
    KalmanState kalman_state2(getPriorMean(), getPriorSegmentCovarianceModel2());

    mt19937 rng(0);
    McmcState mcmc_state{};

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
            double mixed_mean = kalman_mean;
            double mixed_variance = kalman_variance;
            // MCMC法はサンプルが集まるまで精度に不安があるため、序盤は利用しない。
            // MCMC法モデルはM=2のみを仮定しているため、M=1の場合は混合を行わない。
            if (i > 100 && is_model2)
            {
                double mcmc_mean = mcmc_state.edgeMean(edge_index);
                double mcmc_variance = mcmc_state.edgeVariance(edge_index);

                double query_mcmc_weight = 0.5;
                double edge_kalman_weight = 1.0 - query_mcmc_weight;

                mixed_mean =
                    edge_kalman_weight * kalman_mean + query_mcmc_weight * mcmc_mean;
                mixed_variance = calculateMixedVariance(
                    kalman_mean, kalman_variance, mcmc_mean, mcmc_variance,
                    edge_kalman_weight, query_mcmc_weight, mixed_mean);
            }
            mixed_cost[edge_index] =
                max(0.0, mixed_mean - lcb_value * sqrt(mixed_variance));
        }
        graph.setCost(mixed_cost);

        const auto& [vpath, epath] = graph.findShortestPath(vs, vg);
        vector<PathEdge> path_edges = buildPathEdges(epath);
        output(vpath);

        double observed_distance;
        cin >> observed_distance;

        kalman_state1.observe(path_edges, observed_distance);
        kalman_state2.observe(path_edges, observed_distance);

        mcmc_state.observe(path_edges, observed_distance);

        if (is_model2)
        {
            mcmc_state.resample(rng);
        }
    }
    return 0;
}
