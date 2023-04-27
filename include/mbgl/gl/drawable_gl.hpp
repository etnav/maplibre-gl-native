#pragma once

#include <mbgl/gfx/drawable.hpp>
#include <mbgl/gl/vertex_attribute_gl.hpp>

#include <memory>

namespace mbgl {

namespace gfx {

class IndexBuffer;
class VertexBufferResource;

using UniqueVertexBufferResource = std::unique_ptr<gfx::VertexBufferResource>;

} // namespace gfx

namespace gl {

class VertexArray;

class DrawableGL : public gfx::Drawable {
public:
    DrawableGL();
    ~DrawableGL() override;

    void draw(const PaintParameters &) const override;

    void setIndexData(std::vector<uint16_t> indexes,
                      std::size_t indexOffset = 0,
                      std::size_t indexLength = 0);
    std::vector<std::uint16_t>& getIndexData() const override;

    const gfx::VertexAttributeArray& getVertexAttributes() const override;
    void setVertexAttributes(const gfx::VertexAttributeArray& value) override;
    void setVertexAttributes(gfx::VertexAttributeArray&& value) override;

    gfx::VertexAttributeArray& mutableVertexAttributes();

    const gl::VertexArray& getVertexArray() const;
    void setVertexArray(gl::VertexArray&&, gfx::UniqueVertexBufferResource&&, gfx::IndexBuffer&&);

    const gfx::UniqueVertexBufferResource& getBuffer() const;
    const gfx::IndexBuffer& getIndexBuffer() const;
    
protected:
    class Impl;
    const std::unique_ptr<Impl> impl;

    // For testing only.
    DrawableGL(std::unique_ptr<Impl>);
    
private:
};

} // namespace gl
} // namespace mbgl