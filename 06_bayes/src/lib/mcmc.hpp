#pragma GCC optimize("Ofast")
#pragma once
#include <bits/stdc++.h>
using namespace std;

#include "lib/constants.hpp"
#include "lib/edge_mapping.hpp"
#include "lib/matrix.hpp"

/*************************************************************************
 * 05_kalman_mcmc.cpp から必要:
 * MCMC の状態管理・観測履歴蓄積・再サンプリング更新を行う。
 *************************************************************************/
// 各観測をセグメントごとに見たときの、split候補ごとの左右カウント。
struct SegmentObservationCounts
{
    array<int, SEGMENT_LENGTH - 1> left_counts;
    array<int, SEGMENT_LENGTH - 1> right_counts;
};

// 経路情報を加工した後にも必要な、観測ごとの状態を保持する。
struct ObservationState
{
    double observed_cost;
    double noise_variance;
    double current_predicted_cost;
};

// セグメントの split 位置と左右コスト。
struct SegmentParameters
{
    int split_index;
    double left_cost;
    double right_cost;
};

double segmentCostAtBin(const SegmentParameters& parameters, int bin_index)
{
    return (bin_index <= parameters.split_index) ? parameters.left_cost
                                                 : parameters.right_cost;
}

// 各セグメントの事後分布を管理する。
struct SegmentPosteriorState
{
    SegmentParameters parameters;
    vector<SegmentObservationCounts> observation_counts;
    vector<int> relevant_observation_indices;
    array<double, SEGMENT_LENGTH> mean_cost_by_bin;
    array<double, SEGMENT_LENGTH> var_cost_by_bin;
};

// 観測誤差の分散を計算する。
double computeObservationNoiseVariance(double observed_cost)
{
    return observed_cost * observed_cost * MULTIPLICATIVE_NOISE_VARIANCE +
           ROUNDING_NOISE_VARIANCE;
}

// clampされた正規分布からサンプルを生成する。
double sampleTruncatedNormal(double mean, double sigma, mt19937& rng)
{
    normal_distribution<double> dist(mean, sigma);
    return clamp(dist(rng), MIN_EDGE_COST, MAX_EDGE_COST);
}

// セグメントの事後分布を構築するための split 候補ごとの項。
struct SegmentPosteriorTerms
{
    array<double, SEGMENT_LENGTH - 1> log_weights;
    array<Matrix<2, 2>, SEGMENT_LENGTH - 1> precision;
    array<Matrix<1, 2>, SEGMENT_LENGTH - 1> weighted_mean;
};

// セグメントのパラメータをサンプルする。
SegmentParameters sampleSegmentParameters(const SegmentPosteriorTerms& posterior,
                                          mt19937& rng)
{
    array<double, SEGMENT_LENGTH - 1> weights;
    for (int split_candidate = 0; split_candidate < SEGMENT_LENGTH - 1;
         split_candidate++)
    {
        weights[split_candidate] = exp(posterior.log_weights[split_candidate]);
    }

    int split_index = discrete_distribution{weights.begin(), weights.end()}(rng);

    Matrix<2, 2> covariance = inv(posterior.precision[split_index]);
    Matrix<2, 1> mean =
        covariance * posterior.weighted_mean[split_index].transpose();
    Matrix<2, 2> basis = llt(covariance);
    Matrix<2, 1> sampled_costs = mean + basis * randNormal<2>(rng);

    return SegmentParameters{
        .split_index = split_index,
        .left_cost = clamp(sampled_costs[0][0], MIN_EDGE_COST, MAX_EDGE_COST),
        .right_cost = clamp(sampled_costs[1][0], MIN_EDGE_COST, MAX_EDGE_COST),
    };
}

// MCMCの状態を管理する。
struct McmcState
{
    array<SegmentPosteriorState, SEGMENT_COUNT> segments;
    // observation_index は observations と各 segment.observation_counts で共通。
    vector<ObservationState> observations;

    McmcState()
    {
        for (int segment_index = 0; segment_index < SEGMENT_COUNT;
             segment_index++)
        {
            SegmentPosteriorState& segment = segments[segment_index];
            segment.parameters = SegmentParameters{
                .split_index = INITIAL_SPLIT_INDEX,
                .left_cost = INITIAL_EDGE_COST,
                .right_cost = INITIAL_EDGE_COST,
            };
            segment.mean_cost_by_bin.fill(INITIAL_EDGE_COST);
            segment.var_cost_by_bin.fill(0.0);
        }
    }

    // 観測を追加する。
    void observe(const vector<PathEdge>& path_edges, double observed_cost)
    {
        double noise_variance = computeObservationNoiseVariance(observed_cost);
        addObservation(path_edges, observed_cost, noise_variance);
    }

    // セグメントを再サンプリングする。
    void resample(mt19937& rng)
    {
        // 1. 各セグメント・各位置・各試行のサンプルを保存する。
        array<array<array<double, RESAMPLE_COUNT>, SEGMENT_LENGTH>, SEGMENT_COUNT>
            sample_costs{};

        // 2. 各セグメントを RESAMPLE_COUNT 回ずつサンプルし直す。
        for (int trial = 0; trial < RESAMPLE_COUNT; trial++)
        {
            for (int segment_index = 0; segment_index < SEGMENT_COUNT;
                 segment_index++)
            {
                resampleSegment(segment_index, rng);
                // 3. 各位置の辺コストサンプルに直して保存する。
                for (int bin_index = 0; bin_index < SEGMENT_LENGTH; bin_index++)
                {
                    double sampled_cost =
                        segmentCostAtBin(segments[segment_index].parameters,
                                         bin_index);
                    sample_costs[segment_index][bin_index][trial] = sampled_cost;
                }
            }
        }

        // 4. 保存したサンプル列から平均と分散を計算する。
        for (int segment_index = 0; segment_index < SEGMENT_COUNT; segment_index++)
        {
            SegmentPosteriorState& segment = segments[segment_index];
            for (int bin_index = 0; bin_index < SEGMENT_LENGTH; bin_index++)
            {
                double sum = 0.0;
                for (int trial = 0; trial < RESAMPLE_COUNT; trial++)
                {
                    sum += sample_costs[segment_index][bin_index][trial];
                }
                double mean = sum / RESAMPLE_COUNT;
                segment.mean_cost_by_bin[bin_index] = mean;

                double variance_sum = 0.0;
                for (int trial = 0; trial < RESAMPLE_COUNT; trial++)
                {
                    double diff =
                        sample_costs[segment_index][bin_index][trial] - mean;
                    variance_sum += diff * diff;
                }
                segment.var_cost_by_bin[bin_index] =
                    variance_sum / RESAMPLE_COUNT;
            }
        }
    }

    // 辺の平均コストを取得する。
    double edgeMean(int edge_index) const
    {
        PathEdge path_edge = getPathEdge(edge_index);
        return segments[path_edge.segment_index]
            .mean_cost_by_bin[path_edge.bin_index];
    }

    // 辺の分散を取得する。
    double edgeVariance(int edge_index) const
    {
        PathEdge path_edge = getPathEdge(edge_index);
        return segments[path_edge.segment_index]
            .var_cost_by_bin[path_edge.bin_index];
    }

    // 観測を追加し、経路情報はセグメントごとのカウントへ加工してから捨てる。
    void addObservation(const vector<PathEdge>& path_edges, double observed_cost,
                        double noise_variance)
    {
        for (int segment_index = 0; segment_index < SEGMENT_COUNT;
             segment_index++)
        {
            segments[segment_index].observation_counts.emplace_back();
        }

        array<bool, SEGMENT_COUNT> visited_segment{};
        double predicted_cost = 0.0;

        for (const PathEdge& edge : path_edges)
        {
            SegmentPosteriorState& segment = segments[edge.segment_index];

            if (!visited_segment[edge.segment_index])
            {
                segment.relevant_observation_indices.push_back(
                    static_cast<int>(observations.size()));
            }
            visited_segment[edge.segment_index] = true;

            for (int split_candidate = 0; split_candidate < SEGMENT_LENGTH - 1;
                 split_candidate++)
            {
                if (edge.bin_index <= split_candidate)
                {
                    segment.observation_counts.back().left_counts[split_candidate]++;
                }
                else
                {
                    segment.observation_counts.back().right_counts[split_candidate]++;
                }
            }

            predicted_cost += segmentCostAtBin(segment.parameters, edge.bin_index);
        }

        observations.emplace_back(observed_cost, noise_variance, predicted_cost);
    }

    // セグメントの事後分布を構築する。
    SegmentPosteriorTerms buildSegmentPosterior(int segment_index) const
    {
        SegmentPosteriorTerms posterior{};

        double prior_precision = 1.0 / (INITIAL_EDGE_SIGMA * INITIAL_EDGE_SIGMA);
        double prior_weighted_mean = INITIAL_EDGE_COST * prior_precision;

        for (int split_candidate = 0; split_candidate < SEGMENT_LENGTH - 1;
             split_candidate++)
        {
            posterior.precision[split_candidate] = {
                {{prior_precision, 0.0}, {0.0, prior_precision}}};
            posterior.weighted_mean[split_candidate] = {
                {{prior_weighted_mean, prior_weighted_mean}}};
        }

        const SegmentPosteriorState& segment = segments[segment_index];

        for (int observation_index : segment.relevant_observation_indices)
        {
            const ObservationState& observation = observations[observation_index];
            const SegmentObservationCounts& counts =
                segment.observation_counts[observation_index];

            // 現在の全体予測からこのセグメントの旧寄与を戻し、
            // split 候補ごとの左/右コストだけで説明する観測値に直す。
            double normalized_observation =
                observation.observed_cost - observation.current_predicted_cost +
                segment.parameters.left_cost *
                    counts.left_counts[segment.parameters.split_index] +
                segment.parameters.right_cost *
                    counts.right_counts[segment.parameters.split_index];

            double inv_noise_variance = 1.0 / observation.noise_variance;

            for (int split_candidate = 0; split_candidate < SEGMENT_LENGTH - 1;
                 split_candidate++)
            {
                Matrix<1, 2> left_right_counts{
                    {{double(counts.left_counts[split_candidate]),
                      double(counts.right_counts[split_candidate])}}};

                posterior.precision[split_candidate] +=
                    left_right_counts.transpose() * inv_noise_variance *
                    left_right_counts;
                posterior.weighted_mean[split_candidate] +=
                    normalized_observation * inv_noise_variance * left_right_counts;
            }
        }

        double max_log_weight = -1e30;
        for (int split_candidate = 0; split_candidate < SEGMENT_LENGTH - 1;
             split_candidate++)
        {
            double precision_det =
                max(det(posterior.precision[split_candidate]), 1e-12);
            double quadratic_form =
                (posterior.weighted_mean[split_candidate] *
                 inv(posterior.precision[split_candidate]) *
                 posterior.weighted_mean[split_candidate].transpose())[0][0];
            posterior.log_weights[split_candidate] =
                -0.5 * log(precision_det) + 0.5 * quadratic_form;
            max_log_weight =
                max(max_log_weight, posterior.log_weights[split_candidate]);
        }
        for (int split_candidate = 0; split_candidate < SEGMENT_LENGTH - 1;
             split_candidate++)
        {
            posterior.log_weights[split_candidate] -= max_log_weight;
        }

        return posterior;
    }

    // サンプルしたセグメントパラメータを適用する。
    void applySegmentParameters(int segment_index,
                                const SegmentParameters& sampled_parameters)
    {
        SegmentPosteriorState& segment = segments[segment_index];

        for (int observation_index : segment.relevant_observation_indices)
        {
            const SegmentObservationCounts& counts =
                segment.observation_counts[observation_index];
            observations[observation_index].current_predicted_cost +=
                (sampled_parameters.left_cost *
                     counts.left_counts[sampled_parameters.split_index] -
                 segment.parameters.left_cost *
                     counts.left_counts[segment.parameters.split_index]) +
                (sampled_parameters.right_cost *
                     counts.right_counts[sampled_parameters.split_index] -
                 segment.parameters.right_cost *
                     counts.right_counts[segment.parameters.split_index]);
        }

        segment.parameters = sampled_parameters;
    }

    // セグメントを再サンプリングする。
    void resampleSegment(int segment_index, mt19937& rng)
    {
        if (segments[segment_index].relevant_observation_indices.empty())
        {
            SegmentParameters sampled_parameters{
                .split_index =
                    uniform_int_distribution{0, SEGMENT_LENGTH - 2}(rng),
                .left_cost =
                    sampleTruncatedNormal(INITIAL_EDGE_COST, INITIAL_EDGE_SIGMA, rng),
                .right_cost =
                    sampleTruncatedNormal(INITIAL_EDGE_COST, INITIAL_EDGE_SIGMA, rng),
            };
            applySegmentParameters(segment_index, sampled_parameters);
            return;
        }

        SegmentPosteriorTerms posterior = buildSegmentPosterior(segment_index);
        SegmentParameters sampled_parameters =
            sampleSegmentParameters(posterior, rng);
        applySegmentParameters(segment_index, sampled_parameters);
    }
};
