#ifndef TEXTURE_2D_H
#define TEXTURE_2D_H

#include "texture.h"

namespace hyperion {

class Texture2D : public Texture {
public:
    Texture2D();
    Texture2D(int width, int height, unsigned char *bytes);
    virtual ~Texture2D();

    void Resize(int new_width, int new_height);

    virtual void End() override;
    virtual void CopyData(Texture * const other) override;

protected:
    virtual void UploadGpuData(bool should_upload_data) override;
    virtual void Use() override;
};

} // namespace hyperion

#endif
