#pragma once
#if MLN_DRAWABLE_RENDERER
#include <mbgl/gfx/drawable.hpp>
#include <mbgl/renderer/change_request.hpp>
#endif
#include <mbgl/layout/layout.hpp>
#include <mbgl/renderer/render_pass.hpp>
#include <mbgl/renderer/render_source.hpp>
#include <mbgl/style/layer_properties.hpp>
#include <mbgl/tile/geometry_tile_data.hpp>
#include <mbgl/util/mat4.hpp>

#include <list>
#include <memory>
#include <string>

namespace mbgl {
class Bucket;
class DynamicFeatureIndex;
class LayerGroupBase;
class LineAtlas;
class PropertyEvaluationParameters;
class PaintParameters;
class PatternAtlas;
class RenderTile;
class RenderTree;
class SymbolBucket;
class TransformState;
class TransitionParameters;
class UploadParameters;

#if MLN_DRAWABLE_RENDERER
class ChangeRequest;
using LayerGroupBasePtr = std::shared_ptr<LayerGroupBase>;
using UniqueChangeRequest = std::unique_ptr<ChangeRequest>;
using UniqueChangeRequestVec = std::vector<UniqueChangeRequest>;
#endif

namespace gfx {
class Context;
class ShaderGroup;
class ShaderRegistry;
using ShaderGroupPtr = std::shared_ptr<ShaderGroup>;

#if MLN_DRAWABLE_RENDERER
class UniformBuffer;
using UniformBufferPtr = std::shared_ptr<UniformBuffer>;
#endif
} // namespace gfx

class LayerRenderData {
public:
    std::shared_ptr<Bucket> bucket;
    Immutable<style::LayerProperties> layerProperties;
};

class SortKeyRange {
public:
    bool isFirstRange() const { return start == 0u; }
    float sortKey;
    size_t start;
    size_t end;
};

class BucketPlacementData {
public:
    std::reference_wrapper<Bucket> bucket;
    std::reference_wrapper<const RenderTile> tile;
    std::shared_ptr<FeatureIndex> featureIndex;
    std::string sourceId;
    std::optional<SortKeyRange> sortKeyRange;
};

using LayerPlacementData = std::list<BucketPlacementData>;

class LayerPrepareParameters {
public:
    RenderSource* source;
    ImageManager& imageManager;
    PatternAtlas& patternAtlas;
    LineAtlas& lineAtlas;
    const TransformState& state;
};

class RenderLayer {
protected:
    RenderLayer(Immutable<style::LayerProperties>);

public:
    virtual ~RenderLayer() = default;

    // Begin transitions for any properties that have changed since the last frame.
    virtual void transition(const TransitionParameters&) = 0;

    // Overloaded version for transitions to a new layer impl.
    void transition(const TransitionParameters&, Immutable<style::Layer::Impl> newImpl);

    // Fully evaluate possibly-transitioning paint properties based on a zoom
    // level. Updates the contained `evaluatedProperties` member.
    virtual void evaluate(const PropertyEvaluationParameters&) = 0;

    // Returns true if any paint properties have active transitions.
    virtual bool hasTransition() const = 0;

    // Returns true if the layer has a pattern property and is actively crossfading.
    virtual bool hasCrossfade() const = 0;

    // Returns true if layer writes to depth buffer by drawing using PaintParameters::depthModeFor3D().
    virtual bool is3D() const { return false; }

    // Returns true is the layer is subject to placement.
    bool needsPlacement() const;

    const std::string& getID() const;

    int32_t getLayerIndex() const noexcept;

    // Checks whether this layer needs to be rendered in the given render pass.
    bool hasRenderPass(RenderPass) const;

    // Checks whether this layer can be rendered.
    bool needsRendering() const;

    // Checks whether the given zoom is inside this layer zoom range.
    bool supportsZoom(float zoom) const;

    virtual void upload(gfx::UploadPass&) {}
    virtual void render(PaintParameters&){};

    // Check wether the given geometry intersects
    // with the feature
    virtual bool queryIntersectsFeature(const GeometryCoordinates&,
                                        const GeometryTileFeature&,
                                        const float,
                                        const TransformState&,
                                        const float,
                                        const mat4&,
                                        const FeatureState&) const {
        return false;
    };

    virtual void populateDynamicRenderFeatureIndex(DynamicFeatureIndex&) const {}

    virtual void prepare(const LayerPrepareParameters&);

    const LayerPlacementData& getPlacementData() const { return placementData; }

    // Latest evaluated properties.
    Immutable<style::LayerProperties> evaluatedProperties;
    // Private implementation
    Immutable<style::Layer::Impl> baseImpl;

    virtual void markContextDestroyed();

    // TODO: Only for background layers.
    virtual std::optional<Color> getSolidBackground() const;

#if MLN_DRAWABLE_RENDERER
    /// Generate any changes needed by the layer
    virtual void update(
        gfx::ShaderRegistry&, gfx::Context&, const TransformState&, const RenderTree&, UniqueChangeRequestVec&) {}

    /// Called when the style layer is replaced (same ID and type), and the render layer is reused.
    virtual void layerChanged(const TransitionParameters&,
                              const Immutable<style::Layer::Impl>& newLayer,
                              UniqueChangeRequestVec&);

    /// Called when the style layer is removed
    virtual void layerRemoved(UniqueChangeRequestVec&);

    /// @brief Called by the RenderOrchestrator during RenderTree construction.
    /// This event is run when a layer is added or removed from the style.
    /// @param newLayerIndex The new layer index for this layer
    /// @param changes The collection of current pending change requests
    virtual void layerIndexChanged(int32_t newLayerIndex, UniqueChangeRequestVec& changes);

    /// @brief Called by the RenderOrchestrator during RenderTree construction.
    /// This event is run to indicate if the layer should render or not for the current frame.
    /// @param willRender Indicates if this layer should render or not
    /// @param changes The collection of current pending change requests
    virtual void markLayerRenderable(bool willRender, UniqueChangeRequestVec& changes);
#endif

protected:
    // Checks whether the current hardware can render this layer. If it can't,
    // we'll show a warning in the console to inform the developer.
    void checkRenderability(const PaintParameters&, uint32_t activeBindingCount);

    void addRenderPassesFromTiles();

    const LayerRenderData* getRenderDataForPass(const RenderTile&, RenderPass) const;

#if MLN_DRAWABLE_RENDERER
    void setLayerGroup(LayerGroupBasePtr, UniqueChangeRequestVec&);

    /// (Un-)Register the layer group with the orchestrator
    void activateLayerGroup(const LayerGroupBasePtr&, bool activate, UniqueChangeRequestVec& changes);

    /// Remove all drawables for the tile from the layer group
    void removeTile(RenderPass, const OverscaledTileID&);

    /// Remove all the drawables for tiles
    void removeAllDrawables();
#endif

protected:
    // Stores current set of tiles to be rendered for this layer.
    RenderTiles renderTiles;

    // Stores what render passes this layer is currently enabled for. This depends on the
    // evaluated StyleProperties object and is updated accordingly.
    RenderPass passes = RenderPass::None;

    LayerPlacementData placementData;

#if MLN_DRAWABLE_RENDERER
    // will need to be overriden to handle their activation.
    LayerGroupBasePtr layerGroup;
#endif
    // Current layer index as specified by the layerIndexChanged event
    int32_t layerIndex{0};

    // Current renderable status as specified by the markLayerRenderable event
    bool isRenderable{false};

    std::mutex mutex;

    struct Stats {
        size_t propertyEvaluations = 0;
        size_t drawablesAdded = 0;
        size_t drawablesRemoved = 0;
    } stats;

private:
    // Some layers may not render correctly on some hardware when the vertex
    // attribute limit of that GPU is exceeded. More attributes are used when
    // adding many data driven paint properties to a layer.
    bool hasRenderFailures = false;
};

using RenderLayerReferences = std::vector<std::reference_wrapper<RenderLayer>>;

} // namespace mbgl
