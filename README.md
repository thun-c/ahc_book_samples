# 「AHC入門 ～見て、考えて、改善するAtCoder Heuristic Contest」サンプルコード

## ディレクトリ構成

```bash
.
├── [??_chapter]
│   └── src
│       ├── *.cpp
│       └── lib 一部の章のみ
│           └── *.hpp
├── LICENSE
└── README.md
```

## 使い方

各章の問題文ページからローカルテスタのzipファイルをダウンロードし、解凍して以下のようにtoolsディレクトリを配置してください。

```diff
 .
 ├── [??_chapter]
 │   ├── src
+│   └── tools
 └── README.md
```

章のディレクトリに移動し、実験したいコードをコンパイルします。
例えば、2章の最初のサンプルコードをコンパイルする場合は以下のコマンドを実行します。

```bash
cd 02_intro/
g++ -std=gnu++23 -O2 src/01_greedy.cpp
```

すると、`a.out`という実行ファイルができます。

```diff
 .
 ├── 02_intro
 │   ├── src
 │   ├── tools
+│   └── a.out
 └── README.md
```

多くの章では、以下のようなコマンドを実行して`tools/out/`配下に出力結果を保存します。
ここでは、テストケース0を入力として、コードに紐づいた名前`01_greedy.txt`で出力ファイルを保存します。

```bash
mkdir -p tools/out
./a.out < tools/in/0000.txt > tools/out/01_greedy.txt
```

多くの章では、以下のようなコマンドを実行し、公式ツールでスコアを評価します。
`vis`の後ろに入力ファイル、出力ファイルの順で指定します。
ここでは、テストケース0を入力、`01_greedy.txt`を出力として指定しています。
必ず出力ファイルを作成した時に使った入力ファイルを指定するようにしてください。

```bash: 評価
cd tools
cargo run --release --bin vis in/0000.txt out/01_greedy.txt
```

3章の一部コードではAtCoder Library(ACL)を利用します。
リポジトリ直下で以下のコマンドを打ち、ダウンロードしてください。

```bash
git clone https://github.com/atcoder/ac-library.git
```

以下のように、リポジトリ直下に`ac-library`というディレクトリができます。

```diff
 .
 ├── [??_chapter]
 ├── README.md
+└── ac-library
```

コンパイル時は、以下のように`-I../ac-library`を指定し、ライブラリを認識できるようにしてください。

```bash
cd 03_koutiku/
g++ -std=gnu++23 -I../ac-library -O2 src/07_euler_beam.cpp
```

6章ではツールの形式が異なる点と、コードの使い方が異なる点から、本文を参照してください。

## ライセンス

本リポジトリのコードは[MITライセンス](LICENSE)で公開しています。

3章の差分更新ビームサーチライブラリ（`03_koutiku/src/lib/edge_beam.hpp`、および同ライブラリを含む`03_koutiku/src/07_euler_beam.cpp`〜`09_nohash.cpp`）は、eijirou氏の[差分更新ビームサーチライブラリ](https://eijirou-kyopro.hatenablog.com/entry/2024/02/01/115639)を、同氏の許諾を得て改変したものです。

### コンテストに提出する場合

各ソースファイルの先頭には、著作権表示とライセンス文を記載しています（`// ==== LICENSE BEGIN ====`から`// ==== LICENSE END ====`まで）。
本リポジトリのコードを含めて提出する場合は、この部分を消さずに提出してください。
一部の関数などを切り出して使う場合も、この部分を一緒に貼り付けてください。

本リポジトリのコードには、生成AIを用いて作成した部分が含まれます。
本リポジトリ公開時点の[短期AtCoder Heuristic Contest 生成AI利用ルール（20260818版）](https://info.atcoder.jp/entry/short-ahc-llm-rules-ja)では、コンテスト開始前に生成AIを用いて作成したコードを使う場合、あらかじめコードを公開した上で、提出コードの該当部分にそのURLを明記することが求められています。
この部分にはリポジトリのURLが含まれているため、消さずに提出すれば、MITライセンスの条件とあわせてこのルールも満たせます。

ただし、AtCoderのルールは改定されることがあり、上記がお読みいただいている時点で最新とは限りません。
参加するコンテストのルールページなどから、最新のルールをご自身で確認してください。

6章の`combiner.sh`で複数ファイルを1ファイルに統合した場合は、統合後のファイルの先頭に1回だけ記載されます。
