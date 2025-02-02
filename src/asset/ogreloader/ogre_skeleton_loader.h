#ifndef OGRE_SKELETON_LOADER_H
#define OGRE_SKELETON_LOADER_H

#include "../asset_loader.h"
#include "../../animation/skeleton.h"
#include "../../animation/bone.h"
#include "../../util/xml/sax_parser.h"

#include <string>

namespace hyperion {
class OgreSkeletonLoader : public AssetLoader {
public:
    std::shared_ptr<Loadable> LoadFromFile(const std::string &);
};
}

#endif
