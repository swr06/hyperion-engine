#ifndef AUDIO_MANAGER_H
#define AUDIO_MANAGER_H

#include <AL/al.h>
#include <AL/alc.h>

#include "audio_control.h"
#include "audio_source.h"
#include "../math/vector3.h"

namespace hyperion {
class AudioManager {
public:
    static AudioManager *GetInstance();
    static void Deinitialize();

    AudioManager();
    ~AudioManager();

    bool Initialize();
    inline bool IsInitialized() const { return m_is_initialized; }
    void ListDevices();
    inline ALCdevice *GetDevice() const { return m_device; }
    inline ALCcontext *GetContext() const { return m_context; }

    void SetListenerPosition(const Vector3 &position);
    void SetListenerOrientation(const Vector3 &forward, const Vector3 &up);

private:
    static AudioManager *instance;

    bool m_is_initialized;

    ALCdevice *m_device;
    ALCcontext *m_context;
};
} // namespace hyperion

#endif
