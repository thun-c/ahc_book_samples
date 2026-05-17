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
mt19937 rng(0);
uniform_real_distribution<double> di_double(0.0, 1.0);

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

// startからendまでの範囲でprogressに応じ、最初に急激に減少、その後緩やかに減少した値をとる
double exponentialDecrease(double start, double end, double progress)
{
    return pow(start, 1.0 - progress) * pow(end, progress);
}

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
    vector<int> path_;         // coordのリスト。今探索中のパス
    vector<int> best_path_;    // coordのリスト。今のところ一番いいパス
    int best_score_ = 0;       // 今のところ一番いいスコア
    int score_ = 0;            // 今探索中のパスのスコア
    int remaining_search_cnt_; // 残り探索回数

    // dfsを開始する。dfsは再帰で実装するので開始する関数が必要
    void start()
    {
        best_path_.clear();
        score_ = 0;
        visiteds_.fill(false);
        path_.clear();
        remaining_search_cnt_ = 4000;
        dfs(first_coord);
    }

    // 座標coordを始点とした探索
    void dfs(const int coord)
    {
        if (remaining_search_cnt_ <= 0)
            return;
        remaining_search_cnt_--;
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
            if (!visiteds_[tiles[next_coord]])
            {
                dfs(next_coord);
            }
        }

        // 探索が終わった座標に関する情報を巻き戻す
        path_.pop_back();                // 座標をパスから削除
        score_ -= points[coord];         // 座標を踏んだことで得られるポイントをスコアから減算
        visiteds_[tiles[coord]] = false; // 座標に紐づくタイルを訪問済みとして記録から解除
    }
};

// 始点と終点を指定して問題の一部を探索するクラス
// 近傍計算に使う
class DfsPartSolver
{
public:
    // visiteds_[tile]はtileが踏まれたかどうかを表す
    array<bool, TILE_N> visiteds_;
    vector<int> path_;         // coordのリスト。今探索中のパス
    vector<int> best_path_;    // coordのリスト。今のところ一番いいパス
    int best_score_ = 0;       // 今のところ一番いいスコア
    int score_ = 0;            // 今探索中のパスのスコア
    int remaining_search_cnt_; // 残り探索回数
    int goal_ = 0;             // 目標地点

    // dfsを開始する。dfsは再帰で実装するので開始する関数が必要
    // DfsSolverのstartとは以下の点で異なる
    //  始点、終点を指定する
    //  注目しない部分はタイルが踏まれているものとして初期化する
    void start(const int start, const int goal,
               const array<bool, TILE_N>& visiteds,
               const int remaining_search_cnt)
    {
        goal_ = goal;
        best_path_.clear();
        best_score_ = 0;
        score_ = 0;
        visiteds_ = visiteds;
        path_.clear();
        remaining_search_cnt_ = remaining_search_cnt;
        dfs(start, true);
    }

    // coordを始点としてgoal_を終点とした探索
    void dfs(const int coord, const bool is_start = false)
    {
        if (remaining_search_cnt_ <= 0)
            return;
        remaining_search_cnt_--;
        // 始点は追加経路に含めない
        if (!is_start)
        {
            path_.emplace_back(coord);
            score_ += points[coord];
            visiteds_[tiles[coord]] = true;
        }
        if (coord != goal_)
        {
            vector<int> legal_next_coords;
            for (const auto& next_coord : next_coords[coord])
            {
                if (!visiteds_[tiles[next_coord]])
                {
                    legal_next_coords.emplace_back(next_coord);
                }
                else if (next_coord == goal_)
                {
                    // goalにたどり着いた時点で探索を終了
                    best_score_ = score_;
                    best_path_ = path_;
                    remaining_search_cnt_ = 0;
                    return;
                }
            }
            shuffle(legal_next_coords.begin(), legal_next_coords.end(), rng);
            for (const auto& next_coord : legal_next_coords)
            {
                if (!visiteds_[tiles[next_coord]])
                {
                    dfs(next_coord);
                    if (remaining_search_cnt_ == 0)
                        return;
                }
            }
        }

        // 始点はパスに積んでいないので、始点以外のみ巻き戻す
        if (!is_start)
        {
            path_.pop_back();                // 座標をパスから削除
            score_ -= points[coord];         // 座標を踏んだことで得られるポイントをスコアから減算
            visiteds_[tiles[coord]] = false; // 座標に紐づくタイルを訪問済みとして記録から解除
        }
    }
};

class LocalSearchSolver
{
public:
    vector<int> best_path_; // coordのリスト。今までに遷移してきた現在のパス

    LocalSearchSolver(const vector<int>& now_path)
        : best_path_(now_path)
    {
    }

    // now_pathのstart_path_idからend_path_idの間にupdate_pathで置き換えたパスを生成
    vector<int> getInsertedPath(
        const vector<int>& now_path,
        const int start_path_id,
        const int end_path_id,
        const vector<int>& update_path)
    {
        // 今回生成した部分パスを挿入した全体パスを生成
        vector<int> inserted_path;
        for (int i = 0; i <= start_path_id; i++)
        {
            inserted_path.emplace_back(now_path[i]);
        }

        for (int i = 0; i < update_path.size(); i++)
        {
            inserted_path.emplace_back(update_path[i]);
        }

        for (int i = end_path_id; i < now_path.size(); i++)
        {
            inserted_path.emplace_back(now_path[i]);
        }
        return inserted_path;
    }

    void simulatedAnnealing()
    {
        double start_temperature = 500;
        double end_temperature = 10;
        double end_time = 1.9;

        vector<int> now_path_ = best_path_;
        array<bool, TILE_N> now_visiteds{};
        DfsPartSolver dfs_part_solver{};
        int now_score_total = 0;
        for (int i = 0; i < now_path_.size(); i++)
        {
            now_visiteds[tiles[now_path_[i]]] = true;
            now_score_total += points[now_path_[i]];
        }
        int best_score = now_score_total;

        while (true)
        {
            double elapsed_time = timer.getTime();
            if (elapsed_time > end_time)
            {
                break;
            }

            double temperature = exponentialDecrease(
                start_temperature, end_temperature, elapsed_time / end_time);

            int min_delete_path_length = 2;
            int max_delete_path_length = max(
                (int)(now_path_.size() * 0.05),
                min_delete_path_length);
            uniform_int_distribution<int> di_delete(
                min_delete_path_length,
                max_delete_path_length);
            int delete_path_length = di_delete(rng);
            uniform_int_distribution<int> di_start(
                0, (int)(now_path_.size() - 1 - delete_path_length));
            int start_path_id = di_start(rng);
            int end_path_id = start_path_id + delete_path_length;

            auto next_visiteds = now_visiteds;
            // 探索するパスの長さよりちょっと多いぐらいの探索をする
            int remaining_search_cnt = 4 * delete_path_length;

            // start_path_idとend_path_idの間のパスを削り、
            // 削った部分のスコアをメモする
            int now_score = 0;
            for (int i = start_path_id + 1; i <= end_path_id - 1; i++)
            {
                now_score += points[now_path_[i]];
                next_visiteds[tiles[now_path_[i]]] = false;
            }

            dfs_part_solver.start(now_path_[start_path_id], now_path_[end_path_id],
                                  next_visiteds, remaining_search_cnt);
            int next_score = dfs_part_solver.best_score_;
            int diff = next_score - now_score;
            if (dfs_part_solver.best_path_.size() > 0 && exp(diff / temperature) > di_double(rng))
            {
                // visitedsの更新
                now_visiteds = dfs_part_solver.visiteds_;

                now_path_ = getInsertedPath(now_path_, start_path_id, end_path_id, dfs_part_solver.best_path_);
                now_score_total += diff;
                if (best_score < now_score_total)
                {
                    best_score = now_score_total;
                    best_path_ = now_path_;
                }
            }
        }
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
    auto now_path_ = dfs_solver.best_path_;
    LocalSearchSolver solver(now_path_);
    solver.simulatedAnnealing();
    auto best_path = solver.best_path_;
    cerr << "visited tiles count: " << best_path.size() << endl;
    output(best_path);
    return 0;
}
