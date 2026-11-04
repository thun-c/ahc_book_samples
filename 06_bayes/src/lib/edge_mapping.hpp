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
#include "lib/constants.hpp"
#include <bits/stdc++.h>
using namespace std;

struct EdgeMapping {
  array<int, NUM_EDGES> edge_to_segment{};
  array<int, NUM_EDGES> edge_to_bin{};
  array<int, NUM_EDGES> edge_to_var{};
  array<int, NUM_EDGES> var_to_edge{};
};

EdgeMapping buildEdgeMapping() {
  EdgeMapping mapping{};

  for (int y = 0; y < GRID_SIZE; y++) {
    for (int x = 0; x < GRID_SIZE - 1; x++) {
      int edge = y * (GRID_SIZE - 1) + x;
      int segment = y;
      int bin = x;
      int var_index = segment * SEGMENT_LENGTH + bin;
      mapping.edge_to_segment[edge] = segment;
      mapping.edge_to_bin[edge] = bin;
      mapping.edge_to_var[edge] = var_index;
      mapping.var_to_edge[var_index] = edge;
    }
  }

  for (int y = 0; y < GRID_SIZE - 1; y++) {
    for (int x = 0; x < GRID_SIZE; x++) {
      int edge = HORIZONTAL_EDGE_COUNT + y * GRID_SIZE + x;
      int segment = GRID_SIZE + x;
      int bin = y;
      int var_index = segment * SEGMENT_LENGTH + bin;
      mapping.edge_to_segment[edge] = segment;
      mapping.edge_to_bin[edge] = bin;
      mapping.edge_to_var[edge] = var_index;
      mapping.var_to_edge[var_index] = edge;
    }
  }

  return mapping;
}

EdgeMapping EDGE_MAPPING = buildEdgeMapping();

struct PathEdge {
  int segment_index;
  int bin_index;
  int var_index;
};

PathEdge getPathEdge(int edge_index) {
  return {
      EDGE_MAPPING.edge_to_segment[edge_index],
      EDGE_MAPPING.edge_to_bin[edge_index],
      EDGE_MAPPING.edge_to_var[edge_index],
  };
}

vector<PathEdge> buildPathEdges(const vector<int> &edge_indices) {
  vector<PathEdge> path_edges;
  path_edges.reserve(edge_indices.size());
  for (int edge_index : edge_indices) {
    path_edges.push_back(getPathEdge(edge_index));
  }
  return path_edges;
}
