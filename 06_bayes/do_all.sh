#!/usr/bin/env bash
# 不測の事態に安全にスクリプトを終了させる設定群
set -euo pipefail

# コマンドライン引数で与えたコードをコンパイルして実行ファイルa.outを作成
g++ -std=gnu++23 -O2 -o "a.out" "src/$1" -Isrc

# 公式ツールのディレクトリに移動
cd "tools"

total_score=0
case_count=0

for input_file in "in/"*; do
    echo "running $input_file ..."
    # テスターの標準エラー出力だけをtester_logに格納
    tester_log=$(cargo run --release --bin tester "$input_file" "../a.out" 2>&1 1>/dev/null)
    # テスターの標準エラー出力からScore = 1234のような行の数字だけを抽出
    score=$(printf '%s\n' "$tester_log" | sed -n 's/^Score = //p')
    # scoreをtotal_scoreに加算
    total_score=$((total_score + score))
    # case_countをインクリメント
    case_count=$((case_count + 1))
done

# スコアの統計情報を表示
average_score=$((total_score / case_count))
echo "Total score: $total_score"
echo "Average score: $average_score"
echo "Case count: $case_count"
