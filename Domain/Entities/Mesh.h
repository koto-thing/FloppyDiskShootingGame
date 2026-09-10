#pragma once

/**
 * @brief 頂点バッファレス描画のための極小メッシュ定義クラス
 */
class Mesh {
public:
    /**
     * @brief メッシュ定義を生成する
     * @param shapeType 形状種別
     * @param vertexCount 頂点数
     */
    Mesh(int shapeType, int vertexCount) 
        : m_shapeType(shapeType), m_vertexCount(vertexCount) {}
    
    /** @brief メッシュ定義を破棄する */
    ~Mesh() {}

    /** @brief 形状種別を取得する */
    int GetShapeType() const { return m_shapeType; }
    /** @brief 頂点数を取得する */
    int GetVertexCount() const { return m_vertexCount; }

private:
    int m_shapeType;   // 0: Plate (XZ面), 1: Cube, 2: Pyramid, 3: Sprite2D (XY面)
    int m_vertexCount; // 36, 18, 4 など
};
