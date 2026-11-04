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
#pragma once
#include <bits/stdc++.h>
using namespace std;

/*************************************************************************
 * 01_dijkstra.cpp から必要:
 * グリッドサイズ・クエリ数・辺の添字付けに使う基本定数。
 *************************************************************************/
constexpr int GRID_SIZE = 30;
constexpr int TOTAL_QUERIES = 1000;
constexpr int SEGMENT_COUNT = 2 * GRID_SIZE;
constexpr int SEGMENT_LENGTH = GRID_SIZE - 1;
constexpr int NUM_EDGES = SEGMENT_COUNT * SEGMENT_LENGTH;
constexpr int HORIZONTAL_EDGE_COUNT = GRID_SIZE * (GRID_SIZE - 1);
constexpr double MIN_EDGE_COST = 1000.0;
constexpr double MAX_EDGE_COST = 9000.0;
constexpr double INITIAL_EDGE_COST = (MIN_EDGE_COST + MAX_EDGE_COST) / 2.0;

/*************************************************************************
 * 02_simple_mean.cpp からの追加定数はなし
 *************************************************************************/

/*************************************************************************
 * 03_single_kalman.cpp から必要:
 * LCB探索・初期事前分布・観測ノイズ近似に使う定数。
 *************************************************************************/
constexpr double LCB_START = 1.5; // ハイパーパラメータ
constexpr double LCB_END = 0.1;   // ハイパーパラメータ

// 事前分散/共分散の近似:
// 生成モデルを D ~ U(100,2000), H|D ~ U(1000 + D, 9000 - D),
// δ|D ~ U(-D, D), h = H + δ とおく。
// 一様分布の分散 Var(U(a,b)) = (b-a)^2 / 12 を使うと、
// E[Var(H|D)] = E[(8000 - 2D)^2 / 12] = 3001111.111...
// E[Var(δ|D)] = E[(2D)^2 / 12] = E[D^2 / 3] = 467777.778...
// よって 1辺の事前分散は
// Var(h) ≈ E[Var(H|D)] + E[Var(δ|D)] = 3468888.889...
// また同一セグメント内の別 bin は共通成分 H を共有する近似なので、
// Cov(h_i, h_j) ≈ E[Var(H|D)] = 3001111.111...
constexpr double E_VAR_H_GIVEN_D = 3001111.1;
constexpr double E_VAR_DELTA_GIVEN_D = 467777.8;

// 観測分散の近似:
// e を [0.9, 1.1] の一様分布とし、観測値 y = 四捨五入(a * e) を
// y ~= a + a * 微小ゆらぎ + 丸め誤差 とみなし、微小ゆらぎの分散を
// 0.2^2/12、丸め誤差の分散を 1/12 とする。
constexpr double MULTIPLICATIVE_NOISE_VARIANCE = (0.2 * 0.2) / 12.0;
constexpr double ROUNDING_NOISE_VARIANCE = 1.0 / 12.0;

/*************************************************************************
 * 04_double_kalman.cpp からの追加定数はなし
 *************************************************************************/

/*************************************************************************
 * 05_kalman_mcmc.cpp から必要:
 * MCMC の再サンプリング回数と分割初期値、およびサンプル分布の初期sigma。
 *************************************************************************/
constexpr int RESAMPLE_COUNT = 6;
constexpr int INITIAL_SPLIT_INDEX = 14;
constexpr double INITIAL_EDGE_SIGMA = sqrt(E_VAR_H_GIVEN_D);
