#pragma GCC optimize("Ofast")
#include <bits/stdc++.h>
using namespace std;

constexpr int H = 50;                   // 盤面の縦方向の大きさ
constexpr int W = 50;                   // 盤面の横方向の大きさ
constexpr int TILE_N = H * W;           // 多めに見積もったタイルの総数
int first_coord;                        // 高橋くんの初期位置:(y,x)はy*W+xとして表現する
int tiles[H * W];                       // tiles[coord1]とtiles[coord2]が同じなら同じタイル
int points[H * W];                      // tiles[coord]を訪れた場合の得点
vector<vector<int>> next_coords{H * W}; // next_coords[coord]=coordの隣のリスト

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
Timer timer{};

// 入力を受け取る
void input()
{
    int sy, sx;
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

void preprocess()
{
    // 各座標から行ける場所(盤面の枠内かつタイルが異なる)をメモしておく
    constexpr int dy[4] = {1, 0, -1, 0};
    constexpr int dx[4] = {0, 1, 0, -1};
    for (int y = 0; y < H; y++)
    {
        for (int x = 0; x < W; x++)
        {
            int coord = y * W + x;
            for (int d = 0; d < 4; d++)
            {
                int ty = y + dy[d];
                int tx = x + dx[d];
                if (ty >= 0 && ty < H && tx >= 0 && tx < W)
                {
                    int tcoord = ty * W + tx;
                    if (tiles[coord] != tiles[tcoord])
                    {
                        next_coords[coord].emplace_back(tcoord);
                    }
                }
            }
        }
    }
}

// 始点を指定して探索するクラス
class DfsSolver
{
public:
    // visiteds_[tile]はtileが踏まれたかどうかを表す
    array<bool, TILE_N> visiteds_;
    vector<int> path_;      // coordのリスト。今探索中のパス
    vector<int> best_path_; // coordのリスト。今のところ一番いいパス
    int best_score_ = 0;    // 今のところ一番いいスコア
    int score_ = 0;         // 今探索中のパスのスコア
    const double end_time = 1.9;

    // dfsを開始する。dfsは再帰で実装するので開始する関数が必要
    void start()
    {
        best_path_.clear();
        score_ = 0;
        visiteds_.fill(false);
        path_.clear();
        dfs(first_coord);
    }

    // 座標coordを始点とした探索
    void dfs(const int coord)
    {
        // 制限時間になったら終了
        if (timer.getTime() > end_time)
            return;
        path_.emplace_back(coord);      // 座標をパスに追加
        score_ += points[coord];        // 座標を踏んだことで得られるポイントをスコアに加算
        visiteds_[tiles[coord]] = true; // 座標に紐づくタイルを訪問済みとして記録

        // 今のスコアが今までの最高スコアより高い場合、最高スコアとパスを更新
        if (best_score_ < score_)
        {
            best_score_ = score_;
            best_path_ = path_;
        }

        // 未訪問で隣接している座標のリストを取得する
        vector<int> legal_next_coords;
        // next_coordsはあらかじめ計算しておいたcoordの隣の座標のリスト
        for (const auto& next_coord : next_coords[coord])
        {
            if (!visiteds_[tiles[next_coord]])
            {
                legal_next_coords.emplace_back(next_coord);
            }
        }

        // 未訪問で隣接している座標を探索
        for (const auto& next_coord : legal_next_coords)
        {
            dfs(next_coord);
        }

        // 探索が終わった座標に関する情報を巻き戻す
        path_.pop_back();                // 座標をパスから削除
        score_ -= points[coord];         // 座標を踏んだことで得られるポイントをスコアから減算
        visiteds_[tiles[coord]] = false; // 座標に紐づくタイルを訪問済みとして記録から解除
    }
};

// 解を出力する
void output(const vector<int>& now_path)
{
    // now_pathは座標のリストになっているため、移動方向に直して出力する
    string res;
    for (int i = 0; i < now_path.size() - 1; i++)
    {
        int diff = now_path[i + 1] - now_path[i];
        if (diff == 1)
        {
            res.push_back('R');
        }
        else if (diff == -1)
        {
            res.push_back('L');
        }
        else if (diff == W)
        {
            res.push_back('D');
        }
        else if (diff == -W)
        {
            res.push_back('U');
        }
    }
    cout << res << endl;
}

int main()
{
    input();
    // 前計算などの準備
    preprocess();
    DfsSolver dfs_solver{};
    dfs_solver.start();
    auto best_path = dfs_solver.best_path_;
    cerr << "visited tiles count: " << best_path.size() << endl;
    output(best_path);
    return 0;
}
