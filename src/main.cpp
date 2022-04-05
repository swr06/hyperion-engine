#include "glfw_engine.h"
#include "game.h"
#include "scene/node.h"
#include "util.h"
#include "asset/asset_manager.h"
#include "rendering/shader.h"
#include "rendering/environment.h"
#include "rendering/texture.h"
#include "rendering/framebuffer_2d.h"
#include "rendering/framebuffer_cube.h"
#include "rendering/shaders/lighting_shader.h"
#include "rendering/shader_manager.h"
#include "rendering/shadow/pssm_shadow_mapping.h"
#include "audio/audio_manager.h"
#include "animation/skeleton_control.h"
#include "math/bounding_box.h"

#include "rendering/probe/envmap/envmap_probe_control.h"

#include "terrain/terrain_shader.h"

/* Post */
#include "rendering/postprocess/filters/gamma_correction_filter.h"
#include "rendering/postprocess/filters/ssao_filter.h"
#include "rendering/postprocess/filters/deferred_rendering_filter.h"
#include "rendering/postprocess/filters/bloom_filter.h"
#include "rendering/postprocess/filters/depth_of_field_filter.h"
#include "rendering/postprocess/filters/fxaa_filter.h"
#include "rendering/postprocess/filters/default_filter.h"

/* Extra */
#include "rendering/skydome/skydome.h"
#include "rendering/skybox/skybox.h"
#include "terrain/noise_terrain/noise_terrain_control.h"

/* Physics */
#include "physics/physics_manager.h"
#include "physics/rigid_body.h"
#include "physics/box_physics_shape.h"
#include "physics/sphere_physics_shape.h"
#include "physics/plane_physics_shape.h"

/* Particles */
#include "particles/particle_renderer.h"
#include "particles/particle_emitter_control.h"

/* Misc. Controls */
#include "controls/bounding_box_control.h"
#include "controls/camera_follow_control.h"
// #include "rendering/renderers/bounding_box_renderer.h"

/* UI */
#include "rendering/ui/ui_object.h"
#include "rendering/ui/ui_button.h"
#include "rendering/ui/ui_text.h"

/* Populators */
#include "terrain/populators/populator.h"

#include "util/noise_factory.h"
#include "util/img/write_bitmap.h"
#include "util/enum_options.h"
#include "util/mesh_factory.h"

#include "rendering/probe/gi/gi_probe_control.h"
#include "rendering/probe/spherical_harmonics/spherical_harmonics_control.h"
#include "rendering/shaders/gi/gi_voxel_debug_shader.h"
#include "rendering/shadow/shadow_map_control.h"

#include "asset/fbom/fbom.h"
#include "asset/byte_writer.h"

/* Standard library */
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <unordered_map>
#include <string>
#include <math.h>

using namespace hyperion;

class SceneEditor : public Game {
public:
    std::vector<std::shared_ptr<Node>> m_raytested_entities;
    std::unordered_map<HashCode_t, std::shared_ptr<Node>> m_hit_to_entity;
    std::shared_ptr<Node> m_selected_node;
    bool m_dragging_node = false;
    float m_dragging_timer = 0.0f;
    bool m_left_click_left = false;
    RaytestHit m_ray_hit;
    bool m_is_ray_hit = false;
    std::vector<std::thread> m_threads;

    std::shared_ptr<ui::UIText> m_selected_node_text;
    std::shared_ptr<ui::UIButton> m_rotate_mode_btn;

    SceneEditor(const RenderWindow &window)
        : Game(window)
    {
        std::srand(std::time(nullptr));

        ShaderProperties defines;
    }

    ~SceneEditor()
    {
        for (auto &thread : m_threads) {
            thread.join();
        }

        AudioManager::Deinitialize();
    }

    std::shared_ptr<Cubemap> InitCubemap()
    {
        AssetManager *asset_manager = AssetManager::GetInstance();
        std::shared_ptr<Cubemap> cubemap(new Cubemap({
            asset_manager->LoadFromFile<Texture2D>("textures/Lycksele3/posx.jpg"),
            asset_manager->LoadFromFile<Texture2D>("textures/Lycksele3/negx.jpg"),
            asset_manager->LoadFromFile<Texture2D>("textures/Lycksele3/posy.jpg"),
            asset_manager->LoadFromFile<Texture2D>("textures/Lycksele3/negy.jpg"),
            asset_manager->LoadFromFile<Texture2D>("textures/Lycksele3/posz.jpg"),
            asset_manager->LoadFromFile<Texture2D>("textures/Lycksele3/negz.jpg")
        }));

        cubemap->SetFilter(Texture::TextureFilterMode::TEXTURE_FILTER_LINEAR_MIPMAP);

        if (!ProbeManager::GetInstance()->EnvMapEnabled()) {
            Environment::GetInstance()->SetGlobalCubemap(cubemap);
        }

        return cubemap;
    }

    void PerformRaytest()
    {
        m_raytested_entities.clear();

        Ray ray;
        ray.m_direction = GetCamera()->GetDirection();
        ray.m_direction.Normalize();
        ray.m_position = GetCamera()->GetTranslation();


        using Intersection_t = std::pair<std::shared_ptr<Node>, RaytestHit>;

        RaytestHit intersection;
        std::vector<Intersection_t> intersections = { { GetScene(), intersection } };

        while (true) {
            std::vector<Intersection_t> new_intersections;

            for (auto& it : intersections) {
                for (size_t i = 0; i < it.first->NumChildren(); i++) {
                    if (const auto &child = it.first->GetChild(i)) {
                        BoundingBox aabb = child->GetAABB();

                        if (aabb.IntersectRay(ray, intersection)) {
                            new_intersections.push_back({ child, intersection });
                        }
                    }
                }
            }

            if (new_intersections.empty()) {
                break;
            }

            intersections = new_intersections;
        }

        if (intersections.empty()) {
            return;
        }

        // RAY HIT POSITION / NORMAL

        RaytestHitList_t mesh_intersections;
        m_hit_to_entity.clear();

        for (Intersection_t& it : intersections) {
            it.first->AddControl(std::make_shared<BoundingBoxControl>());

            m_raytested_entities.push_back(it.first);

            if (auto renderable = it.first->GetRenderable()) {
                RaytestHitList_t entity_hits;
                if (renderable->IntersectRay(ray, it.first->GetGlobalTransform(), entity_hits)) {
                    mesh_intersections.insert(mesh_intersections.end(), entity_hits.begin(), entity_hits.end());

                    for (auto& hit : entity_hits) {
                        m_hit_to_entity[hit.GetHashCode().Value()] = it.first;
                    }
                }

            }
        }

        if (mesh_intersections.empty()) {
            m_is_ray_hit = false;
            return;
        }

        std::sort(mesh_intersections.begin(), mesh_intersections.end(), [=](const RaytestHit& a, const RaytestHit& b) {
            return a.hitpoint.Distance(GetCamera()->GetTranslation()) < b.hitpoint.Distance(GetCamera()->GetTranslation());
        });

        m_ray_hit = mesh_intersections[0];
        m_is_ray_hit = true;
    }

    void Initialize()
    {

        ShaderManager::GetInstance()->SetBaseShaderProperties(ShaderProperties()
            .Define("NORMAL_MAPPING", true)
            .Define("SHADOW_MAP_RADIUS", 0.005f)
            .Define("SHADOW_PCF", false)
            .Define("SHADOWS_VARIANCE", true)
        );

        AssetManager *asset_manager = AssetManager::GetInstance();

        Environment::GetInstance()->SetShadowsEnabled(true);
        Environment::GetInstance()->SetNumCascades(1);
        Environment::GetInstance()->GetProbeManager()->SetEnvMapEnabled(false);
        Environment::GetInstance()->GetProbeManager()->SetSphericalHarmonicsEnabled(true);
        Environment::GetInstance()->GetProbeManager()->SetVCTEnabled(true);

        GetRenderer()->GetDeferredPipeline()->GetPreFilterStack()->AddFilter<SSAOFilter>("ssao", 5);
        GetRenderer()->GetDeferredPipeline()->GetPreFilterStack()->AddFilter<FXAAFilter>("fxaa", 6);
        GetRenderer()->GetDeferredPipeline()->GetPostFilterStack()->AddFilter<BloomFilter>("bloom", 80);

        AudioManager::GetInstance()->Initialize();

        Environment::GetInstance()->GetSun().SetDirection(Vector3(0.2, 1, 0.2).Normalize());
        Environment::GetInstance()->GetSun().SetIntensity(700000.0f);
        Environment::GetInstance()->GetSun().SetColor(Vector4(1.0, 0.8, 0.65, 1.0));
        
        GetCamera()->SetTranslation(Vector3(0, 8, 0));

        auto cm = InitCubemap();
        
        GetScene()->AddControl(std::make_shared<SkydomeControl>(GetCamera()));

        GetScene()->AddControl(std::make_shared<SphericalHarmonicsControl>(Vector3(0.0f), BoundingBox(-25.0f, 25.0f)));

        auto model = asset_manager->LoadFromFile<Node>("models/sponza/sponza.obj");
        model->SetName("model");
        model->Scale(Vector3(0.01f));

        // Add an AudioControl, provide it with an AudioSource (which is an instance of Loadable)
        auto audio_ctrl = std::make_shared<AudioControl>(
            AssetManager::GetInstance()->LoadFromFile<AudioSource>("sounds/cartoon001.wav"));

        // Add that AudioControl to your node so it is associated with a 3d space
        model->AddControl(audio_ctrl);
        audio_ctrl->GetSource()->SetLoop(true);
        audio_ctrl->GetSource()->Play();

        for (size_t i = 0; i < model->NumChildren(); i++) {
            if (model->GetChild(i) == nullptr) {
                continue;
            }

            if (model->GetChild(i)->GetRenderable() == nullptr) {
                continue;
            }

            if (StringUtil::StartsWith(model->GetChild(i)->GetName(), "grey_and_white_room:Branches")) {
                model->GetChild(i)->GetSpatial().SetBucket(Spatial::Bucket::RB_TRANSPARENT);
                model->GetChild(i)->GetSpatial().GetMaterial().alpha_blended = true;
                model->GetChild(i)->GetMaterial().SetParameter(MATERIAL_PARAMETER_METALNESS, 0.04f);
                model->GetChild(i)->GetMaterial().SetParameter(MATERIAL_PARAMETER_ROUGHNESS, 0.8f);
            } else if (StringUtil::StartsWith(model->GetChild(i)->GetName(), "grey_and_white_room:Floor")) {
                model->GetChild(i)->GetMaterial().SetParameter(MATERIAL_PARAMETER_METALNESS, 0.04f);
                model->GetChild(i)->GetMaterial().SetParameter(MATERIAL_PARAMETER_ROUGHNESS, 0.05f);
            } else if (StringUtil::StartsWith(model->GetChild(i)->GetName(), "grey_and_white_room:Leaves")) {
                model->GetChild(i)->GetSpatial().SetBucket(Spatial::Bucket::RB_TRANSPARENT);
                model->GetChild(i)->GetSpatial().GetMaterial().alpha_blended = true;
                model->GetChild(i)->GetMaterial().SetParameter(MATERIAL_PARAMETER_METALNESS, 0.04f);
                model->GetChild(i)->GetMaterial().SetParameter(MATERIAL_PARAMETER_ROUGHNESS, 0.8f);
            } else if (StringUtil::StartsWith(model->GetChild(i)->GetName(), "grey_and_white_room:TableWood")) {
                model->GetChild(i)->GetMaterial().SetParameter(MATERIAL_PARAMETER_METALNESS, 0.04f);
                model->GetChild(i)->GetMaterial().SetParameter(MATERIAL_PARAMETER_ROUGHNESS, 0.05f);
            } else if (StringUtil::StartsWith(model->GetChild(i)->GetName(), "grey_and_white_room:Transluscent")) {
                model->GetChild(i)->GetSpatial().SetBucket(Spatial::Bucket::RB_TRANSPARENT);
                model->GetChild(i)->GetSpatial().GetMaterial().alpha_blended = true;
                model->GetChild(i)->GetMaterial().diffuse_color = Vector4(1.0, 1.0, 1.0, 0.5f);
                model->GetChild(i)->GetMaterial().SetParameter(MATERIAL_PARAMETER_METALNESS, 0.04f);
                model->GetChild(i)->GetMaterial().SetParameter(MATERIAL_PARAMETER_ROUGHNESS, 0.05f);
            }

            if (StringUtil::StartsWith(model->GetChild(i)->GetName(), "Bin")) {

                model->GetChild(i)->GetMaterial().SetParameter(MATERIAL_PARAMETER_METALNESS, 0.9f);
                model->GetChild(i)->GetMaterial().SetParameter(MATERIAL_PARAMETER_ROUGHNESS, 0.7f);
            } else if (StringUtil::StartsWith(model->GetChild(i)->GetName(), "Ceramic")) {

                model->GetChild(i)->GetMaterial().SetParameter(MATERIAL_PARAMETER_METALNESS, 0.1f);
                model->GetChild(i)->GetMaterial().SetParameter(MATERIAL_PARAMETER_ROUGHNESS, 0.1f);
            } else if (StringUtil::StartsWith(model->GetChild(i)->GetName(), "Mirror")) {

                model->GetChild(i)->GetMaterial().SetParameter(MATERIAL_PARAMETER_METALNESS, 0.9f);
                model->GetChild(i)->GetMaterial().SetParameter(MATERIAL_PARAMETER_ROUGHNESS, 0.01f);
            } else if (StringUtil::StartsWith(model->GetChild(i)->GetName(), "Plastic")) {

                model->GetChild(i)->GetMaterial().SetParameter(MATERIAL_PARAMETER_METALNESS, 0.04f);
                model->GetChild(i)->GetMaterial().SetParameter(MATERIAL_PARAMETER_ROUGHNESS, 0.9f);
            }

            if (StringUtil::StartsWith(model->GetChild(i)->GetName(), "leaf")) {
                model->GetChild(i)->GetSpatial().SetBucket(Spatial::Bucket::RB_TRANSPARENT);
                model->GetChild(i)->GetSpatial().GetMaterial().alpha_blended = true;
                model->GetChild(i)->GetMaterial().SetParameter(MATERIAL_PARAMETER_METALNESS, 0.04f);
                model->GetChild(i)->GetMaterial().SetParameter(MATERIAL_PARAMETER_ROUGHNESS, 0.8f);
            } else if (StringUtil::StartsWith(model->GetChild(i)->GetName(), "Material__57")) {
                model->GetChild(i)->GetSpatial().SetBucket(Spatial::Bucket::RB_TRANSPARENT);
                model->GetChild(i)->GetSpatial().GetMaterial().alpha_blended = true;
                model->GetChild(i)->GetMaterial().SetParameter(MATERIAL_PARAMETER_METALNESS, 0.04f);
                model->GetChild(i)->GetMaterial().SetParameter(MATERIAL_PARAMETER_ROUGHNESS, 0.8f);
            } else if (StringUtil::StartsWith(model->GetChild(i)->GetName(), "vase")) {

                model->GetChild(i)->GetMaterial().SetParameter(MATERIAL_PARAMETER_METALNESS, 0.6f);
                model->GetChild(i)->GetMaterial().SetParameter(MATERIAL_PARAMETER_ROUGHNESS, 0.8f);
            } else if (StringUtil::StartsWith(model->GetChild(i)->GetName(), "chain")) {
                model->GetChild(i)->GetMaterial().SetParameter(MATERIAL_PARAMETER_METALNESS, 0.04f);
                model->GetChild(i)->GetMaterial().SetParameter(MATERIAL_PARAMETER_ROUGHNESS, 0.8f);
            } else if (StringUtil::StartsWith(model->GetChild(i)->GetName(), "flagpole")) {
                model->GetChild(i)->GetMaterial().SetParameter(MATERIAL_PARAMETER_METALNESS, 0.04f);
                model->GetChild(i)->GetMaterial().SetParameter(MATERIAL_PARAMETER_ROUGHNESS, 0.8f);
            } else {
                model->GetChild(i)->GetMaterial().SetParameter(MATERIAL_PARAMETER_METALNESS, 0.04f);
                model->GetChild(i)->GetMaterial().SetParameter(MATERIAL_PARAMETER_ROUGHNESS, 1.0f);
            }
        }

        model->AddControl(std::make_shared<GIProbeControl>(Vector3(0.0f, 1.0f, 0.0f)));
        model->AddControl(std::make_shared<EnvMapProbeControl>(Vector3(0.0f, 3.0f, 4.0f)));
        GetScene()->AddChild(model);
        
        auto shadow_node = std::make_shared<Node>("shadow_node");
        shadow_node->AddControl(std::make_shared<ShadowMapControl>(GetRenderer()->GetEnvironment()->GetSun().GetDirection() * -1.0f, 15.0f));
        shadow_node->AddControl(std::make_shared<CameraFollowControl>(GetCamera()));
        GetScene()->AddChild(shadow_node);

        bool add_spheres = true;

        if (add_spheres) {

            for (int x = 0; x < 5; x++) {
                for (int z = 0; z < 5; z++) {

                    Vector3 box_position = Vector3(((float(x))), 6.4f, (float(z)));
                    auto box = asset_manager->LoadFromFile<Node>("models/material_sphere/material_sphere.obj", true);
                    box->SetLocalScale(0.35f);

                    for (size_t i = 0; i < box->NumChildren(); i++) {
                        box->GetChild(0)->GetMaterial().SetTexture(MATERIAL_TEXTURE_DIFFUSE_MAP, asset_manager->LoadFromFile<Texture2D>("textures/rough-wet-cobble-ue/rough-wet-cobble-albedo.png"));
                        box->GetChild(0)->GetMaterial().SetTexture(MATERIAL_TEXTURE_AO_MAP, asset_manager->LoadFromFile<Texture2D>("textures/rough-wet-cobble-ue/rough-wet-cobble-ao.png"));
                        box->GetChild(0)->GetMaterial().SetTexture(MATERIAL_TEXTURE_NORMAL_MAP, asset_manager->LoadFromFile<Texture2D>("textures/rough-wet-cobble-ue/rough-wet-cobble-normal-dx.png"));
                        box->GetChild(0)->GetMaterial().SetTexture(MATERIAL_TEXTURE_ROUGHNESS_MAP, asset_manager->LoadFromFile<Texture2D>("textures/rough-wet-cobble-ue/rough-wet-cobble-roughness.png"));
                        box->GetChild(0)->GetMaterial().SetTexture(MATERIAL_TEXTURE_METALNESS_MAP, asset_manager->LoadFromFile<Texture2D>("textures/rough-wet-cobble-ue/rough-wet-cobble-metallic.png"));
                        box->GetChild(i)->GetMaterial().SetParameter(MATERIAL_PARAMETER_FLIP_UV, Vector2(0, 1));
                        
                    }
                    box->SetLocalTranslation(box_position);
                    GetScene()->AddChildAsync(box, [](auto) {});
                }
            }
        }

        GetInputManager()->RegisterKeyEvent(KEY_1, InputEvent([=](bool pressed) {
            if (!pressed) {
                return;
            }

            Environment::GetInstance()->SetShadowsEnabled(!Environment::GetInstance()->ShadowsEnabled());
        }));

        GetInputManager()->RegisterKeyEvent(KEY_3, InputEvent([=](bool pressed) {
            if (!pressed) {
                return;
            }

            ProbeManager::GetInstance()->SetEnvMapEnabled(!ProbeManager::GetInstance()->EnvMapEnabled());
        }));

        GetInputManager()->RegisterKeyEvent(KEY_4, InputEvent([=](bool pressed) {
            if (!pressed) {
                return;
            }

            ProbeManager::GetInstance()->SetSphericalHarmonicsEnabled(!ProbeManager::GetInstance()->SphericalHarmonicsEnabled());
        }));

        GetInputManager()->RegisterKeyEvent(KEY_5, InputEvent([=](bool pressed) {
            if (!pressed) {
                return;
            }

            ProbeManager::GetInstance()->SetVCTEnabled(!ProbeManager::GetInstance()->VCTEnabled());
        }));

        InputEvent raytest_event([=](bool pressed)
            {
                if (!pressed) {
                    return;
                }

                if (m_selected_node != nullptr) {
                    m_selected_node->RemoveControl(m_selected_node->GetControl<BoundingBoxControl>(0));
                }

                PerformRaytest();

                if (!m_is_ray_hit) {
                    return;
                }

                ex_assert(m_hit_to_entity.find(m_ray_hit.GetHashCode().Value()) != m_hit_to_entity.end());

                auto node = m_hit_to_entity[m_ray_hit.GetHashCode().Value()];

                m_selected_node = node;
                m_selected_node->AddControl(std::make_shared<BoundingBoxControl>());

                std::stringstream ss;
                ss << "Selected object: ";
                ss << m_selected_node->GetName();
                ss << " ";
                ss << m_selected_node->GetGlobalTranslation();
                ss << " " << m_selected_node->GetAABB();
                
                m_selected_node_text->SetText(ss.str());
            });

        GetInputManager()->RegisterClickEvent(MOUSE_BTN_LEFT, raytest_event);
    }

    void Logic(double dt)
    {
        if (GetInputManager()->IsButtonDown(MouseButton::MOUSE_BTN_LEFT) && m_selected_node != nullptr) {
            //std::cout << "Left button down\n";
            if (!m_dragging_node) {
                m_dragging_timer += dt;

                if (m_dragging_timer >= 0.5f) {
                    m_dragging_node = true;
                }
            } else {
                PerformRaytest();

                if (m_is_ray_hit) {
                    // check what it is intersecting with
                    auto intersected_with = m_hit_to_entity[m_ray_hit.GetHashCode().Value()];

                    ex_assert(intersected_with != nullptr);

                    if (intersected_with != m_selected_node) {
                        m_selected_node->SetGlobalTranslation(m_ray_hit.hitpoint);
                        m_selected_node->UpdateTransform();

                        std::stringstream ss;
                        ss << "Selected object: ";
                        ss << m_selected_node->GetName();
                        ss << " ";
                        ss << m_selected_node->GetGlobalTranslation();

                        m_selected_node_text->SetText(ss.str());
                    }
                }
            }
        } else {
            m_dragging_node = false;
            m_dragging_timer = 0.0f;
        }

        AudioManager::GetInstance()->SetListenerPosition(GetCamera()->GetTranslation());
        AudioManager::GetInstance()->SetListenerOrientation(GetCamera()->GetDirection(), GetCamera()->GetUpVector());

        PhysicsManager::GetInstance()->RunPhysics(dt);

    }

    void OnRender()
    {
        Environment::GetInstance()->CollectVisiblePointLights(GetCamera());
    }
};

int main()
{
    CoreEngine *engine = new GlfwEngine();
    CoreEngine::SetInstance(engine);

    std::string base_path = HYP_ROOT_DIR;
    AssetManager::GetInstance()->SetRootDir(base_path+"/res/");

    auto *game = new SceneEditor(RenderWindow(1480, 1200, "Hyperion Demo"));

    engine->InitializeGame(game);

    delete game;
    delete engine;

    return 0;
}
