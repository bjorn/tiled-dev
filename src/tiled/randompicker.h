/*
 * randompicker.h
 * Copyright 2015, Thorbjørn Lindeijer <thorbjorn@lindeijer.nl>
 *
 * This file is part of Tiled.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program. If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <QMap>
#include <QVector>

#include <random>

namespace Tiled {

inline std::default_random_engine &globalRandomEngine()
{
    static std::default_random_engine engine(std::random_device{}());
    return engine;
}

/**
 * A class that helps pick random things that each have a probability
 * assigned.
 */
template<typename T, typename Real = qreal>
class RandomPicker
{
public:
    RandomPicker()
        : mSum(0.0)
    {}

    void add(const T &value, Real probability = 1.0)
    {
        if (probability > 0) {
            mSum += probability;
            mThresholds.insert(mSum, value);
        }
    }

    bool isEmpty() const
    {
        return mThresholds.isEmpty();
    }

    const T &pick() const
    {
        Q_ASSERT(!isEmpty());

        if (mThresholds.size() == 1)
            return mThresholds.first();

        std::uniform_real_distribution<Real> dis(0, mSum);
        const Real random = dis(globalRandomEngine());
        auto it = mThresholds.lowerBound(random);
        if (it == mThresholds.end())
            --it;

        return it.value();
    }

    void clear()
    {
        mSum = 0.0;
        mThresholds.clear();
    }

private:
    Real mSum;
    QMap<Real, T> mThresholds;
};

/**
 * A class that helps take random things that each have a probability
 * assigned. Each added value can only be taken once.
 */
template<typename T, typename Real = qreal>
class RandomTaker
{
public:
    RandomTaker()
        : mSum(0.0)
    {}

    void add(const T &value, Real probability = 1.0)
    {
        if (probability > 0) {
            mSum += probability;
            mEntries.append({ value, probability });
        }
    }

    bool isEmpty() const
    {
        return mEntries.isEmpty();
    }

    T take()
    {
        Q_ASSERT(!isEmpty());

        std::uniform_real_distribution<Real> dis(0, mSum);
        const Real threshold = dis(globalRandomEngine());

        Real sum = 0.0;
        auto i = mEntries.size() - 1;

        for (; i > 0; --i) {
            sum += mEntries[i].second;
            if (sum > threshold)
                break;
        }

        // Make sure chosen entry is the last one
        if (i != mEntries.size() - 1)
            std::swap(mEntries[i], mEntries.last());

        auto entry = mEntries.takeLast();
        mSum -= entry.second;
        return entry.first;
    }

    void clear()
    {
        mSum = 0.0;
        mEntries.clear();
    }

private:
    Real mSum;
    QVector<QPair<T, Real>> mEntries;
};

} // namespace Tiled
