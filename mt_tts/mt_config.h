
// Marcel Timm, RhinoDevel, 2025mar10

#ifndef MT_CONFIG
#define MT_CONFIG

#include <filesystem>
#include <optional>

using namespace std;

struct mt_config
{
    filesystem::path model_path; // Path to ONNX voice file.
    filesystem::path model_config_path; // Path to JSON voice config file.
    bool use_cuda; // Try to use GPU via CUDA.

    // Amount of noise to add during audio generation
    // (generator noise, default: 0.667):
    //
    optional<float> noise_scale;

    // Speed of speaking / phoneme length
    // (1.0 = normal/default, < 1.0 is faster, > 1.0 is slower):
    //
    optional<float> length_scale;

    // Variation in phoneme lengths (phoneme width noise, default: 0.8):
    //
    optional<float> noise_w;

    // Seconds of silence to add after each sentence (default: 0.2):
    //
    optional<float> sentence_silence_seconds;

    // Path to espeak-ng data directory (default is next to executable):
    //
    optional<filesystem::path> espeak_data_path;

    // Path to libtashkeel ORT model
    // (see https://github.com/mush42/libtashkeel/):
    //
    optional<filesystem::path> tashkeel_model_path;
};

#endif //MT_CONFIG
