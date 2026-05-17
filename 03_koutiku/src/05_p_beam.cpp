#pragma GCC optimize("Ofast")
#include <bits/stdc++.h>
using namespace std;

constexpr int N = 30;
constexpr int ALL_BALL_SIZE = N * (N + 1) / 2;

constexpr size_t BEAM_WIDTH = 60;
constexpr int TARGET_COEFFICIENT = 600;

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
    int p_;                                          // 初期値から見たi*ボールiの高さの相対値の総和

    vector<Action> actions_;
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
        this->p_ = 0;
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
        p_ += b_[action.x2][action.y2] - b_[action.x1][action.y1];
        swapBalls(action.x1, action.y1, action.x2, action.y2);
        auto new_target_ball = getTarget(target_ball_);
        target_ball_ = new_target_ball;
        evaluated_score_ = TARGET_COEFFICIENT * target_ball_ + p_;
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

bool operator>(const State& state_1, const State& state_2)
{
    return state_1.evaluated_score_ > state_2.evaluated_score_;
}

// ビーム幅を指定してビームサーチで操作列を決定する
vector<Action> beamSearch(
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
void output(const vector<Action>& answer)
{
    cout << answer.size() << "\n";
    for (Action action : answer)
    {
        cout << action.x1 << " " << action.y1 << " " << action.x2 << " " << action.y2 << "\n";
    }
}

int main()
{
    input();
    State state{};
    vector<Action> answer = beamSearch(state, BEAM_WIDTH);
    cerr << "action count: " << answer.size() << endl;
    output(answer);

    return 0;
}
