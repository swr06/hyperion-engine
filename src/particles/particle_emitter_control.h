#ifndef PARTICLE_EMITTER_CONTROL_H
#define PARTICLE_EMITTER_CONTROL_H

#include "../controls/entity_control.h"
#include "../rendering/camera/camera.h"
#include "particle.h"
#include "particle_construction_info.h"
#include "particle_renderer.h"

#include <vector>
#include <memory>

namespace hyperion {
class ParticleEmitterControl : public EntityControl {
public:
    ParticleEmitterControl(Camera *camera, const ParticleConstructionInfo &info);

    void ResetParticle(Particle &particle);

    void OnAdded();
    void OnRemoved();
    void OnUpdate(double dt);

private:
    virtual std::shared_ptr<Control> CloneImpl() override;

    Camera *m_camera;
    std::vector<Particle> m_particles;
    std::shared_ptr<Node> m_node;
    std::shared_ptr<ParticleRenderer> m_particle_renderer;
};
} // namespace hyperion

#endif
