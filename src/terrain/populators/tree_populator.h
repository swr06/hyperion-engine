#ifndef TREE_POPULATOR_H
#define TREE_POPULATOR_H

#include "populator.h"

namespace hyperion {

class TreePopulator : public Populator {
public:
    TreePopulator(
        Camera *camera,
        unsigned long seed = 555,
        double probability_factor = 0.3,
        float tolerance = 0.15f,
        float max_distance = 200.0f,
        float spread = 8.5f,
        int num_entities_per_chunk = 2,
        int num_patches = 4
    );
    ~TreePopulator();

    std::shared_ptr<Node> CreateEntity(const Vector3 &position) const override;

protected:
    virtual std::shared_ptr<Control> CloneImpl() override;
};

} // namespace hyperion

#endif
