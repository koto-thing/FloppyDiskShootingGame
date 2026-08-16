#pragma once

/**
 * @brief 頂点バッファレス描画のための極小メッシュ定義クラス
 */
class Mesh {
public:
    Mesh(int shapeType, int vertexCount) 
        : m_shapeType(shapeType), m_vertexCount(vertexCount) {}
    
    ~Mesh() {}

    int GetShapeType() const { return m_shapeType; }
    int GetVertexCount() const { return m_vertexCount; }

private:
    int m_shapeType;   // 0: Plate (XZ面), 1: Cube, 2: Pyramid, 3: Sprite2D (XY面)
    int m_vertexCount; // 36, 18, 4 など
};
