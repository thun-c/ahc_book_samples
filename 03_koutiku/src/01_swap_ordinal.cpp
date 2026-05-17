#pragma GCC optimize("Ofast")
#include <bits/stdc++.h>
using namespace std;

constexpr int N = 30;

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
    vector<vector<int>> b_; // ボール番号

    State()
        : b_(b)
    {
    }

    void swapBalls(int x1, int y1, int x2, int y2)
    {
        int b1 = b_[x1][y1];
        int b2 = b_[x2][y2];
        b_[x1][y1] = b2;
        b_[x2][y2] = b1;
    }

    bool canMoveLeft(int x, int y) const
    {
        return y > 0 && b_[x - 1][y - 1] > b_[x][y];
    }

    bool canMoveRight(int x, int y) const
    {
        return y < x && b_[x - 1][y] > b_[x][y];
    }
};

// 操作列を決定する
std::vector<Action> solve(const State& state)
{
    std::vector<Action> best_actions{};
    State now_state = state;
    while (true)
    {
        bool changed = false;
        for (int x = 1; x < N; ++x)
        {
            for (int y = 0; y <= x; ++y)
            {
                if (now_state.canMoveLeft(x, y))
                {
                    now_state.swapBalls(x, y, x - 1, y - 1);
                    best_actions.emplace_back(x, y, x - 1, y - 1);
                    changed = true;
                }
                if (now_state.canMoveRight(x, y))
                {
                    now_state.swapBalls(x, y, x - 1, y);
                    best_actions.emplace_back(x, y, x - 1, y);
                    changed = true;
                }
            }
        }

        if (!changed)
        {
            break;
        }
    }
    return best_actions;
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
    vector<Action> answer = solve(state);
    cerr << "action count: " << answer.size() << endl;
    output(answer);

    return 0;
}
