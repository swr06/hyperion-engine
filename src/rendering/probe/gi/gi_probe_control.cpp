#include "gi_probe_control.h"
#include "gi_mapper.h"
#include "../probe_manager.h"
#include "../../../scene/node.h"

#include <memory>

namespace hyperion {
GIProbeControl::GIProbeControl(const Vector3 &origin)
    : EntityControl(fbom::FBOMObjectType("GI_PROBE_CONTROL"), 10.0),
      m_origin(origin),
      m_gi_mapper_node(new Node("GI Mapper Node")),
      m_gi_mapper(std::make_shared<GIMapper>(origin, BoundingBox(-Vector3(ProbeManager::voxel_map_size / 2.0f), Vector3(ProbeManager::voxel_map_size / 2.0f))))
{
    m_gi_mapper_node->SetRenderable(m_gi_mapper);
    m_gi_mapper_node->GetSpatial().SetBucket(Spatial::Bucket::RB_BUFFER);
}

void GIProbeControl::OnAdded()
{
    // Add node to parent, with renderable as GIMapper
    parent->AddChild(m_gi_mapper_node);
    ProbeManager::GetInstance()->AddProbe(m_gi_mapper);
}

void GIProbeControl::OnRemoved()
{
    parent->RemoveChild(m_gi_mapper_node);
    ProbeManager::GetInstance()->RemoveProbe(m_gi_mapper);
}

void GIProbeControl::OnUpdate(double dt)
{
    m_gi_mapper->SetOrigin(parent->GetGlobalTranslation());
    m_gi_mapper->Update(dt);
}

std::shared_ptr<Control> GIProbeControl::CloneImpl()
{
    return std::make_shared<GIProbeControl>(m_origin);
}
} // namespace hyperion