#ifndef BOUNDING_BOX_CONTROL_H
#define BOUNDING_BOX_CONTROL_H

#include <cstddef>
#include <vector>

#include "entity_control.h"
#include "../math/bounding_box.h"
#include "../scene/node.h"
#include "../rendering/renderers/bounding_box_renderer.h"

namespace hyperion {
class BoundingBoxControl : public EntityControl {
public:
    BoundingBoxControl();
    virtual ~BoundingBoxControl();

    virtual void OnAdded();
    virtual void OnRemoved();
    virtual void OnUpdate(double dt);

private:
    virtual std::shared_ptr<Control> CloneImpl() override;

    std::shared_ptr<Node> m_node;
    std::shared_ptr<BoundingBoxRenderer> m_bounding_box_renderer;
};
} // namespace hyperion

#endif
