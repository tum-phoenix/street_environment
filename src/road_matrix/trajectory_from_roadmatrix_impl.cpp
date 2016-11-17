#include "street_environment/road_matrix/trajectory_from_roadmatrix_impl.h"

#include <math.h>
#include <stdlib.h>

#include "lms/math/vertex.h"

namespace {
const int kLaneValueStep = 1;
const float kPerfectTrajectoryFactor = 0.75;
const lms::math::vertex2f kCarPosition = lms::math::vertex2f(0, 0);
const int kObstacleClearanceCellsFrontCurrentLane = 5;
const int kObstacleClearanceCellsFrontOtherLane = 15;
const int kObstacleClearanceCellsBackCurrentLane = 5;
const int kObstacleClearanceCellsBackOtherLane = 5;
}

void TrajectoryFromRoadmatrixImpl::calculateCycleConstants(
    const street_environment::RoadMatrix& roadMatrix) {
    m_cellsPerLane = roadMatrix.width() / 2;
    m_carWidthCells = ceil(m_carWidthMeter / roadMatrix.cellWidth());
    m_perfectTrajectory = roadMatrix.width() * kPerfectTrajectoryFactor;
    m_numLanes = roadMatrix.width() - m_carWidthCells + 1;
    m_maxCellValue = m_perfectTrajectory * kLaneValueStep;
    m_maxLanePieceValue = m_maxCellValue * m_carWidthCells + 1;

    m_obstacleClearanceCellsFrontCurrentLane = ceil(
        m_obstacleClearanceMeterFrontCurrentLane / roadMatrix.cellLength());
    m_obstacleClearanceCellsFrontOtherLane =
        ceil(m_obstacleClearanceMeterFrontOtherLane / roadMatrix.cellLength());
    m_obstacleClearanceCellsBackCurrentLane =
        ceil(m_obstacleClearanceMeterBackCurrentLane / roadMatrix.cellLength());
    m_obstacleClearanceCellsBackOtherLane =
        ceil(m_obstacleClearanceMeterBackOtherLane / roadMatrix.cellLength());
}

std::unique_ptr<LanePieceMatrix>
TrajectoryFromRoadmatrixImpl::createLanePieceMatrix(
    const street_environment::RoadMatrix& roadMatrix) const {
    std::unique_ptr<LanePieceMatrix> lanePieceMatrix(new LanePieceMatrix(
        roadMatrix.length(), std::vector<LanePiece>(m_numLanes)));
    for (int x = 0; x < roadMatrix.length(); x++) {
        for (int y = 0; y < m_numLanes; y++) {
            int value = 0;
            LanePiece lanePiece;
            for (int i = 0; i < m_carWidthCells; i++) {
                const street_environment::RoadMatrixCell& cell =
                    roadMatrix.cell(x, y + i);
                value += valueFunction(cell, roadMatrix);
                lanePiece.cells.push_back(cell);
            }

            // TODO(arthurmathies) This needs some rethinking. Maybe there is a
            // simpler way:
            // Scale the whole lane piece back down so that lane pieces that are
            // partially blocked and fully blocked are comparable.
            if (value < m_carWidthCells * m_maxLanePieceValue) {
                int free_cells = value / m_maxLanePieceValue;
                value -= free_cells * m_maxLanePieceValue;
            }
            lanePiece.value = value;
            lanePieceMatrix->at(x).push_back(lanePiece);
        }
    }
    return lanePieceMatrix;
}

std::unique_ptr<LanePieceTrajectory>
TrajectoryFromRoadmatrixImpl::getOptimalLanePieceTrajectory(
    const LanePieceMatrix& lanePieceMatrix) const {
    std::unique_ptr<LanePieceTrajectory> cellLane(new LanePieceTrajectory);
    for (const auto& pieces : lanePieceMatrix) {
        if (pieces.size() > 0) {
            const LanePiece* a = &pieces[0];
            for (const auto& piece : pieces) {
                if (piece.value > a->value) {
                    a = &piece;
                }
            }
            // The road is blocked. No need to calculate the trajectory any
            // further.
            if (a->value <= m_maxLanePieceValue * m_carWidthCells) {
                cellLane->push_back(*a);
                cellLane->back().stop = true;
                return cellLane;
            }
            cellLane->push_back(*a);
        }
    }
    return cellLane;
}

bool TrajectoryFromRoadmatrixImpl::fillTrajectory(
    const LanePieceTrajectory& lanePieceTrajectory,
    street_environment::Trajectory& trajectory) const {
    street_environment::TrajectoryPoint prevTp;
    street_environment::TrajectoryPoint curTp;
    prevTp.position = kCarPosition;

    for (const LanePiece& piece : lanePieceTrajectory) {
        curTp.position =
            (piece.cells.front().points[1] + piece.cells.back().points[2]) / 2;
        if (piece.stop) {
            curTp.velocity = 0;
        } else {
            curTp.velocity = 1;
        }
        curTp.directory = (curTp.position - prevTp.position).normalize();
        trajectory.push_back(curTp);
        prevTp = curTp;
    }

    return true;
}

int TrajectoryFromRoadmatrixImpl::valueFunction(
    const street_environment::RoadMatrixCell& cell,
    const street_environment::RoadMatrix& roadMatrix) const {
    int value =
        m_maxCellValue - (abs(m_perfectTrajectory - cell.y) * kLaneValueStep);

    if (cell.hasObstacle) {
        value = -m_maxLanePieceValue - 1;
        return value;
    }

    if (cell.y < m_perfectTrajectory - m_carWidthCells/2) {
        for (int x = 1; x <= m_obstacleClearanceCellsFrontOtherLane; x++) {
            if ((cell.x + x < roadMatrix.length()) &&
                (roadMatrix.cell(cell.x + x, cell.y).hasObstacle)) {
                return value;
            }
        }
        for (int x = 1; x <= m_obstacleClearanceCellsBackOtherLane; x++) {
            if ((cell.x - x >= 0) &&
                (roadMatrix.cell(cell.x - x, cell.y).hasObstacle)) {
                return value;
            }
        }
    } else {
        for (int x = 1; x <= m_obstacleClearanceCellsFrontCurrentLane; x++) {
            if ((cell.x + x < roadMatrix.length()) &&
                (roadMatrix.cell(cell.x + x, cell.y).hasObstacle)) {
                return value;
            }
        }
        for (int x = 1; x <= m_obstacleClearanceCellsBackCurrentLane; x++) {
            if ((cell.x - x >= 0) &&
                (roadMatrix.cell(cell.x - x, cell.y).hasObstacle)) {
                return value;
            }
        }
    }

    value += m_maxLanePieceValue;
    return value;
}
