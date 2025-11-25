#include "pch.h"
#include "systems/SpatialHashGrid.h"
#include <unordered_set>
#include <cmath> // For floor

SpatialHashGrid::SpatialHashGrid(float cellSize)
    : m_cellSize(cellSize), m_inverseCellSize(1.0f / cellSize) {}

void SpatialHashGrid::Clear() {
    m_grid.clear();
}

void SpatialHashGrid::Insert(Entity entity, const DirectX::BoundingBox& box) {
    DirectX::XMFLOAT3 min, max;
    GetMinMaxGridCoords(box, min, max);

    for (int x = static_cast<int>(min.x); x <= static_cast<int>(max.x); ++x) {
        for (int y = static_cast<int>(min.y); y <= static_cast<int>(max.y); ++y) {
            for (int z = static_cast<int>(min.z); z <= static_cast<int>(max.z); ++z) {
                int64_t hash = Hash(DirectX::XMFLOAT3(static_cast<float>(x), static_cast<float>(y), static_cast<float>(z)));
                auto it = m_grid.find(hash);
                if (it != m_grid.end()) {
                    it->second.push_back(entity);
                } else {
                    m_grid.emplace(hash, std::vector<Entity>{entity});
                }
            }
        }
    }
}

void SpatialHashGrid::Query(const DirectX::BoundingBox& box, std::vector<Entity>& potentialColliders) {
    DirectX::XMFLOAT3 min, max;
    GetMinMaxGridCoords(box, min, max);

    potentialColliders.clear();
    m_tempQuerySet.clear();

    for (int x = static_cast<int>(min.x); x <= static_cast<int>(max.x); ++x) {
        for (int y = static_cast<int>(min.y); y <= static_cast<int>(max.y); ++y) {
            for (int z = static_cast<int>(min.z); z <= static_cast<int>(max.z); ++z) {
                int64_t hash = Hash(DirectX::XMFLOAT3(static_cast<float>(x), static_cast<float>(y), static_cast<float>(z)));
                auto it = m_grid.find(hash);
                if (it != m_grid.end()) {
                    for (Entity entity : it->second) {
                        if (m_tempQuerySet.find(entity.id) == m_tempQuerySet.end()) {
                            potentialColliders.push_back(entity);
                            m_tempQuerySet.insert(entity.id);
                        }
                    }
                }
            }
        }
    }
}

int64_t SpatialHashGrid::Hash(const DirectX::XMFLOAT3& point) const {
    // A common way to hash a 3D point in a grid.
    // Large prime numbers are used to reduce hash collisions.
    const int64_t p1 = 73856093;
    const int64_t p2 = 19349663;
    const int64_t p3 = 83492791;
    return static_cast<int64_t>(floor(point.x)) * p1 ^
           static_cast<int64_t>(floor(point.y)) * p2 ^
           static_cast<int64_t>(floor(point.z)) * p3;
}

void SpatialHashGrid::GetMinMaxGridCoords(const DirectX::BoundingBox& box, DirectX::XMFLOAT3& min, DirectX::XMFLOAT3& max) const {
    using namespace DirectX;
    XMVECTOR boxCenter = XMLoadFloat3(&box.Center);
    XMVECTOR boxExtents = XMLoadFloat3(&box.Extents);

    XMVECTOR boxMinVec = XMVectorSubtract(boxCenter, boxExtents);
    XMVECTOR boxMaxVec = XMVectorAdd(boxCenter, boxExtents);
    
    XMFLOAT3 boxMin, boxMax;
    XMStoreFloat3(&boxMin, boxMinVec);
    XMStoreFloat3(&boxMax, boxMaxVec);

    min.x = floor(boxMin.x * m_inverseCellSize);
    min.y = floor(boxMin.y * m_inverseCellSize);
    min.z = floor(boxMin.z * m_inverseCellSize);
    max.x = floor(boxMax.x * m_inverseCellSize);
    max.y = floor(boxMax.y * m_inverseCellSize);
    max.z = floor(boxMax.z * m_inverseCellSize);
}
