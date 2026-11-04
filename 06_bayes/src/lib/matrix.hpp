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
#pragma GCC optimize("Ofast")
#pragma once
#include <bits/stdc++.h>
using namespace std;

template <int H, int W>
struct Matrix
{
    double data_[H][W] = {};

    struct RowProxy
    {
        double* row_;
        constexpr double& operator[](int x)
        {
            assert(0 <= x);
            assert(x < W);
            return row_[x];
        }
    };

    struct ConstRowProxy
    {
        const double* row_;
        constexpr double operator[](int x) const
        {
            assert(0 <= x);
            assert(x < W);
            return row_[x];
        }
    };

    constexpr RowProxy operator[](int y)
    {
        assert(0 <= y);
        assert(y < H);
        return RowProxy{data_[y]};
    }

    constexpr ConstRowProxy operator[](int y) const
    {
        assert(0 <= y);
        assert(y < H);
        return ConstRowProxy{data_[y]};
    }

    constexpr Matrix<H, W>& operator+=(const Matrix<H, W>& a)
    {
        for (int y = 0; y < H; y++)
        {
            for (int x = 0; x < W; x++)
            {
                (*this)[y][x] += a[y][x];
            }
        }
        return *this;
    }

    constexpr Matrix<H, W>& operator-=(const Matrix<H, W>& a)
    {
        for (int y = 0; y < H; y++)
        {
            for (int x = 0; x < W; x++)
            {
                (*this)[y][x] -= a[y][x];
            }
        }
        return *this;
    }

    constexpr Matrix<H, W>& operator*=(double scalar)
    {
        for (int y = 0; y < H; y++)
        {
            for (int x = 0; x < W; x++)
            {
                (*this)[y][x] *= scalar;
            }
        }
        return *this;
    }

    friend constexpr Matrix<H, W> operator+(Matrix<H, W> a,
                                            const Matrix<H, W>& b)
    {
        return a += b;
    }

    friend constexpr Matrix<H, W> operator-(Matrix<H, W> a,
                                            const Matrix<H, W>& b)
    {
        return a -= b;
    }

    friend constexpr Matrix<H, W> operator-(const Matrix<H, W>& a)
    {
        Matrix<H, W> ret;
        for (int y = 0; y < H; y++)
        {
            for (int x = 0; x < W; x++)
            {
                ret[y][x] = -a[y][x];
            }
        }
        return ret;
    }

    friend constexpr Matrix<H, W> operator*(Matrix<H, W> a, double scalar)
    {
        return a *= scalar;
    }

    friend constexpr Matrix<H, W> operator*(double scalar, Matrix<H, W> a)
    {
        return a *= scalar;
    }

    template <int L>
    friend constexpr Matrix<H, L> operator*(const Matrix<H, W>& a,
                                            const Matrix<W, L>& b)
    {
        Matrix<H, L> ret;
        for (int y = 0; y < H; y++)
        {
            for (int x = 0; x < L; x++)
            {
                double sum = 0;
                for (int k = 0; k < W; k++)
                {
                    sum += a[y][k] * b[k][x];
                }
                ret[y][x] = sum;
            }
        }
        return ret;
    }

    constexpr Matrix<W, H> transpose() const
    {
        Matrix<W, H> ret;
        for (int y = 0; y < W; y++)
        {
            for (int x = 0; x < H; x++)
            {
                ret[y][x] = (*this)[x][y];
            }
        }
        return ret;
    }
};

double det(Matrix<2, 2> a)
{
    return a[0][0] * a[1][1] - a[0][1] * a[1][0];
}
Matrix<2, 2> inv(Matrix<2, 2> a)
{
    double d = det(a);
    return Matrix<2, 2>{
        {{a[1][1] / d, -a[0][1] / d}, {-a[1][0] / d, a[0][0] / d}}};
}
Matrix<2, 2> llt(Matrix<2, 2> a)
{
    return Matrix<2, 2>{
        {{sqrt(a[0][0]), 0},
         {a[1][0] / sqrt(a[0][0]), sqrt(a[1][1] - a[1][0] * a[1][0] / a[0][0])}}};
}

template <int N, class Rng>
Matrix<N, 1> randNormal(Rng& rng)
{
    Matrix<N, 1> ret = {};
    for (int i = 0; i < N; i++)
    {
        ret[i][0] = normal_distribution{}(rng);
    }
    return ret;
}
