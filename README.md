# 「はじめてのAtCoder Heuristic Contest」サンプルコード

## ディレクトリ構成

```bash
.
├── [??_chapter]
│   └── src
│       ├── *.cpp
│       └── lib 一部の章のみ
│           └── *.hpp
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