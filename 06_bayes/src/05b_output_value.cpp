#pragma GCC optimize("Ofast")
#include <bits/stdc++.h>
using namespace std;

#include "lib/constants.hpp"  // 定数をまとめたコード
#include "lib/kalman.hpp"     // カルマンフィルタの状態を管理するクラス
#include "lib/mcmc.hpp"       // MCMC法の状態を管理するクラス
#include "lib/path_graph.hpp" // グラフを管理するクラス

constexpr int OUTPUT_INTERVAL = 100;
constexpr const char* OUTPUT_VALUE_FILE = "05b_output_value.csv";

struct EdgeEstimate
{
    double mean = 0.0;
    double variance = 0.0;
};

// 進捗に応じて、序盤は急減し終盤は緩やかに減少する補間値を返す
double exponentialDecrease(double start, double end, double progress)
{
    return pow(start, 1.0 - progress) * pow(end, progress);
}

EdgeEstimate estimateEdge(int edge_index, const KalmanState& kalman,
                          const McmcState& mcmc_state, bool use_mcmc)
{
    double kalman_mean = kalman.edgeMean(edge_index);
    double kalman_variance = kalman.edgeVariance(edge_index);
    double mixed_mean = kalman_mean;
    double mixed_second_moment = kalman_variance + kalman_mean * kalman_mean;
    if (use_mcmc)
    {
        double mcmc_mean = mcmc_state.edgeMean(edge_index);
        double mcmc_variance = mcmc_state.edgeVariance(edge_index);

        // モデル同士の独立性を保つため、混合比は
        // 辺ごとの不確実性バランスでのみ調整する。
        double query_mcmc_weight = 0.5;
        double edge_kalman_weight = 1.0 - query_mcmc_weight;

        mixed_mean =
            edge_kalman_weight * kalman_mean + query_mcmc_weight * mcmc_mean;
        // モデル独立を仮定した近似的なモーメント補間。
        mixed_second_moment =
            edge_kalman_weight * (kalman_variance + kalman_mean * kalman_mean) +
            query_mcmc_weight * (mcmc_variance + mcmc_mean * mcmc_mean);
    }
    double mixed_variance =
        max(0.0, mixed_second_moment - mixed_mean * mixed_mean);
    return {mixed_mean, mixed_variance};
}

void outputEstimatedValueHeader(ofstream& file)
{
    file << "step,model";
    for (int edge_index = 0; edge_index < NUM_EDGES; edge_index++)
    {
        file << ",edge_" << edge_index;
    }
    file << '\n';
}

void outputEstimatedValues(ofstream& file, int step, int model,
                           const KalmanState& kalman,
                           const McmcState& mcmc_state, bool use_mcmc)
{
    file << step << ',' << model;
    for (int edge_index = 0; edge_index < NUM_EDGES; edge_index++)
    {
        file << ',' << estimateEdge(edge_index, kalman, mcmc_state, use_mcmc).mean;
    }
    file << '\n';
}

int main()
{
    Graph graph(GRID_SIZE);

    KalmanState kalman_state1(getPriorMean(), getPriorSegmentCovarianceModel1());
    KalmanState kalman_state2(getPriorMean(), getPriorSegmentCovarianceModel2());

    mt19937 rng(0);
    McmcState mcmc_state{};

    ofstream output_value_file(OUTPUT_VALUE_FILE);
    output_value_file << fixed << setprecision(6);
    outputEstimatedValueHeader(output_value_file);

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
        bool use_mcmc = i > 75 && is_model2;
        for (int edge_index = 0; edge_index < NUM_EDGES; edge_index++)
        {
            EdgeEstimate estimate =
                estimateEdge(edge_index, kalman, mcmc_state, use_mcmc);
            mixed_cost[edge_index] =
                max(0.0, estimate.mean - lcb_value * sqrt(estimate.variance));
        }
        graph.setCost(mixed_cost);

        const auto& [vpath, epath] = graph.findShortestPath(vs, vg);
        vector<PathEdge> path_edges = buildPathEdges(epath);
        output(vpath);

        double observed_distance;
        cin >> observed_distance;

        kalman_state1.observe(path_edges, observed_distance);
        kalman_state2.observe(path_edges, observed_distance);

        double path_mean_estimate = kalman_state2.pathMeanSum(path_edges);
        mcmc_state.observe(path_edges, observed_distance, path_mean_estimate);

        if (is_model2)
        {
            mcmc_state.resample(rng);
        }

        if ((i + 1) % OUTPUT_INTERVAL == 0)
        {
            bool output_model2 =
                kalman_state2.log_likelihood >= kalman_state1.log_likelihood;
            const KalmanState& output_kalman =
                output_model2 ? kalman_state2 : kalman_state1;
            bool output_use_mcmc = (i + 1) > 75 && output_model2;
            outputEstimatedValues(output_value_file, i + 1,
                                  output_model2 ? 2 : 1, output_kalman,
                                  mcmc_state, output_use_mcmc);
        }
    }
    return 0;
}
