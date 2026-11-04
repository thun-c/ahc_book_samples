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

constexpr int H = 50;         // 盤面の縦方向の大きさ
constexpr int W = 50;         // 盤面の横方向の大きさ
constexpr int TILE_N = H * W; // 多めに見積もったタイルの総数
int sy, sx;
int first_coord;   // 高橋くんの初期位置:(y,x)はy*W+xとして表現する
int tiles[H * W];  // tiles[coord1]とtiles[coord2]が同じなら同じタイル
int points[H * W]; // tiles[coord]を訪れた場合の得点
constexpr int dy[4] = {1, 0, -1, 0};
constexpr int dx[4] = {0, 1, 0, -1};
constexpr char dirs[4] = {'D', 'R', 'U', 'L'};
constexpr int INF = 10000000; // あり得ないぐらい大きなスコアの例を用意しておく

// 入力を受け取る
void input()
{
    cin >> sy >> sx;
    first_coord = sy * W + sx;
    for (int y = 0; y < H; y++)
    {
        for (int x = 0; x < W; x++)
        {
            cin >> tiles[y * W + x];
        }
    }
    for (int y = 0; y < H; y++)
    {
        for (int x = 0; x < W; x++)
        {
            cin >> points[y * W + x];
        }
    }
}

class State
{
public:
    int evaluated_score_; // 探索上で評価したスコア
    int y, x;
    // visiteds_[tile]はtileが踏まれたかどうかを表す
    array<bool, TILE_N> visiteds_;
    State()
    {
        y = sy;
        x = sx;
        evaluated_score_ = 0;
        visiteds_.fill(false);
        visiteds_[tiles[first_coord]] = true;
    }

    // 終了判定
    bool isDone() const
    {
        auto action = legalActions();
        return action.size() == 0;
    }

    // 指定したactionで状態を1手進め、評価する
    void advance(const int action)
    {

        y += dy[action];
        x += dx[action];
        auto next_coord = y * W + x;
        visiteds_[tiles[next_coord]] = true;
        evaluated_score_ += points[next_coord];
    }

    // 現在の状況で可能な操作を全て取得する
    std::vector<int> legalActions() const
    {
        std::vector<int> legal_actions;
        for (int d = 0; d < 4; d++)
        {
            int ty = y + dy[d];
            int tx = x + dx[d];
            if (ty >= 0 && ty < H && tx >= 0 && tx < W)
            {
                int next_y = ty;
                int next_x = tx;
                int next_coord = next_y * W + next_x;
                if (!visiteds_[tiles[next_coord]])
                {
                    legal_actions.emplace_back(d);
                }
            }
        }
        return legal_actions;
    }
};

// 貪欲法で操作列を決定する
std::vector<int> greedy(const State& state)
{
    std::vector<int> best_actions{};
    State now_state = state;
    while (!now_state.isDone())
    {
        int64_t best_score =
            -INF;             // 絶対にありえない小さな値でベストスコアを初期化する
        int best_action = -1; // ありえない行動で初期化する
        auto legal_actions = now_state.legalActions();
        for (const auto action : legal_actions)
        {
            auto next_state = now_state;
            next_state.advance(action);
            if (next_state.evaluated_score_ > best_score)
            {
                best_score = next_state.evaluated_score_;
                best_action = action;
            }
        }
        best_actions.emplace_back(best_action);
        now_state.advance(best_action);
    }
    return best_actions;
}

// 解を出力する
void output(const vector<int>& actions)
{
    for (const auto action : actions)
    {
        cout << dirs[action];
    }
    cout << endl;
}

int main()
{
    input();
    State state{};
    auto actions = greedy(state);
    int visited_tiles_count = actions.size() + 1; // 初期位置を含む
    cerr << "visited tiles count: " << visited_tiles_count << endl;
    output(actions);
    return 0;
}
