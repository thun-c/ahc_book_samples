#pragma GCC optimize("Ofast")
#include <bits/stdc++.h>

using namespace std;

constexpr int STRAWBERRY = 1;
constexpr int WATERMELON = 2;
constexpr int PUMPKIN = 3;
constexpr int T = 100;
constexpr int BACKWARD = 0;
constexpr int RIGHT = 1;
constexpr int FORWARD = 2;
constexpr int LEFT = 3;
constexpr char dirs[4] = {'B', 'R', 'F', 'L'};

array<int, T> f; // 各ターンで追加されるキャンディの種類

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
    cout << dirs[action] << endl;
}

// ターンごとの固定ルールで方向を決めて出力する
void ruleBase(void)
{
    for (int t = 0; t < T; ++t)
    {
        int p;
        cin >> p;
        int dir;
        if (t == T - 1)
        {
            // 最後のターンはnext_candyが存在しない
            // かつ、どの方向に傾けても同じ結果
            dir = FORWARD;
        }
        else
        {
            int candy = f[t];
            int next_candy = f[t + 1];
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
        output(dir);
    }
}

int main()
{
    input();
    ruleBase();
    return 0;
}
