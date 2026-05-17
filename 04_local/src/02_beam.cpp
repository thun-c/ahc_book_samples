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
    vector<int> actions_;
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

bool operator>(const State& state_1, const State& state_2)
{
    return state_1.evaluated_score_ > state_2.evaluated_score_;
}

// ビーム幅を指定してビームサーチで操作列を決定する
vector<int> beamSearch(const State& state, const int beam_width)
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

        if (now_beam.size() == 0)
        {
            break;
        }
        if (best_state.evaluated_score_ < now_beam[0].evaluated_score_)
        {
            best_state = now_beam[0];
        }
    }
    return best_state.actions_;
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
    auto actions = beamSearch(state, 1000);
    int visited_tiles_count = actions.size() + 1; // 初期位置を含む
    cerr << "visited tiles count: " << visited_tiles_count << endl;
    output(actions);
    return 0;
}
