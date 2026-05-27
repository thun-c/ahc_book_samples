#pragma GCC optimize("Ofast")
#include <bits/stdc++.h>
using namespace std;

constexpr int N = 30;
constexpr int ALL_BALL_SIZE = N * (N + 1) / 2;
constexpr const int64_t INF = 1000000000LL; // あり得ないぐらい大きなスコアの例を用意しておく

vector<vector<int>> b;

void input()
{
    b.resize(N);
    for (int x = 0; x < N; ++x)
    {
        b[x] = vector<int>(x + 1);
        for (int y = 0; y <= x; ++y)
        {
            cin >> b[x][y]; // ボールの位置を入力
        }
    }
}

// 状態遷移を行うために必要な情報
// (x1,y1)のボールを(x2,y2)のボールと入れ替える
struct Action
{
    int x1, y1, x2, y2;

    Action(int x1, int y1, int x2, int y2)
        : x1(x1), y1(y1), x2(x2), y2(y2)
    {
    }
};

class State
{
public:
    int evaluated_score_;                            // 探索上で評価したスコア
    vector<vector<int>> b_;                          // ボール番号
    array<pair<int, int>, ALL_BALL_SIZE> positions_; // ボール番号に対応する座標
    int target_ball_;                                // 今正しい位置に置きたいボール番号

    State()
        : b_(b)
    {

        for (int x = 0; x < N; ++x)
        {
            for (int y = 0; y <= x; ++y)
            {
                positions_[b_[x][y]] = {x, y};
            }
        }

        auto new_target_ball = getTarget(0);
        this->target_ball_ = new_target_ball;
        this->evaluated_score_ = 0;
    }

    // 現在の状況で可能な操作を全て取得する
    vector<Action> legalActions() const
    {
        // 探索終了状態では参照せずに空操作を返す
        if (isDone())
        {
            return {};
        }

        std::vector<Action> actions{};
        auto [x, y] = positions_[target_ball_];

        if (canMoveLeft(x, y))
        {
            actions.emplace_back(x, y, x - 1, y - 1);
        }
        if (canMoveRight(x, y))
        {
            actions.emplace_back(x, y, x - 1, y);
        }
        return actions;
    }

    // 終了判定
    bool isDone() const
    {
        return this->target_ball_ == ALL_BALL_SIZE;
    }

    // actionを実行して次の状態に遷移する
    void advance(Action action)
    {
        evaluated_score_ = b_[action.x2][action.y2];
        swapBalls(action.x1, action.y1, action.x2, action.y2);
        auto new_target_ball = getTarget(target_ball_);
        target_ball_ = new_target_ball;
    }

    void swapBalls(int x1, int y1, int x2, int y2)
    {
        int b1 = b_[x1][y1];
        int b2 = b_[x2][y2];
        b_[x1][y1] = b2;
        b_[x2][y2] = b1;
        positions_[b2] = {x1, y1};
        positions_[b1] = {x2, y2};
    }

    bool canMoveLeft(int x, int y) const
    {
        return y > 0 && b_[x - 1][y - 1] > b_[x][y];
    }

    bool canMoveRight(int x, int y) const
    {
        return y < x && b_[x - 1][y] > b_[x][y];
    }

    int getTarget(int target_ball) const
    {
        while (target_ball < ALL_BALL_SIZE)
        {
            auto [x, y] = positions_[target_ball];
            if (canMoveLeft(x, y) || canMoveRight(x, y))
            {
                break;
            }
            else
            {
                ++target_ball;
            }
        }
        return target_ball;
    }
};

// 貪欲法で操作列を決定する
std::vector<Action> greedy(const State& state)
{
    std::vector<Action> best_actions{};
    State now_state = state;
    while (!now_state.isDone())
    {
        int64_t best_score = -INF;          // 絶対にありえない小さな値でベストスコアを初期化する
        Action best_action(-1, -1, -1, -1); // ありえない行動で初期化する
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
void output(const vector<Action>& answer)
{
    cout << answer.size() << endl;
    for (Action action : answer)
    {
        cout << action.x1 << " " << action.y1 << " " << action.x2 << " " << action.y2 << endl;
    }
}

int main()
{
    input();
    State state{};
    vector<Action> answer = greedy(state);
    cerr << "action count: " << answer.size() << endl;
    output(answer);

    return 0;
}
