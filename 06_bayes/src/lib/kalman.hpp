#pragma GCC optimize("Ofast")
#pragma once
#include <bits/stdc++.h>
using namespace std;

#include "lib/constants.hpp"
#include "lib/edge_mapping.hpp"

using PriorSegmentCovariance =
    array<array<double, SEGMENT_LENGTH>, SEGMENT_LENGTH>;

/*************************************************************************
 * 04_double_kalman.cpp から必要:
 * M=1 を仮定した事前共分散を生成する。
 *************************************************************************/
PriorSegmentCovariance getPriorSegmentCovarianceModel1()
{
    PriorSegmentCovariance covariance{};
    for (int i = 0; i < SEGMENT_LENGTH; i++)
    {
        for (int j = 0; j < SEGMENT_LENGTH; j++)
        {
            if (i == j)
            {
                covariance[i][j] = E_VAR_H_GIVEN_D + E_VAR_DELTA_GIVEN_D;
            }
            else
            {
                covariance[i][j] = E_VAR_H_GIVEN_D;
            }
        }
    }
    return covariance;
}

/*************************************************************************
 * 03_single_kalman.cpp から必要:
 * M=2 を仮定した事前共分散を生成する。
 *************************************************************************/
PriorSegmentCovariance getPriorSegmentCovarianceModel2()
{
    PriorSegmentCovariance covariance{};
    for (int i = 0; i < SEGMENT_LENGTH; i++)
    {
        for (int j = 0; j < SEGMENT_LENGTH; j++)
        {
            if (i == j)
            {
                covariance[i][j] = E_VAR_H_GIVEN_D + E_VAR_DELTA_GIVEN_D;
            }
            else
            {
                covariance[i][j] =
                    E_VAR_H_GIVEN_D * (1.0 - fabs((double)i - (double)j) / 28.0);
            }
        }
    }
    return covariance;
}

/*************************************************************************
 * 03_single_kalman.cpp から必要:
 * 全辺の事前平均を同一値で初期化する。
 *************************************************************************/
vector<double> getPriorMean()
{
    return vector<double>(NUM_EDGES, INITIAL_EDGE_COST);
}

struct KalmanState
{
    int n;               // 潜在状態ベクトルの次元（ここでは辺の数）
    vector<double> mu;   // 各変数（var_index順）の事後平均。
    vector<double> diag; // 事後共分散の対角成分（各変数の分散）。
    PriorSegmentCovariance
        base_covariance{}; // 更新時に使うセグメント単位の事前共分散ブロック。
    vector<array<double, NUM_EDGES>>
        c;                           // 低ランク（rank-1）共分散更新で保存する c_t ベクトル群。
    vector<double> minus_inv_sigmas; // 各時刻の -1 / sigma_bb_t。
    int t;                           // これまでに処理した観測回数。
    double log_likelihood;           // 04_double_kalman.cpp から必要: モデル比較用の累積対数尤度。

    KalmanState(
        const vector<double>& initial_mean_edge_order,
        const PriorSegmentCovariance& prior_segment_covariance)
        : n(NUM_EDGES), mu(NUM_EDGES), diag(NUM_EDGES), base_covariance(prior_segment_covariance), c(TOTAL_QUERIES), minus_inv_sigmas(TOTAL_QUERIES), t(0), log_likelihood(0.0)
    {
        // 初期事後平均を初期辺長ベクトルから設定
        for (int edge = 0; edge < NUM_EDGES; edge++)
        {
            int var_index = EDGE_MAPPING.edge_to_var[edge];
            mu[var_index] = initial_mean_edge_order[edge];
        }

        // 初期事後共分散をセグメント単位の事前共分散ブロックから設定
        for (int segment = 0; segment < SEGMENT_COUNT; segment++)
        {
            int offset = segment * SEGMENT_LENGTH;
            for (int bin = 0; bin < SEGMENT_LENGTH; bin++)
            {
                diag[offset + bin] = base_covariance[bin][bin];
            }
        }
    }

    void observe(const vector<PathEdge>& path_edges, double observed_cost)
    {
        // 経路上の事後平均を計算しつつ、c_t の P_base * b_t 相当を計算
        auto& c_t = c[t];
        c_t.fill(0.0);
        double mu_b_t = 0.0;
        for (const PathEdge& path_edge : path_edges)
        {
            int segment = path_edge.segment_index;
            int bin = path_edge.bin_index;
            int var_index = path_edge.var_index;
            mu_b_t += mu[var_index];

            int segment_offset = segment * SEGMENT_LENGTH;
            for (int row = 0; row < SEGMENT_LENGTH; row++)
            {
                c_t[segment_offset + row] += base_covariance[row][bin];
            }
        }

        // この時点でc_tはP_base * b_t 相当の値になっている
        // ここにΣ_{r=1}^{t-1} (-1 / sigma_bb_r) * c_r * (c_r^T b_t) 相当を足し込む
        for (int rank = 0; rank < t; rank++)
        {
            const auto& c_r = c[rank];
            double minus_inv_sigma_r = minus_inv_sigmas[rank];
            double c_r_dot_b_t = 0.0;
            for (const PathEdge& path_edge : path_edges)
            {
                c_r_dot_b_t += c_r[path_edge.var_index];
            }
            double scale = minus_inv_sigma_r * c_r_dot_b_t;
            if (scale == 0.0)
                continue;
            for (int i = 0; i < n; i++)
            {
                c_t[i] += scale * c_r[i];
            }
        }

        // sigma_bb_t の計算に使う b_t^T c_t を経路上の c_t 成分の和として計算
        double s_t = 0.0;
        for (const PathEdge& path_edge : path_edges)
        {
            s_t += c_t[path_edge.var_index];
        }

        double sigma_bb_t = (1.0 + MULTIPLICATIVE_NOISE_VARIANCE) * s_t +
                            MULTIPLICATIVE_NOISE_VARIANCE * mu_b_t * mu_b_t +
                            ROUNDING_NOISE_VARIANCE;
        // sigma_bb_t は正の値になるようにする
        sigma_bb_t = max(sigma_bb_t, 1e-6);

        double z_t_minus_mu_b_t = observed_cost - mu_b_t;
        double inv_sigma_bb_t = 1.0 / sigma_bb_t;
        // 平均と分散を更新
        for (int i = 0; i < n; i++)
        {
            double c_t_i = c_t[i];
            mu[i] += c_t_i * inv_sigma_bb_t * z_t_minus_mu_b_t;
            diag[i] -= c_t_i * inv_sigma_bb_t * c_t_i;
        }

        // 次の更新で使う -1 / sigma_bb_t を保存
        minus_inv_sigmas[t] = -inv_sigma_bb_t;
        // 観測回数をインクリメント
        t++;

        // 対数尤度を更新 04_double_kalman.cpp から必要: モデル比較用の累積対数尤度。
        log_likelihood += -0.5 * log(2.0 * M_PI * sigma_bb_t) -
                          z_t_minus_mu_b_t * z_t_minus_mu_b_t / (2.0 * sigma_bb_t);
    }

    double edgeMean(int edge_index) const
    {
        return mu[getPathEdge(edge_index).var_index];
    }

    double edgeVariance(int edge_index) const
    {
        return max(0.0, diag[getPathEdge(edge_index).var_index]);
    }

    /*************************************************************************
     * 05_kalman_mcmc.cpp から必要:
     * MCMC 観測ノイズ近似のために、経路上の事後平均和を返す。
     *************************************************************************/
    double pathMeanSum(const vector<PathEdge>& path_edges) const
    {
        double sum_mu = 0.0;
        for (const PathEdge& path_edge : path_edges)
        {
            sum_mu += mu[path_edge.var_index];
        }
        return sum_mu;
    }
};
