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
#include "lib/constants.hpp"
using namespace std;

void output(const vector<int>& path)
{
    // pathは座標のリストになっているため、移動方向に直して出力する
    string res;
    for (int i = 0; i < path.size() - 1; i++)
    {
        int diff = path[i + 1] - path[i];
        if (diff == 1)
        {
            res.push_back('R');
        }
        else if (diff == -1)
        {
            res.push_back('L');
        }
        else if (diff == GRID_SIZE)
        {
            res.push_back('D');
        }
        else if (diff == -GRID_SIZE)
        {
            res.push_back('U');
        }
    }
    cout << res << endl;
}

struct Edge
{
    int st;
    int ed;
    double cost;
    Edge(int st_, int ed_, double cost_)
        : st(st_), ed(ed_), cost(cost_)
    {
    }
};

struct Graph
{
    int V;                        // 頂点数
    int E;                        // 辺数
    vector<Edge> edges;           // 辺のリスト
    vector<vector<int>> adj_list; // 隣接リスト

    Graph(int grid_size)
    {
        V = grid_size * grid_size;
        vector<int> st, ed;
        for (int i = 0; i < grid_size; i++)
        {
            for (int j = 0; j < grid_size - 1; j++)
            {
                int v = i * grid_size + j;
                int v2 = i * grid_size + (j + 1);
                st.emplace_back(v);
                ed.emplace_back(v2);
            }
        }
        for (int i = 0; i < grid_size - 1; i++)
        {
            for (int j = 0; j < grid_size; j++)
            {
                int v = i * grid_size + j;
                int v2 = (i + 1) * grid_size + j;
                st.emplace_back(v);
                ed.emplace_back(v2);
            }
        }
        E = static_cast<int>(st.size());
        for (int i = 0; i < E; i++)
            edges.emplace_back(Edge(st[i], ed[i], 1.0));

        adj_list = vector<vector<int>>(V);
        for (int i = 0; i < E; i++)
        {
            adj_list[st[i]].emplace_back(i);
            adj_list[ed[i]].emplace_back(i);
        }
    }

    void setCost(const vector<double>& cost)
    {
        for (int i = 0; i < E; i++)
            edges[i].cost = cost[i];
    }

    pair<vector<int>, vector<int>> findShortestPath(const int start,
                                                    const int goal)
    {
        const double INFTY = numeric_limits<double>::infinity();
        vector<double> dist(V, INFTY);
        vector<int> from_memo(V, -1);
        using State = pair<double, int>;
        priority_queue<State, vector<State>, greater<State>> pq;
        dist[start] = 0.0;
        pq.push({0.0, start});
        while (!pq.empty())
        {
            auto [new_dist, v] = pq.top();
            pq.pop();
            if (new_dist > dist[v])
                continue;
            if (v == goal)
                break;
            for (int eid : adj_list[v])
            {
                Edge& e = edges[eid];
                int v2 = (e.st == v) ? e.ed : e.st;
                double cand_dist = new_dist + e.cost;
                if (cand_dist >= dist[v2])
                    continue;
                dist[v2] = cand_dist;
                from_memo[v2] = eid;
                pq.push({cand_dist, v2});
            }
        }

        vector<int> vpath = {goal}, epath;
        int now = goal;
        while (now != start)
        {
            int eid = from_memo[now];
            Edge& e = edges[eid];
            int prev = (e.st == now) ? e.ed : e.st;
            vpath.emplace_back(prev);
            epath.emplace_back(eid);
            now = prev;
        }
        reverse(vpath.begin(), vpath.end());
        reverse(epath.begin(), epath.end());
        return {vpath, epath};
    }
};
