// ==== LICENSE BEGIN ====
// ahc_book_samples
// https://github.com/thun-c/ahc_book_samples
//
// MIT License
//
// Copyright (c) 2026 Eita Aoki
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
// ==== LICENSE END ====
// This file contains a modified version of the beam search library by eijirou.
// https://eijirou-kyopro.hatenablog.com/entry/2024/02/01/115639
/**************************************************************/
// Euler Tour の辺を保持する差分更新ビームサーチライブラリ
// Hashを用いた同一盤面除去をする場合は
// Hash, Action, Score, State を自分で定義、実装して
// using BeamSearchUser = BeamSearch<Hash, Action, Score, StateBase>;
// Hashの処理を記述するのが面倒な場合は
// Action, Score, State を自分で定義、実装して
// using BeamSearchUser = BeamSearchNoHash<Action, Score, StateBase>;
// と記述し、
// BeamSearchUser beamSearch;
// を用いてビームサーチを実行する。
// Action以外はそれぞれのconceptに準拠している必要がある。
/**************************************************************/
#ifndef EDGE_BEAM_HPP
#define EDGE_BEAM_HPP
#include <bits/stdc++.h>
#include <atcoder/segtree>

namespace edge_beam_library
{
    using namespace std;

    // 連想配列
    // Keyにハッシュ関数を適用しない
    // open addressing with linear probing
    // unordered_mapよりも速い
    // nは格納する要素数よりも16倍ほど大きくする
    template <class Key, class T>
    struct HashMap
    {
    public:
        explicit HashMap(uint32_t n)
        {
            if (n % 2 == 0)
            {
                ++n;
            }
            n_ = n;
            valid_.resize(n_, false);
            data_.resize(n_);
        }

        // 戻り値
        // - 存在するならtrue、存在しないならfalse
        // - index
        pair<bool, int> getIndex(Key key) const
        {
            Key i = key % n_;
            while (valid_[i])
            {
                if (data_[i].first == key)
                {
                    return {true, i};
                }
                if (++i == n_)
                {
                    i = 0;
                }
            }
            return {false, i};
        }

        // 指定したindexにkeyとvalueを格納する
        void set(int i, Key key, T value)
        {
            valid_[i] = true;
            data_[i] = {key, value};
        }

        // 指定したindexのvalueを返す
        T get(int i) const
        {
            assert(valid_[i]);
            return data_[i].second;
        }

        void clear()
        {
            fill(valid_.begin(), valid_.end(), false);
        }

    private:
        uint32_t n_;
        vector<bool> valid_;
        vector<pair<Key, T>> data_;
    };

    template <typename HashType>
    concept HashConcept = requires(HashType hash) {
        { std::is_unsigned_v<HashType> };
    };
    template <typename ScoreType>
    concept ScoreConcept = requires(ScoreType score) {
        { std::is_arithmetic_v<ScoreType> };
    };

    template <typename StateType, typename HashType, typename ScoreType, typename ActionType, typename SelectorType>
    concept StateConcept = HashConcept<HashType> &&
                           ScoreConcept<ScoreType> &&
                           requires(StateType state, SelectorType selector) {
                               { state.expand(std::declval<int>(), selector) } -> same_as<void>;
                               { state.advanceForward(std::declval<ActionType>()) } -> same_as<void>;
                               { state.advanceBackward(std::declval<ActionType>()) } -> same_as<void>;
                           };

    template <HashConcept Hash, typename Action, ScoreConcept Score, template <typename> class State>
    struct EdgeBeamSearch
    {

        // ビームサーチの設定
        struct Config
        {
            int max_turn;
            size_t beam_width;
            size_t max_action_size;
            // 実行可能解が見つかったらすぐに返すかどうか
            // ビーム内のターンと問題文のターンが同じレイヤーで、
            // かつターン数最小化問題であればtrueにする。
            // そうでなければfalse
            bool return_finished_immediately;
        };

        struct Param
        {
            int max_turn;
            size_t beam_width;
            size_t tour_capacity;
            uint32_t hash_map_capacity;
            bool return_finished_immediately;
            Param(Config config) : max_turn(config.max_turn),
                                   beam_width(config.beam_width),
                                   tour_capacity((config.max_action_size + 2) * config.beam_width * 4),
                                   hash_map_capacity((config.max_action_size + 2) * config.beam_width * 16),
                                   return_finished_immediately(config.return_finished_immediately) {}
        };

        // 展開するノードの候補を表す構造体
        struct Candidate
        {
            Action action;
            Score score;
            Hash hash;
            int parent;

            Candidate(Action action, Score score, Hash hash, int parent) : action(action),
                                                                           score(score),
                                                                           hash(hash),
                                                                           parent(parent) {}
        };

        // ノードの候補から実際に追加するものを選ぶクラス
        // ビーム幅の個数だけ、評価がよいものを選ぶ
        // ハッシュ値が一致したものについては、評価がよいほうのみを残す
        class Selector
        {
        public:
            explicit Selector(const Param &param) : hash_to_index_(param.hash_map_capacity)
            {
                beam_width = param.beam_width;
                candidates_.reserve(beam_width);
                full_ = false;

                scores_.resize(beam_width);
                for (size_t i = 0; i < beam_width; ++i)
                {
                    scores_[i] = {0, i};
                }
            }

            // 候補を追加する
            // ターン数最小化型の問題で、candidateによって実行可能解が得られる場合にのみ finished = true とする
            // ビーム幅分の候補をCandidateを追加したときにsegment treeを構築する
            void push(const Action &action, const Score &score, const Hash &hash, int parent, bool finished)
            {
                Candidate candidate(action, score, hash, parent);
                if (finished)
                {
                    finished_candidates_.emplace_back(candidate);
                    return;
                }
                if (full_ && score <= st_.all_prod().first)
                {
                    // 保持しているどの候補よりも評価値が小さくないとき
                    return;
                }
                auto [valid, i] = hash_to_index_.getIndex(candidate.hash);

                if (valid)
                {
                    int j = hash_to_index_.get(i);
                    if (candidate.hash == candidates_[j].hash)
                    {
                        // ハッシュ値が等しいものが存在しているとき
                        if (full_)
                        {
                            // segment treeが構築されている場合
                            if (score > st_.get(j).first)
                            {
                                candidates_[j] = candidate;
                                st_.set(j, {score, j});
                            }
                        }
                        else
                        {
                            // segment treeが構築されていない場合
                            if (score > scores_[j].first)
                            {
                                candidates_[j] = candidate;
                                scores_[j].first = score;
                            }
                        }
                        return;
                    }
                }
                if (full_)
                {
                    // segment treeが構築されている場合
                    int j = st_.all_prod().second;
                    hash_to_index_.set(i, candidate.hash, j);
                    candidates_[j] = candidate;
                    st_.set(j, {score, j});
                }
                else
                {
                    // segment treeが構築されていない場合
                    int j = candidates_.size();
                    hash_to_index_.set(i, candidate.hash, j);
                    candidates_.emplace_back(candidate);
                    scores_[j].first = score;

                    if (candidates_.size() == beam_width)
                    {
                        // 保持している候補がビーム幅分になったときにsegment treeを構築する
                        full_ = true;
                        st_ = MaxSegtree(scores_);
                    }
                }
            }

            // 選んだ候補を返す
            const vector<Candidate> &select() const
            {
                return candidates_;
            }

            // 実行可能解が見つかったか
            bool haveFinished() const
            {
                return !finished_candidates_.empty();
            }

            // 実行可能解に到達するCandidateを返す
            vector<Candidate> getFinishedCandidates() const
            {
                return finished_candidates_;
            }

            // 最もよいCandidateを返す
            Candidate calculateBestCandidate() const
            {
                if (full_)
                {
                    size_t best = 0;
                    for (size_t i = 0; i < beam_width; ++i)
                    {
                        if (st_.get(i).first > st_.get(best).first)
                        {
                            best = i;
                        }
                    }
                    return candidates_[best];
                }
                else
                {
                    size_t best = 0;
                    for (size_t i = 0; i < candidates_.size(); ++i)
                    {
                        if (scores_[i].first > scores_[best].first)
                        {
                            best = i;
                        }
                    }
                    return candidates_[best];
                }
            }

            void clear()
            {
                candidates_.clear();
                hash_to_index_.clear();
                full_ = false;
            }

            void clearFinishedCandidates()
            {
                finished_candidates_.clear();
            }

        private:
            // 削除可能な優先度付きキュー（大きいほど良いScore用）
            using MaxSegtree = atcoder::segtree<
                pair<Score, int>,
                [](pair<Score, int> a, pair<Score, int> b)
                {
                    if (a.first <= b.first)
                    {
                        return a;
                    }
                    else
                    {
                        return b;
                    }
                },
                []()
                { return make_pair(numeric_limits<Score>::max(), -1); }>;

            size_t beam_width;
            vector<Candidate> candidates_;
            HashMap<Hash, int> hash_to_index_;
            bool full_;
            vector<pair<Score, int>> scores_;
            MaxSegtree st_;
            vector<Candidate> finished_candidates_;
        };

        // Euler Tourを管理するためのクラス
        class Tree
        {
        public:
            explicit Tree(const State<Selector> &state, const Param &param) : state_(state)
            {
                curr_tour_.reserve(param.tour_capacity);
                next_tour_.reserve(param.tour_capacity);
                leaves_.reserve(param.beam_width);
                buckets_.assign(param.beam_width, {});
            }

            // 状態を更新しながら深さ優先探索を行い、次のノードの候補を全てselectorに追加する
            void dfs(Selector &selector)
            {
                if (curr_tour_.empty())
                {
                    // 最初のターン
                    state_.expand(0, selector);
                    return;
                }

                for (auto [leaf_index, action] : curr_tour_)
                {
                    if (leaf_index >= 0)
                    {
                        // 葉
                        state_.advanceForward(action);
                        auto &[score, hash] = leaves_[leaf_index];
                        state_.expand(leaf_index, selector);
                        state_.advanceBackward(action);
                    }
                    else if (leaf_index == -1)
                    {
                        // 前進辺
                        state_.advanceForward(action);
                    }
                    else
                    {
                        // 後退辺
                        state_.advanceBackward(action);
                    }
                }
            }

            // 木を更新する
            void update(const vector<Candidate> &candidates)
            {
                leaves_.clear();

                if (curr_tour_.empty())
                {
                    // 最初のターン
                    for (const Candidate &candidate : candidates)
                    {
                        curr_tour_.push_back({(int)leaves_.size(), candidate.action});
                        leaves_.push_back({candidate.score, candidate.hash});
                    }
                    return;
                }

                for (const Candidate &candidate : candidates)
                {
                    buckets_[candidate.parent].push_back({candidate.action, candidate.score, candidate.hash});
                }

                auto it = curr_tour_.begin();

                // 一本道を反復しないようにする
                while (it->first == -1 && it->second == curr_tour_.back().second)
                {
                    Action action = (it++)->second;
                    state_.advanceForward(action);
                    direct_road_.push_back(action);
                    curr_tour_.pop_back();
                }

                // 葉の追加や不要な辺の削除をする
                while (it != curr_tour_.end())
                {
                    auto [leaf_index, action] = *(it++);
                    if (leaf_index >= 0)
                    {
                        // 葉
                        if (buckets_[leaf_index].empty())
                        {
                            continue;
                        }
                        next_tour_.push_back({-1, action});
                        for (auto [new_action, score, hash] : buckets_[leaf_index])
                        {
                            int new_leaf_index = leaves_.size();
                            next_tour_.push_back({new_leaf_index, new_action});
                            leaves_.push_back({score, hash});
                        }
                        buckets_[leaf_index].clear();
                        next_tour_.push_back({-2, action});
                    }
                    else if (leaf_index == -1)
                    {
                        // 前進辺
                        next_tour_.push_back({-1, action});
                    }
                    else
                    {
                        // 後退辺
                        auto [old_leaf_index, old_action] = next_tour_.back();
                        if (old_leaf_index == -1)
                        {
                            next_tour_.pop_back();
                        }
                        else
                        {
                            next_tour_.push_back({-2, action});
                        }
                    }
                }
                swap(curr_tour_, next_tour_);
                next_tour_.clear();
            }

            // 根からのパスを取得する
            vector<Action> calculatePath(int parent, int turn) const
            {
                // cerr << curr_tour_.size() << endl;

                vector<Action> ret = direct_road_;
                ret.reserve(turn);
                for (auto [leaf_index, action] : curr_tour_)
                {
                    if (leaf_index >= 0)
                    {
                        if (leaf_index == parent)
                        {
                            ret.push_back(action);
                            return ret;
                        }
                    }
                    else if (leaf_index == -1)
                    {
                        ret.push_back(action);
                    }
                    else
                    {
                        ret.pop_back();
                    }
                }

                assert(false);
                return {};
            }

        private:
            State<Selector> state_;
            vector<pair<int, Action>> curr_tour_;
            vector<pair<int, Action>> next_tour_;
            vector<pair<Score, Hash>> leaves_;
            vector<vector<tuple<Action, Score, Hash>>> buckets_;
            vector<Action> direct_road_;
        };

        // ビームサーチを行う関数
        vector<Action> beamSearch(const Config &config, const State<Selector> &state)
        {
            Param param(config);
            Tree tree(state, param);

            // 新しいノード候補の集合
            Selector selector(param);

            // param.return_finished_immediately が false のときに、
            // 実行可能解の中で一番よいものを覚えておくための変数
            // ビームサーチ内で扱うturnと問題のturnが一致しないときに使う
            Score best_score = numeric_limits<Score>::min();
            vector<Action> best_ret;
            for (int turn = 0; turn < config.max_turn; ++turn)
            {
                // Euler Tourでselectorに候補を追加する
                tree.dfs(selector);

                if (selector.haveFinished())
                {
                    // ターン数最小化型の問題で実行可能解が見つかったとき
                    if (config.return_finished_immediately)
                    {
                        Candidate candidate = selector.getFinishedCandidates()[0];
                        vector<Action> ret = tree.calculatePath(candidate.parent, turn + 1);
                        ret.push_back(candidate.action);
                        return ret;
                    }
                    else
                    {
                        for (auto candidate : selector.getFinishedCandidates())
                        {
                            vector<Action> ret = tree.calculatePath(candidate.parent, turn + 1);
                            ret.push_back(candidate.action);
                            if (candidate.score > best_score)
                            {
                                best_score = candidate.score;
                                best_ret = ret;
                            }
                        }
                    }
                    selector.clearFinishedCandidates();
                }
                if (selector.select().empty())
                {
                    return best_ret;
                }
                assert(!selector.select().empty());

                if (turn == config.max_turn - 1)
                {
                    assert(selector.select().empty());
                    // ターン数固定型の問題で全ターンが終了したとき
                    Candidate best_candidate = selector.calculateBestCandidate();
                    vector<Action> ret = tree.calculatePath(best_candidate.parent, turn + 1);
                    ret.push_back(best_candidate.action);
                    return ret;
                }

                // 木を更新する
                tree.update(selector.select());

                selector.clear();
            }

            assert(false);
            return {};
        }

        // StateConcept のチェックを構造体内で実施
        static_assert(StateConcept<State<Selector>, Hash, Score, Action, Selector>,
                      "State template must satisfy StateConcept with BeamSearch::Selector");

    }; // EdgeBeamSearch

    template <typename StateType, typename ScoreType, typename ActionType, typename SelectorType>
    concept StateConceptNoHash =
        ScoreConcept<ScoreType> &&
        requires(StateType state, SelectorType selector) {
            { state.expand(std::declval<int>(), selector) } -> same_as<void>;
            { state.advanceForward(std::declval<ActionType>()) } -> same_as<void>;
            { state.advanceBackward(std::declval<ActionType>()) } -> same_as<void>;
        };

    template <typename Action, ScoreConcept Score, template <typename> class State>
    struct EdgeBeamSearchNoHash
    {
        // ビームサーチの設定
        struct Config
        {
            int max_turn;
            size_t beam_width;
            size_t max_action_size;
            // 実行可能解が見つかったらすぐに返すかどうか
            // ビーム内のターンと問題文のターンが同じレイヤーで、
            // かつターン数最小化問題であればtrueにする。
            // そうでなければfalse
            bool return_finished_immediately;
        };

        struct Param
        {
            int max_turn;
            size_t beam_width;
            size_t tour_capacity;
            bool return_finished_immediately;
            Param(Config config) : max_turn(config.max_turn),
                                   beam_width(config.beam_width),
                                   tour_capacity((config.max_action_size + 2) * config.beam_width * 4),
                                   return_finished_immediately(config.return_finished_immediately) {}
        };

        // 展開するノードの候補を表す構造体
        struct Candidate
        {
            Action action;
            Score score;
            int parent;

            Candidate(Action action, Score score, int parent) : action(action),
                                                                score(score),
                                                                parent(parent) {}
        };

        // ノードの候補から実際に追加するものを選ぶクラス
        // ビーム幅の個数だけ、評価がよいものを選ぶ
        // ハッシュ値が一致したものについては、評価がよいほうのみを残す
        class Selector
        {
        public:
            explicit Selector(const Param &param)
            {
                beam_width = param.beam_width;
                candidates_.reserve(beam_width);
                full_ = false;

                scores_.resize(beam_width);
                for (size_t i = 0; i < beam_width; ++i)
                {
                    scores_[i] = {0, i};
                }
            }

            // 候補を追加する
            // ターン数最小化型の問題で、candidateによって実行可能解が得られる場合にのみ finished = true とする
            // ビーム幅分の候補をCandidateを追加したときにsegment treeを構築する
            void push(const Action &action, const Score &score, int parent, bool finished)
            {
                Candidate candidate(action, score, parent);
                if (finished)
                {
                    finished_candidates_.emplace_back(candidate);
                    return;
                }
                if (full_ && score <= st_.all_prod().first)
                {
                    // 保持しているどの候補よりもScoreが大きくないとき
                    return;
                }
                if (full_)
                {
                    // segment treeが構築されている場合
                    int j = st_.all_prod().second;
                    candidates_[j] = candidate;
                    st_.set(j, {score, j});
                }
                else
                {
                    // segment treeが構築されていない場合
                    int j = candidates_.size();
                    candidates_.emplace_back(candidate);
                    scores_[j].first = score;

                    if (candidates_.size() == beam_width)
                    {
                        // 保持している候補がビーム幅分になったときにsegment treeを構築する
                        full_ = true;
                        st_ = MaxSegtree(scores_);
                    }
                }
            }

            // 選んだ候補を返す
            const vector<Candidate> &select() const
            {
                return candidates_;
            }

            // 実行可能解が見つかったか
            bool haveFinished() const
            {
                return !finished_candidates_.empty();
            }

            // 実行可能解に到達するCandidateを返す
            vector<Candidate> getFinishedCandidates() const
            {
                return finished_candidates_;
            }

            // 最もよいCandidateを返す
            Candidate calculateBestCandidate() const
            {
                if (full_)
                {
                    size_t best = 0;
                    for (size_t i = 0; i < beam_width; ++i)
                    {
                        if (st_.get(i).first > st_.get(best).first)
                        {
                            best = i;
                        }
                    }
                    return candidates_[best];
                }
                else
                {
                    size_t best = 0;
                    for (size_t i = 0; i < candidates_.size(); ++i)
                    {
                        if (scores_[i].first > scores_[best].first)
                        {
                            best = i;
                        }
                    }
                    return candidates_[best];
                }
            }

            void clear()
            {
                candidates_.clear();
                full_ = false;
            }

            void clearFinishedCandidates()
            {
                finished_candidates_.clear();
            }

        private:
            // 削除可能な優先度付きキュー
            using MaxSegtree = atcoder::segtree<
                pair<Score, int>,
                [](pair<Score, int> a, pair<Score, int> b)
                {
                    if (a.first <= b.first)
                    {
                        return a;
                    }
                    else
                    {
                        return b;
                    }
                },
                []()
                { return make_pair(numeric_limits<Score>::max(), -1); }>;

            size_t beam_width;
            vector<Candidate> candidates_;
            bool full_;
            vector<pair<Score, int>> scores_;
            MaxSegtree st_;
            vector<Candidate> finished_candidates_;
        };

        // Euler Tourを管理するためのクラス
        class Tree
        {
        public:
            explicit Tree(const State<Selector> &state, const Param &param) : state_(state)
            {
                curr_tour_.reserve(param.tour_capacity);
                next_tour_.reserve(param.tour_capacity);
                leaves_.reserve(param.beam_width);
                buckets_.assign(param.beam_width, {});
            }

            // 状態を更新しながら深さ優先探索を行い、次のノードの候補を全てselectorに追加する
            void dfs(Selector &selector)
            {
                if (curr_tour_.empty())
                {
                    // 最初のターン
                    state_.expand(0, selector);
                    return;
                }

                for (auto [leaf_index, action] : curr_tour_)
                {
                    if (leaf_index >= 0)
                    {
                        // 葉
                        state_.advanceForward(action);
                        auto score = leaves_[leaf_index];
                        state_.expand(leaf_index, selector);
                        state_.advanceBackward(action);
                    }
                    else if (leaf_index == -1)
                    {
                        // 前進辺
                        state_.advanceForward(action);
                    }
                    else
                    {
                        // 後退辺
                        state_.advanceBackward(action);
                    }
                }
            }

            // 木を更新する
            void update(const vector<Candidate> &candidates)
            {
                leaves_.clear();

                if (curr_tour_.empty())
                {
                    // 最初のターン
                    for (const Candidate &candidate : candidates)
                    {
                        curr_tour_.push_back({(int)leaves_.size(), candidate.action});
                        leaves_.push_back(candidate.score);
                    }
                    return;
                }

                for (const Candidate &candidate : candidates)
                {
                    buckets_[candidate.parent].push_back({candidate.action, candidate.score});
                }

                auto it = curr_tour_.begin();

                // 一本道を反復しないようにする
                while (it->first == -1 && it->second == curr_tour_.back().second)
                {
                    Action action = (it++)->second;
                    state_.advanceForward(action);
                    direct_road_.push_back(action);
                    curr_tour_.pop_back();
                }

                // 葉の追加や不要な辺の削除をする
                while (it != curr_tour_.end())
                {
                    auto [leaf_index, action] = *(it++);
                    if (leaf_index >= 0)
                    {
                        // 葉
                        if (buckets_[leaf_index].empty())
                        {
                            continue;
                        }
                        next_tour_.push_back({-1, action});
                        for (auto [new_action, score] : buckets_[leaf_index])
                        {
                            int new_leaf_index = leaves_.size();
                            next_tour_.push_back({new_leaf_index, new_action});
                            leaves_.push_back(score);
                        }
                        buckets_[leaf_index].clear();
                        next_tour_.push_back({-2, action});
                    }
                    else if (leaf_index == -1)
                    {
                        // 前進辺
                        next_tour_.push_back({-1, action});
                    }
                    else
                    {
                        // 後退辺
                        auto [old_leaf_index, old_action] = next_tour_.back();
                        if (old_leaf_index == -1)
                        {
                            next_tour_.pop_back();
                        }
                        else
                        {
                            next_tour_.push_back({-2, action});
                        }
                    }
                }
                swap(curr_tour_, next_tour_);
                next_tour_.clear();
            }

            // 根からのパスを取得する
            vector<Action> calculatePath(int parent, int turn) const
            {
                // cerr << curr_tour_.size() << endl;

                vector<Action> ret = direct_road_;
                ret.reserve(turn);
                for (auto [leaf_index, action] : curr_tour_)
                {
                    if (leaf_index >= 0)
                    {
                        if (leaf_index == parent)
                        {
                            ret.push_back(action);
                            return ret;
                        }
                    }
                    else if (leaf_index == -1)
                    {
                        ret.push_back(action);
                    }
                    else
                    {
                        ret.pop_back();
                    }
                }

                assert(false);
                return {};
            }

        private:
            State<Selector> state_;
            vector<pair<int, Action>> curr_tour_;
            vector<pair<int, Action>> next_tour_;
            vector<Score> leaves_;
            vector<vector<tuple<Action, Score>>> buckets_;
            vector<Action> direct_road_;
        };

        // ビームサーチを行う関数
        vector<Action> beamSearch(const Config &config, const State<Selector> &state)
        {
            Param param(config);
            Tree tree(state, config);

            // 新しいノード候補の集合
            Selector selector(config);

            // config.return_finished_immediately が false のときに、
            // 実行可能解の中で一番よいものを覚えておくための変数
            // ビームサーチ内で扱うturnと問題のturnが一致しないときに使う
            Score best_score = numeric_limits<Score>::min();
            vector<Action> best_ret;
            for (int turn = 0; turn < config.max_turn; ++turn)
            {
                // Euler Tourでselectorに候補を追加する
                tree.dfs(selector);

                if (selector.haveFinished())
                {
                    // ターン数最小化型の問題で実行可能解が見つかったとき
                    if (config.return_finished_immediately)
                    {
                        Candidate candidate = selector.getFinishedCandidates()[0];
                        vector<Action> ret = tree.calculatePath(candidate.parent, turn + 1);
                        ret.push_back(candidate.action);
                        return ret;
                    }
                    else
                    {
                        for (auto candidate : selector.getFinishedCandidates())
                        {
                            vector<Action> ret = tree.calculatePath(candidate.parent, turn + 1);
                            ret.push_back(candidate.action);
                            if (candidate.score > best_score)
                            {
                                best_score = candidate.score;
                                best_ret = ret;
                            }
                        }
                    }
                    selector.clearFinishedCandidates();
                }
                if (selector.select().empty())
                {
                    return best_ret;
                }
                assert(!selector.select().empty());

                if (turn == config.max_turn - 1)
                {
                    // ターン数固定型の問題で全ターンが終了したとき
                    Candidate best_candidate = selector.calculateBestCandidate();
                    vector<Action> ret = tree.calculatePath(best_candidate.parent, turn + 1);
                    ret.push_back(best_candidate.action);
                    return ret;
                }

                // 木を更新する
                tree.update(selector.select());

                selector.clear();
            }

            assert(false);
            return {};
        }

        // StateConcept のチェックを構造体内で実施
        static_assert(StateConceptNoHash<State<Selector>, Score, Action, Selector>,
                      "State template must satisfy StateConcept with BeamSearch::Selector");

    }; // EdgeBeamSearchNoHash

} // namespace beamSearch
using namespace edge_beam_library;
#endif