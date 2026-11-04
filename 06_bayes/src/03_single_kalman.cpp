// ==== LICENSE BEGIN ====
// ahc_book_samples
// https://github.com/thun-c/ahc_book_samples
//
// MIT License
//
// Copyright (c) 2026 Eita Aoki
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
// ==== LICENSE END ====
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
    // 30×30のグリッドグラフを初期化
    Graph graph(GRID_SIZE);

    // M=2に対応した多変量正規分布に近似したモデルを初期化
    KalmanState kalman(getPriorMean(), getPriorSegmentCovarianceModel2());

    for (int i = 0; i < TOTAL_QUERIES; i++)
    {
        // 1. 始点と終点を標準入力から受け取る
        int si, sj, ti, tj;
        cin >> si >> sj >> ti >> tj;
        int vs = si * GRID_SIZE + sj;
        int vg = ti * GRID_SIZE + tj;

        // 2. 進行状況に応じ、LCBで使う偏差項の重みλ_tに相当するlcb_valueを更新する
        double progress = i / double(TOTAL_QUERIES - 1);
        double lcb_value = exponentialDecrease(LCB_START, LCB_END, progress);

        // 3. LCBで辺長さの期待値と偏差のバランスをとった値mixed_costを全ての辺について更新
        vector<double> mixed_cost(NUM_EDGES, 0.0);
        for (int edge_index = 0; edge_index < NUM_EDGES; edge_index++)
        {
            double kalman_mean = kalman.edgeMean(edge_index);
            double kalman_variance = kalman.edgeVariance(edge_index);
            mixed_cost[edge_index] =
                max(0.0, kalman_mean - lcb_value * sqrt(kalman_variance));
        }
        graph.setCost(mixed_cost);

        // 4. LCBで更新した辺を元に最短経路を計算する
        const auto& [vpath, epath] = graph.findShortestPath(vs, vg);
        vector<PathEdge> path_edges = buildPathEdges(epath);
        output(vpath);

        // 5. 観測値を標準入力から受け取る
        double observed_distance;
        cin >> observed_distance;

        // 6. 観測値を元に事後分布を更新
        kalman.observe(path_edges, observed_distance);
    }
    return 0;
}
