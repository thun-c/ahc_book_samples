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
