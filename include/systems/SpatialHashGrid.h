#pragma once
#include "ecs/Entity.h"
#include <vector>
#include <unordered_map>
#include <DirectXMath.h>
#include <DirectXCollision.h>

// A simple spatial hash grid for 3D space to optimize collision detection.
class SpatialHashGrid {
public:
    // Constructor: Defines the grid's cell size.
    SpatialHashGrid(float cellSize);

    // Clears all objects from the grid.
    void Clear();

    // Inserts an entity with its bounding box into the grid.
    void Insert(Entity entity, const DirectX::BoundingBox& box);

    // Queries the grid to find potential colliders for a given bounding box.
    // Fills the potentialColliders vector with entities in the same or adjacent cells.
    void Query(const DirectX::BoundingBox& box, std::vector<Entity>& potentialColliders);

private:
    // Hash function to convert 3D world coordinates to a 1D grid cell index.
    int64_t Hash(const DirectX::XMFLOAT3& point) const;

    // Get the min and max grid coordinates for a bounding box.
    void GetMinMaxGridCoords(const DirectX::BoundingBox& box, DirectX::XMFLOAT3& min, DirectX::XMFLOAT3& max) const;

    float m_cellSize;
    float m_inverseCellSize;
    std::unordered_map<int64_t, std::vector<Entity>> m_grid;
    std::unordered_set<uint32_t> m_tempQuerySet; // Reusable set for query to avoid allocations
};
