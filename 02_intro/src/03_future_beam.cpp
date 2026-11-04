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

constexpr int D = 365; // 日数
constexpr int CN = 26; // コンテストの種類数
int c[CN];             // コンテストの満足度の下がりやすさ
int s[D][CN];          // ある日付にコンテストを開催したときの満足度の増加量

void input()
{
    int tmp;
    cin >> tmp; // Dは固定値なので読み飛ばす
    for (int i = 0; i < CN; i++)
        cin >> c[i];
    for (int i = 0; i < D; i++)
        for (int j = 0; j < CN; j++)
            cin >> s[i][j];
}

constexpr const int64_t INF = 1000000000LL; // あり得ないぐらい大きなスコアの例を用意しておく

// コンテストが開催されなかった期間の満足度減少の係数を計算する
inline int dayCumsum(const int last_day, const int now_day)
{
    int elapsed = now_day - last_day;
    int sum = 0;
    for (int d = last_day + 1; d <= now_day - 1; d++)
    {
        sum += d - last_day;
    }
    return sum;
}

class State
{
public:
    int64_t actual_score_;    // 実際のスコア
    int64_t evaluated_score_; // 探索上で評価したスコア
    int day_;                 // 日付
    vector<int> last_days_;   // 各コンテストを最後に開いた日付
    vector<int> actions_;     // 操作の履歴
    State()
    {
        actual_score_ = 1e6;
        evaluated_score_ = actual_score_;
        day_ = 0;
        last_days_ = vector<int>(CN, -1);
        actions_ = vector<int>();
    }

    // 終了判定
    bool isDone() const
    {
        return this->day_ == D;
    }

    // 指定したactionで状態を1手進め、評価する
    void advance(const int action)
    {
        this->actual_score_ += s[this->day_][action];
        this->last_days_[action] = this->day_;
        for (int i = 0; i < CN; i++)
        {
            this->actual_score_ -= c[i] * (this->day_ - this->last_days_[i]);
        }
        this->evaluated_score_ = this->actual_score_;
        for (int i = 0; i < CN; i++)
        {
            auto check_day = this->day_ + 4;
            if (check_day >= D)
            {
                check_day = D - 1;
            }
            auto score_gap =
                dayCumsum(
                    this->last_days_[i], check_day + 1) -
                dayCumsum(
                    this->last_days_[i], this->day_ + 1);
            this->evaluated_score_ -= c[i] * score_gap;
        }
        ++this->day_;
    }

    // 現在の状況で可能な操作を全て取得する
    std::vector<int> legalActions() const
    {
        std::vector<int> actions{};
        for (int action = 0; action < CN; action++)
        {
            actions.emplace_back(action);
        }

        return actions;
    }
};

bool operator>(const State& state_1, const State& state_2)
{
    return state_1.evaluated_score_ > state_2.evaluated_score_;
}

// ビーム幅を指定してビームサーチで操作列を決定する
vector<int> beamSearch(
    const State& state,
    const int beam_width)
{
    vector<State> now_beam;
    State best_state;

    now_beam.emplace_back(state);
    for (int t = 0;; t++)
    {
        vector<State> next_beam;
        for (const State& now_state : now_beam)
        {
            auto legal_actions = now_state.legalActions();
            for (const auto& action : legal_actions)
            {
                State next_state = now_state;
                next_state.advance(action);
                next_state.actions_.emplace_back(action);
                next_beam.emplace_back(next_state);
            }
        }
        sort(next_beam.begin(), next_beam.end(), greater<State>());
        next_beam.resize(min(beam_width, (int)next_beam.size()));

        now_beam = next_beam;
        best_state = now_beam[0];

        if (best_state.isDone())
        {
            break;
        }
    }
    return best_state.actions_;
}

// 解を出力する
void output(const std::vector<int>& actions)
{
    for (const auto action : actions)
    {
        // コンテストを0-indexedで計算していたため、1-indexedで出力する
        cout << action + 1 << endl;
    }
}

int main()
{
    input();
    State state{};

    auto actions = beamSearch(state, 200);
    output(actions);
    return 0;
}