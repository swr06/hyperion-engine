#include "mtl_loader.h"
#include "../../util/string_util.h"
#include "../../asset/asset_manager.h"

#include <fstream>

namespace hyperion {
std::shared_ptr<Loadable> MtlLib::Clone()
{
    std::shared_ptr<MtlLib> new_mtl = std::make_shared<MtlLib>();
    new_mtl->m_materials = m_materials;
    return new_mtl;
}

std::shared_ptr<Loadable> MtlLoader::LoadFromFile(const std::string &path)
{
    std::shared_ptr<MtlLib> mtl = std::make_shared<MtlLib>();

    std::string line;
    std::ifstream fs(path);
    if (!fs.is_open()) {
        return nullptr;
    }

    while (std::getline(fs, line)) {
        line = StringUtil::Trim(line);

        auto tokens = StringUtil::Split(line, ' ');
        tokens = StringUtil::RemoveEmpty(tokens);

        if (!tokens.empty() && tokens[0] != "#") {
            if (tokens[0] == "newmtl") {
                if (tokens.size() >= 2) {
                    std::string name = tokens[1];
                    mtl->NewMaterial(name);
                }
            } else if (tokens[0] == "map_Kd") { // diffuse map
                if (tokens.size() >= 2) {
                    const std::string loc = tokens[1];

                    if (Material *last_mtl = mtl->GetLastMaterial()) {
                        if (auto tex = LoadTexture(loc, path)) {
                            last_mtl->SetTexture("DiffuseMap", tex);
                        }
                    }
                }
            } else if (tokens[0] == "map_bump") {
                if (tokens.size() >= 2) {
                    const std::string loc = tokens[1];

                    if (Material *last_mtl = mtl->GetLastMaterial()) {
                        if (auto tex = LoadTexture(loc, path)) {
                            last_mtl->SetTexture("NormalMap", tex);
                        }
                    }
                }
            } else if (tokens[0] == "map_Ks") {
                if (tokens.size() >= 2) {
                    const std::string loc = tokens[1];

                    if (Material *last_mtl = mtl->GetLastMaterial()) {
                        if (auto tex = LoadTexture(loc, path)) {
                            last_mtl->SetTexture("MetalnessMap", tex);
                        }
                    }
                }
            } else if (tokens[0] == "Kd") {
                if (tokens.size() >= 4) {
                    Vector4 color(1.0);

                    color.x = std::stof(tokens[1]);
                    color.y = std::stof(tokens[2]);
                    color.z = std::stof(tokens[3]);

                    if (tokens.size() >= 5) {
                        color.w = std::stof(tokens[4]);
                    }

                    if (Material *last_mtl = mtl->GetLastMaterial()) {
                        last_mtl->diffuse_color = color;
                    }
                }
            }
        }
    }

    return mtl;
}

std::shared_ptr<Texture> MtlLoader::LoadTexture(const std::string &name, const std::string &path)
{
    std::shared_ptr<Texture> tex;

    std::string dir(path);
    dir = dir.substr(0, dir.find_last_of("\\/"));
    if (!(StringUtil::Contains(dir, "/") || StringUtil::Contains(dir, "\\"))) {
        dir.clear();
    }
    dir += "/" + name;

    if (!(tex = AssetManager::GetInstance()->LoadFromFile<Texture>(dir))) {
        // load relatively
        tex = AssetManager::GetInstance()->LoadFromFile<Texture>(name);
    }

    return tex;
}
}