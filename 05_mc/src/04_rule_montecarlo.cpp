#pragma GCC optimize("Ofast")
#include <bits/stdc++.h>

using namespace std;

constexpr int EMPTY = 0;
constexpr int STRAWBERRY = 1;
constexpr int WATERMELON = 2;
constexpr int PUMPKIN = 3;
constexpr int N = 10;
constexpr int BOARD_SIZE = N * N;
constexpr int T = 100;
constexpr int BACKWARD = 0;
constexpr int RIGHT = 1;
constexpr int FORWARD = 2;
constexpr int LEFT = 3;
constexpr int dy[4] = {1, 0, -1, 0};
constexpr int dx[4] = {0, 1, 0, -1};
constexpr int MAX_PLAYOUT_NUM = 500;
constexpr char dirs[4] = {'B', 'R', 'F', 'L'};

array<int, T> f; // 各ターンで追加されるキャンディの種類

mt19937 rng(0);

// 2次元座標 (y, x) を1次元インデックスに変換する。
int calcIndex(int y, int x)
{
    return y * N + x;
}

// 座標 (y, x) が N×N グリッド内かどうかを返す。
bool inGrid(int y, int x)
{
    return 0 <= y && y < N && 0 <= x && x < N;
}

// 標準入力から情報を読み込む。
void input()
{
    for (int i = 0; i < T; ++i)
    {
        cin >> f[i];
    }
}

// 解を出力する
void output(int action)
{
    cout << dirs[action] << '\n';
}

class State
{
public:
    int evaluated_score_ = 0;       // 探索上で評価したスコア
    array<int, BOARD_SIZE> grid_{}; // 盤面
    int t_ = 0;                     // 現在のターン

    // 終了判定
    bool isDone() const
    {
        return t_ >= T;
    }

    // 終了まで進め、そのときのスコアを返す
    int playout()
    {
        while (!isDone())
        {
            int dir;
            if (t_ == T - 1)
            {
                dir = FORWARD;
            }
            else
            {
                int candy = f[t_];
                int next_candy = f[t_ + 1];
                if (next_candy == STRAWBERRY)
                {
                    dir = FORWARD;
                }
                else if (candy == STRAWBERRY)
                {
                    dir = BACKWARD;
                }
                else if (next_candy == WATERMELON)
                {
                    dir = LEFT;
                }
                else if (next_candy == PUMPKIN)
                {
                    dir = RIGHT;
                }
            }
            auto remain = T - t_;
            uniform_int_distribution<int> dist(1, remain);
            int p = dist(rng);
            update(dir, p);
        }
        return getScore();
    }

    // 指定したactionで状態を1手進め、評価する
    void advance(int action, int p)
    {
        update(action, p);
        int total_score = 0;
        for (int i = 0; i < MAX_PLAYOUT_NUM; ++i)
        {
            State playout_state = *this;
            total_score += playout_state.playout();
        }
        evaluated_score_ = total_score;
    }

    // 現在の状況で可能な操作を全て取得する
    vector<int> legalActions() const
    {
        vector<int> legal_actions;
        for (int dir = 0; dir < 4; ++dir)
        {
            legal_actions.push_back(dir);
        }
        return legal_actions;
    }

    // 方向 dir と落ちる位置 p で1ターンだけ盤面を更新する。
    void update(int dir, int p)
    {
        int candy_index = getCandyIndex(p);
        int candy = f[t_];

        grid_[candy_index] = candy;

        switch (dir)
        {
        case FORWARD:
            for (int x = 0; x < N; ++x)
            {
                int target_index = calcIndex(0, x);

                for (int y = 0; y < N; ++y)
                {
                    int index = calcIndex(y, x);
                    if (grid_[index] != EMPTY)
                    {
                        swap(grid_[target_index], grid_[index]);
                        target_index += N;
                    }
                }
            }
            break;
        case RIGHT:
            for (int y = 0; y < N; ++y)
            {
                int target_index = calcIndex(y, N - 1);

                for (int x = N - 1; x >= 0; --x)
                {
                    int index = calcIndex(y, x);
                    if (grid_[index] != EMPTY)
                    {
                        swap(grid_[target_index], grid_[index]);
                        --target_index;
                    }
                }
            }
            break;
        case BACKWARD:
            for (int x = 0; x < N; ++x)
            {
                int target_index = calcIndex(N - 1, x);

                for (int y = N - 1; y >= 0; --y)
                {
                    int index = calcIndex(y, x);
                    if (grid_[index] != EMPTY)
                    {
                        swap(grid_[target_index], grid_[index]);
                        target_index -= N;
                    }
                }
            }
            break;
        case LEFT:
            for (int y = 0; y < N; ++y)
            {
                int target_index = calcIndex(y, 0);

                for (int x = 0; x < N; ++x)
                {
                    int index = calcIndex(y, x);
                    if (grid_[index] != EMPTY)
                    {
                        swap(grid_[target_index], grid_[index]);
                        ++target_index;
                    }
                }
            }
            break;
        }

        ++t_;
    }

    // 空きマスを上から左へ数えて p 番目のマスのインデックスを返す。
    int getCandyIndex(const int p) const
    {
        int count = 0;
        for (int y = 0; y < N; ++y)
        {
            for (int x = 0; x < N; ++x)
            {
                int index = calcIndex(y, x);
                if (grid_[index] == EMPTY)
                {
                    ++count;
                }

                if (count == p)
                {
                    return index;
                }
            }
        }
        return -1;
    }

    // 同色連結成分のサイズの二乗の和をスコアとして計算する。
    int getScore() const
    {
        int score = 0;
        array<bool, BOARD_SIZE> checked{};
        queue<int> q;

        for (int y = 0; y < N; ++y)
        {
            for (int x = 0; x < N; ++x)
            {
                int index = calcIndex(y, x);
                int candy = grid_[index];
                if (candy == EMPTY || checked[index])
                {
                    continue;
                }

                checked[index] = true;
                q.push(index);
                int connected_count = 0;

                while (!q.empty())
                {
                    int now_index = q.front();
                    q.pop();
                    ++connected_count;

                    int now_y = now_index / N;
                    int now_x = now_index % N;
                    for (int dir = 0; dir < 4; ++dir)
                    {
                        int next_y = now_y + dy[dir];
                        int next_x = now_x + dx[dir];
                        if (!inGrid(next_y, next_x))
                        {
                            continue;
                        }

                        int next_index = calcIndex(next_y, next_x);
                        if (checked[next_index] || grid_[next_index] != candy)
                        {
                            continue;
                        }

                        checked[next_index] = true;
                        q.push(next_index);
                    }
                }

                score += connected_count * connected_count;
            }
        }

        return score;
    }
};

// 入力と貪欲法による操作出力を交互に繰り返す
void greedy(const State& state)
{
    State now_state = state;
    while (!now_state.isDone())
    {
        int p;
        cin >> p;
        int max_score = 0;
        int best_action = 0;
        auto best_state = now_state;
        for (int action : now_state.legalActions())
        {
            State next_state = now_state;
            next_state.advance(action, p);
            int score = next_state.evaluated_score_;

            if (max_score < score)
            {
                max_score = score;
                best_action = action;
                best_state = next_state;
            }
        }

        now_state = best_state;
        output(best_action);
    }
}

int main()
{
    input();
    State state;
    greedy(state);

    return 0;
}
