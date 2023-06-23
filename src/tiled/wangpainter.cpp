/*
 * wangpainter.cpp
 * Copyright 2023, a-morphous
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

#include "wangpainter.h"
#include "grid.h"
#include "geometry.h"
#include "wangfiller.h"
#include "hexagonalrenderer.h"

namespace Tiled {

static constexpr QPoint aroundTilePoints[WangId::NumIndexes] = {
    QPoint( 0, -1),
    QPoint( 1, -1),
    QPoint( 1,  0),
    QPoint( 1,  1),
    QPoint( 0,  1),
    QPoint(-1,  1),
    QPoint(-1,  0),
    QPoint(-1, -1)
};

//  3 0
//  2 1
static constexpr QPoint aroundVertexPoints[WangId::NumCorners] = {
    QPoint( 0, -1),
    QPoint( 0,  0),
    QPoint(-1,  0),
    QPoint(-1, -1)
};

WangPainter::WangPainter()
{}

WangPainter::~WangPainter()
{}

WangPainter::BrushMode WangPainter::brushMode()
{
    return mBrushMode;
}

void WangPainter::setWangSet(const WangSet *wangSet)
{
    if (wangSet == mWangSet)
        return;

    mCurrentColor = 0;
    mWangSet = wangSet;

    if (mWangSet) {
        switch (mWangSet->type()) {
        case WangSet::Corner:
            mBrushMode = BrushMode::PaintCorner;
            break;
        case WangSet::Edge:
            mBrushMode = BrushMode::PaintEdge;
            break;
        case WangSet::Mixed:
            mBrushMode = BrushMode::PaintEdgeAndCorner;
            break;
        }
    } else {
        mBrushMode = BrushMode::Idle;
    }
}

void WangPainter::setColor(int color)
{
    if (color == mCurrentColor)
        return;

    mCurrentColor = color;

    if (!mWangSet)
        return;

    switch (mWangSet->type()) {
    case WangSet::Corner:
        mBrushMode = BrushMode::PaintCorner;
        break;
    case WangSet::Edge:
        mBrushMode = BrushMode::PaintEdge;
        break;
    case WangSet::Mixed:
    {
        // Determine a meaningful mode by looking at where the color is used.
        bool usedAsCorner = false;
        bool usedAsEdge = false;

        if (mWangSet && color > 0 && color <= mWangSet->colorCount()) {
            for (const WangId wangId : mWangSet->wangIdByTileId()) {
                for (int i = 0; i < WangId::NumIndexes; ++i) {
                    if (wangId.indexColor(i) == color) {
                        const bool isCorner = WangId::isCorner(i);
                        usedAsCorner |= isCorner;
                        usedAsEdge |= !isCorner;
                    }
                }
            }
        }

        if (usedAsEdge == usedAsCorner)
            mBrushMode = BrushMode::PaintEdgeAndCorner;
        else if (usedAsEdge)
            mBrushMode = BrushMode::PaintEdge;
        else
            mBrushMode = BrushMode::PaintCorner;

        break;
    }
    }
}

WangId::Index WangPainter::getDesiredDirection(WangId::Index initialDirection)
{
    switch (mBrushMode) {
    case BrushMode::Idle:
        break;
    case BrushMode::PaintCorner:
        // override this because it should always be topLeft for PaintCorner.
        return WangId::TopLeft;
    case BrushMode::PaintEdge:
        // no corners, so we have to set these to cardinal coordinates.
        switch (initialDirection) {
        case WangId::BottomRight:
            return WangId::Bottom;
        case WangId::BottomLeft:
            return WangId::Left;
        case WangId::TopLeft:
            return WangId::Top;
        case WangId::TopRight:
            return WangId::Right;
        default:
            break;
        }
        break;

    case BrushMode::PaintEdgeAndCorner:
        switch (initialDirection) {
        case WangId::BottomRight:
            return WangId::TopLeft;
        case WangId::BottomLeft:
            return WangId::TopLeft;
        case WangId::TopRight:
            return WangId::TopLeft;
        default:
            break;
        }
        break;
    }

    return initialDirection;
}

void WangPainter::setTerrain(WangFiller::FillRegion &fill, MapDocument *mapDocument, int color, QPoint pos, WangId::Index directionToGenerate, bool useTileMode)
{
    setColor(color);
    WangId::Index direction = getDesiredDirection(directionToGenerate);
    generateTerrainAt(mapDocument, fill, color, pos, direction, useTileMode);
}

void WangPainter::setTerrain(MapDocument *mapDocument, int color, QPoint pos, WangId::Index directionToGenerate, bool useTileMode)
{
    setTerrain(mCurrentFill, mapDocument, color, pos, directionToGenerate, useTileMode);
}

void WangPainter::setTerrain(int color, QPoint pos, WangId::Index index)
{
    Grid<WangFiller::CellInfo> &grid = mCurrentFill.grid;
    QRegion &region = mCurrentFill.region;

    // Set the requested color at the given index
    WangFiller::CellInfo adjacent = grid.get(pos);
    adjacent.desired.setIndexColor(index, color);
    adjacent.mask.setIndexColor(index, WangId::INDEX_MASK);
    grid.set(p, adjacent);

    // Adjust connected indexes in neighboring tiles
    switch (direction) {
    case WangId::Top:
        break;
    case WangId::TopRight:
        break;
    case WangId::Right:
        break;
    case WangId::BottomRight:
        break;
    case WangId::Bottom:
        break;
    case WangId::BottomLeft:
        break;
    case WangId::Left:
        break;
    case WangId::TopLeft:
        break;
    case WangId::NumCorners:
        break;
    case WangId::NumEdges:
        break;
    case WangId::NumIndexes:
        break;
    }

    auto brushMode = mBrushMode;

    if (brushMode == BrushMode::PaintEdgeAndCorner)
        brushMode = WangId::isCorner(direction) ? BrushMode::PaintCorner : BrushMode::PaintEdge;

    switch (brushMode) {
    case BrushMode::PaintCorner: {
        QPoint adjacentPoints[WangId::NumCorners];

        if (hexgonalRenderer) {
            adjacentPoints[0] = hexgonalRenderer->topRight(pos.x(), pos.y());
            adjacentPoints[1] = pos;
            adjacentPoints[2] = hexgonalRenderer->topLeft(pos.x(), pos.y());
            adjacentPoints[3] = hexgonalRenderer->topRight(adjacentPoints[2].x(), adjacentPoints[2].y());
        } else {
            for (int i = 0; i < WangId::NumCorners; ++i)
                adjacentPoints[i] = pos + aroundVertexPoints[i];
        }

        for (int i = 0; i < WangId::NumCorners; ++i) {
            const QPoint p = adjacentPoints[i];

            region += QRect(p, QSize(1, 1));

            WangFiller::CellInfo adjacent = grid.get(p);
            adjacent.desired.setCornerColor((i + 2) % 4, color);
            adjacent.mask.setCornerColor((i + 2) % 4, WangId::INDEX_MASK);

            grid.set(p, adjacent);
        }

        break;
    }
    case BrushMode::PaintEdge: {
        QPoint dirPoint;
        if (hexgonalRenderer) {
            switch (direction) {
            case WangId::Top:
                dirPoint = hexgonalRenderer->topRight(pos.x(), pos.y());
                break;
            case WangId::Right:
                dirPoint = hexgonalRenderer->bottomRight(pos.x(), pos.y());
                break;
            case WangId::Bottom:
                dirPoint = hexgonalRenderer->bottomLeft(pos.x(), pos.y());
                break;
            case WangId::Left:
                dirPoint = hexgonalRenderer->topLeft(pos.x(), pos.y());
                break;
            default: // Other color indexes not handled when painting edges
                break;
            }
        } else {
            dirPoint = pos + aroundTilePoints[direction];
        }

        region += QRect(pos, QSize(1, 1));
        region += QRect(dirPoint, QSize(1, 1));

        {
            WangFiller::CellInfo info = grid.get(pos);
            info.desired.setIndexColor(direction, color);
            info.mask.setIndexColor(direction, WangId::INDEX_MASK);
            grid.set(pos, info);
        }
        {
            WangFiller::CellInfo info = grid.get(dirPoint);
            info.desired.setIndexColor(WangId::oppositeIndex(direction), color);
            info.mask.setIndexColor(WangId::oppositeIndex(direction), WangId::INDEX_MASK);
            grid.set(dirPoint, info);
        }

        break;
    }
    case BrushMode::PaintEdgeAndCorner: // Handled before switch
    case BrushMode::Idle:
        break;
    }
}

void WangPainter::clear()
{
    mCurrentFill = WangFiller::FillRegion();
}

void WangPainter::commit(MapDocument *mapDocument, TileLayer *tileLayer)
{
    SharedTileLayer stamp = SharedTileLayer::create(QString(), 0, 0, 0, 0);
    WangFiller wangFiller{*mWangSet, mapDocument->renderer()};
    wangFiller.setCorrectionsEnabled(true);

    wangFiller.fillRegion(*stamp, *tileLayer, mCurrentFill.region, mCurrentFill.grid);

    QRegion brushRegion = stamp->region([](const Cell &cell)
                                            { return cell.checked(); });
    brushRegion.translate(tileLayer->position());
    QRect brushRect = brushRegion.boundingRect();
    stamp->setPosition(brushRect.topLeft());
    stamp->resize(brushRect.size(), -brushRect.topLeft());

    for (int j = 0; j < stamp->height(); ++j) {
        for (int i = 0; i < stamp->width(); ++i) {
            const Cell &cell = stamp->cellAt(i, j);
            if (!cell.tile())
                continue;
            
            tileLayer->setCell(stamp->x() + i, stamp->y() + j, cell);
        }
    }
    clear();
}

void WangPainter::generateTerrainAt(MapDocument *mapDocument, WangFiller::FillRegion &fill, int color, QPoint pos, WangId::Index direction, bool useTileMode)
{
    auto hexgonalRenderer = dynamic_cast<HexagonalRenderer *>(mapDocument->renderer());
    Grid<WangFiller::CellInfo> &grid = fill.grid;
    QRegion &region = fill.region;

    if (useTileMode) {
        // array of adjacent positions which is assigned based on map orientation.
        QPoint adjacentPositions[WangId::NumIndexes];
        if (hexgonalRenderer) {
            adjacentPositions[0] = hexgonalRenderer->topRight(pos.x(), pos.y());
            adjacentPositions[2] = hexgonalRenderer->bottomRight(pos.x(), pos.y());
            adjacentPositions[4] = hexgonalRenderer->bottomLeft(pos.x(), pos.y());
            adjacentPositions[6] = hexgonalRenderer->topLeft(pos.x(), pos.y());

            if (mapDocument->map()->staggerAxis() == Map::StaggerX) {
                adjacentPositions[1] = pos + QPoint(2, 0);
                adjacentPositions[3] = pos + QPoint(0, 1);
                adjacentPositions[5] = pos + QPoint(-2, 0);
                adjacentPositions[7] = pos + QPoint(0, -1);
            } else {
                adjacentPositions[1] = pos + QPoint(1, 0);
                adjacentPositions[3] = pos + QPoint(0, 2);
                adjacentPositions[5] = pos + QPoint(-1, 0);
                adjacentPositions[7] = pos + QPoint(0, -2);
            }
        } else {
            for (int i = 0; i < WangId::NumIndexes; ++i)
                adjacentPositions[i] = pos + aroundTilePoints[i];
        }

        WangFiller::CellInfo center = grid.get(pos);

        switch (mBrushMode) {
        case BrushMode::PaintCorner:
            for (int i = 0; i < WangId::NumCorners; ++i) {
                center.desired.setCornerColor(i, color);
                center.mask.setCornerColor(i, WangId::INDEX_MASK);
            }
            break;
        case BrushMode::PaintEdge:
            for (int i = 0; i < WangId::NumEdges; ++i) {
                center.desired.setEdgeColor(i, color);
                center.mask.setEdgeColor(i, WangId::INDEX_MASK);
            }
            break;
        case BrushMode::PaintEdgeAndCorner:
            for (int i = 0; i < WangId::NumIndexes; ++i) {
                center.desired.setIndexColor(i, color);
                center.mask.setIndexColor(i, WangId::INDEX_MASK);
            }
            break;
        case BrushMode::Idle:
            break;
        }

        region += QRect(pos, QSize(1, 1));
        grid.set(pos, center);

        for (int i = 0; i < WangId::NumIndexes; ++i) {
            const bool isCorner = WangId::isCorner(i);
            if (mBrushMode == BrushMode::PaintEdge && isCorner)
                continue;

            QPoint p = adjacentPositions[i];
            WangFiller::CellInfo adjacent = grid.get(p);

            // Mark the opposite side or corner of the adjacent tile
            if (isCorner || (mBrushMode == BrushMode::PaintEdge || mBrushMode == BrushMode::PaintEdgeAndCorner)) {
                adjacent.desired.setIndexColor(WangId::oppositeIndex(i), color);
                adjacent.mask.setIndexColor(WangId::oppositeIndex(i), WangId::INDEX_MASK);
            }

            // Mark the touching corners of the adjacent tile
            if (!isCorner && (mBrushMode == BrushMode::PaintCorner || mBrushMode == BrushMode::PaintEdgeAndCorner)) {
                adjacent.desired.setIndexColor((i + 3) % WangId::NumIndexes, color);
                adjacent.desired.setIndexColor((i + 5) % WangId::NumIndexes, color);
                adjacent.mask.setIndexColor((i + 3) % WangId::NumIndexes, WangId::INDEX_MASK);
                adjacent.mask.setIndexColor((i + 5) % WangId::NumIndexes, WangId::INDEX_MASK);
            }

            region += QRect(p, QSize(1, 1));
            grid.set(p, adjacent);
        }
    } else {
        if (direction == WangId::NumIndexes)
            return;

        auto brushMode = mBrushMode;

        if (brushMode == BrushMode::PaintEdgeAndCorner)
            brushMode = WangId::isCorner(direction) ? BrushMode::PaintCorner : BrushMode::PaintEdge;

        switch (brushMode) {
            case BrushMode::PaintCorner: {
                QPoint adjacentPoints[WangId::NumCorners];

                if (hexgonalRenderer) {
                    adjacentPoints[0] = hexgonalRenderer->topRight(pos.x(), pos.y());
                    adjacentPoints[1] = pos;
                    adjacentPoints[2] = hexgonalRenderer->topLeft(pos.x(), pos.y());
                    adjacentPoints[3] = hexgonalRenderer->topRight(adjacentPoints[2].x(), adjacentPoints[2].y());
                } else {
                    for (int i = 0; i < WangId::NumCorners; ++i)
                        adjacentPoints[i] = pos + aroundVertexPoints[i];
                }

                for (int i = 0; i < WangId::NumCorners; ++i) {
                    const QPoint p = adjacentPoints[i];

                    region += QRect(p, QSize(1, 1));

                    WangFiller::CellInfo adjacent = grid.get(p);
                    adjacent.desired.setCornerColor((i + 2) % 4, color);
                    adjacent.mask.setCornerColor((i + 2) % 4, WangId::INDEX_MASK);

                    grid.set(p, adjacent);
                }

                break;
            }
            case BrushMode::PaintEdge: {
                QPoint dirPoint;
                if (hexgonalRenderer) {
                    switch (direction) {
                    case WangId::Top:
                        dirPoint = hexgonalRenderer->topRight(pos.x(), pos.y());
                        break;
                    case WangId::Right:
                        dirPoint = hexgonalRenderer->bottomRight(pos.x(), pos.y());
                        break;
                    case WangId::Bottom:
                        dirPoint = hexgonalRenderer->bottomLeft(pos.x(), pos.y());
                        break;
                    case WangId::Left:
                        dirPoint = hexgonalRenderer->topLeft(pos.x(), pos.y());
                        break;
                    default: // Other color indexes not handled when painting edges
                        break;
                    }
                } else {
                    dirPoint = pos + aroundTilePoints[direction];
                }

                region += QRect(pos, QSize(1, 1));
                region += QRect(dirPoint, QSize(1, 1));

                {
                    WangFiller::CellInfo info = grid.get(pos);
                    info.desired.setIndexColor(direction, color);
                    info.mask.setIndexColor(direction, WangId::INDEX_MASK);
                    grid.set(pos, info);
                }
                {
                    WangFiller::CellInfo info = grid.get(dirPoint);
                    info.desired.setIndexColor(WangId::oppositeIndex(direction), color);
                    info.mask.setIndexColor(WangId::oppositeIndex(direction), WangId::INDEX_MASK);
                    grid.set(dirPoint, info);
                }

                break;
            }
            case BrushMode::PaintEdgeAndCorner: // Handled before switch
            case BrushMode::Idle:
                break;
        }
    }
}

}
