

#pragma GCC optimize("Ofast")
#include <bits/stdc++.h>
using namespace std;

constexpr int D = 365; // 日数
constexpr int CN = 26; // コンテストの種類数
int c[CN];             // コンテストの満足度の下がりやすさ
int s[D][CN];          // ある日付にコンテストを開催したときの満足度の増加量

struct Timer
{
    chrono::system_clock::time_point start;

    Timer()
    {
        start = chrono::system_clock::now();
    }

    void reset()
    {
        start = chrono::system_clock::now();
    }

    //@brief プログラムの実行時間を取得する
    //@return プログラムの実行時間(秒)
    double getTime()
    {
        auto now = chrono::system_clock::now();
        return chrono::duration<double>(now - start).count();
    }
};

// startからendまでの範囲でprogressに応じ、最初に急激に減少、その後緩やかに減少した値をとる
double exponentialDecrease(double start, double end, double progress)
{
    return pow(start, 1.0 - progress) * pow(end, progress);
}

Timer timer;

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
mt19937 rng(0);
uniform_real_distribution<double> real_rng(0.0, 1.0);
// コンテストが開催されなかった期間の満足度の下がり具合は、
// c[i]*(1+2+3+...+(elapsed-1))
// で表される
// (1+2+3+...+(elapsed-1))は、和の公式により
// elapsed*(elapsed-1)/2
// と計算できるので、これを計算する
inline int dayCumsum(const int last_day, const int now_day)
{
    int elapsed = now_day - last_day;
    return elapsed * (elapsed - 1) / 2;
}

class State
{
public:
    int64_t evaluated_score_; // 探索上で評価したスコア
    int day_;                 // 日付
    vector<int> last_days_;   // 各コンテストを最後に開いた日付
    State()
    {
        evaluated_score_ = 1e6;
        day_ = 0;
        last_days_ = vector<int>(CN, -1);
    }

    // 終了判定
    bool isDone() const
    {
        return this->day_ == D;
    }

    // 指定したactionで状態を1手進め、評価する
    void advance(const int action)
    {
        this->evaluated_score_ += s[this->day_][action];
        this->last_days_[action] = this->day_;
        for (int i = 0; i < CN; i++)
        {
            this->evaluated_score_ -= c[i] * (this->day_ - this->last_days_[i]);
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

// 貪欲法で操作列を決定する
std::vector<int> greedy(const State& state)
{
    std::vector<int> best_actions{};
    State now_state = state;
    while (!now_state.isDone())
    {
        int64_t best_score = -INF; // 絶対にありえない小さな値でベストスコアを初期化する
        int best_action = -1;      // ありえない行動で初期化する
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

class LocalSearchSolver
{
public:
    // contests_[d]はd日目に開かれるコンテスト
    vector<int> contests_;
    vector<int> best_contests_;
    LocalSearchSolver(std::vector<int> contests)
    {
        this->contests_ = contests;
        this->best_contests_ = contests;
    }

    // 全体のスコアを計算する
    int64_t computeWholeScore()
    {
        int score = 1e6;
        vector<int> lst(CN, -1);
        for (int d = 0; d < D; d++)
        {
            int bef_lst = lst[contests_[d]];
            lst[contests_[d]] = d;
            score += s[d][contests_[d]];
            score -= c[contests_[d]] * dayCumsum(bef_lst, d);
        }
        for (int i = 0; i < CN; i++)
        {
            score -= c[i] * dayCumsum(lst[i], D);
        }
        return score;
    }

    // 焼きなまし法でコンテストを決定する
    void simulatedAnnealing()
    {
        double start_temperature = 1000000;
        double end_temperature = 100;
        auto now_score = computeWholeScore();
        auto best_score = now_score;
        double end_time = 1.9;
        int cnt = 0;
        while (true)
        {
            double elapsed_time = timer.getTime();
            if (elapsed_time > end_time)
            {
                break;
            }
            double temperature = exponentialDecrease(start_temperature, end_temperature, elapsed_time / end_time);
            int coin = rng() % 2;
            if (coin == 0)
            {
                // 特定の日付のコンテストを異なるコンテストにする
                int day = rng() % D;
                int next_contest = rng() % CN;
                int before_contest = contests_[day];
                if (before_contest == next_contest)
                {
                    continue;
                }
                contests_[day] = next_contest;
                auto next_score = computeWholeScore();
                auto diff = next_score - now_score;
                if (diff > 0 || exp(diff / temperature) > real_rng(rng))
                {
                    now_score = next_score;
                    if (next_score > best_score)
                    {
                        best_score = next_score;
                        best_contests_ = contests_;
                    }
                }
                else
                {
                    // 状態を元に戻す
                    contests_[day] = before_contest;
                }
            }
            else
            {
                // 2つの日付のコンテストを入れ替える
                int day1 = rng() % D;
                int day2 = rng() % D;
                if (contests_[day1] == contests_[day2])
                {
                    continue;
                }
                swap(contests_[day1], contests_[day2]);
                auto next_score = computeWholeScore();
                auto diff = next_score - now_score;
                if (diff > 0 || exp(diff / temperature) > real_rng(rng))
                {
                    now_score = next_score;
                    if (next_score > best_score)
                    {
                        best_score = next_score;
                        best_contests_ = contests_;
                    }
                }
                else
                {
                    // 状態を元に戻す
                    swap(contests_[day1], contests_[day2]);
                }
            }
            ++cnt;
        }
        cerr << "cnt: " << cnt << endl;
    }
};

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
    auto actions = greedy(state);
    LocalSearchSolver solver(actions);
    solver.simulatedAnnealing();
    output(solver.best_contests_);
    return 0;
}