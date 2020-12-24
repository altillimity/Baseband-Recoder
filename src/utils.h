#include <sstream>

template <typename T>
std::string to_string_with_precision(const T a_value, const int n = 6)
{
    std::ostringstream out;
    out.precision(n);
    out << std::fixed << a_value;
    return out.str();
}

struct WavHeader
{
    uint32_t chunkID; // == 'RIFF'
    uint32_t chunkSize;
    uint32_t format;      // == 'WAVE'
    uint32_t subchunk1ID; // == 'fmt '
    uint32_t subchunk1Size;
    uint16_t audioFormat;
    uint16_t numChannels;
    uint32_t sampleRate;
    uint32_t byteRate;
    uint16_t blockAlign;
    uint16_t bitsPerSample;
    uint32_t subchunk2ID; // == 'data'
    uint32_t subchunk2Size;
};