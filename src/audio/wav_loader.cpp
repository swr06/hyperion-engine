#include "wav_loader.h"
#include "audio_manager.h"
#include "audio_source.h"

#include <iostream>
#include <fstream>

namespace hyperion {
struct RiffHeader {
    char chunk_id[4];
    uint32_t chunk_size;
    char format[4];
};

struct WaveFormat {
    char sub_chunk_id[4];
    uint32_t sub_chunk_size;
    uint16_t audio_format;
    uint16_t num_channels;
    uint32_t sample_rate;
    uint32_t byte_rate;
    uint16_t block_align;
    uint16_t bits_per_sample;
};

struct WaveData {
    char sub_chunk_id[4];
    uint32_t sub_chunk_2_size;
};

std::shared_ptr<Loadable> WavLoader::LoadFromFile(const std::string &path)
{
    WaveFormat wave_format;
    RiffHeader riff_header;
    WaveData wave_data;

    unsigned char *data;
    unsigned int buffer;
    size_t size;
    size_t frequency;
    int format;

    std::ifstream fs;
    try {
        fs.open(path, std::ios::binary);
        if (!fs.is_open()) {
            std::cout << "could not open file\n";
            return nullptr;
        }

        fs.read((char*)&riff_header, sizeof(RiffHeader));
        if ((riff_header.chunk_id[0] != 'R' ||
            riff_header.chunk_id[1] != 'I' ||
            riff_header.chunk_id[2] != 'F' ||
            riff_header.chunk_id[3] != 'F') ||
            (riff_header.format[0] != 'W' ||
                riff_header.format[1] != 'A' ||
                riff_header.format[2] != 'V' ||
                riff_header.format[3] != 'E')) {
            std::cout << "Invalid RIFF or WAVE Header\n";
            throw std::runtime_error("Invalid RIFF or WAVE Header");
        }

        fs.read((char*)&wave_format, sizeof(WaveFormat));
        if (wave_format.sub_chunk_id[0] != 'f' ||
            wave_format.sub_chunk_id[1] != 'm' ||
            wave_format.sub_chunk_id[2] != 't' ||
            wave_format.sub_chunk_id[3] != ' ') {
            std::cout << "Invalid Wave format\n";
            throw std::runtime_error("Invalid Wave Format");
        }

        if (wave_format.sub_chunk_size > 16) {
            fs.seekg(sizeof(uint16_t));
        }


        fs.read((char*)&wave_data, sizeof(WaveData));
        if (wave_data.sub_chunk_id[0] != 'd' ||
            wave_data.sub_chunk_id[1] != 'a' ||
            wave_data.sub_chunk_id[2] != 't' ||
            wave_data.sub_chunk_id[3] != 'a') {
            std::cout << "Invalid data header\n";
            throw std::runtime_error("Invalid data header");
        }

        // Read data
        data = new unsigned char[wave_data.sub_chunk_2_size];
        fs.read((char*)&data[0], wave_data.sub_chunk_2_size);

        size = wave_data.sub_chunk_2_size;
        frequency = wave_format.sample_rate;

        if (wave_format.num_channels == 1) {
            if (wave_format.bits_per_sample == 8) {
                format = AL_FORMAT_MONO8;
            } else if (wave_format.bits_per_sample == 16) {
                format = AL_FORMAT_MONO16;
            }
        } else if (wave_format.num_channels == 2) {
            if (wave_format.bits_per_sample == 8) {
                format = AL_FORMAT_STEREO8;
            } else if (wave_format.bits_per_sample == 16) {
                format = AL_FORMAT_STEREO16;
            }
        }

        auto audio_source = std::make_shared<AudioSource>(format, data, size, frequency);

        delete[] data;
        fs.close();

        return audio_source;
    } catch (...) {
        if (fs.is_open()) {
            fs.close();
        }

        return nullptr;
    }
}
} // namespace hyperion
