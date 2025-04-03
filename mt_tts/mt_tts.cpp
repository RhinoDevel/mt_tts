
// Marcel Timm, RhinoDevel, 2025mar09

#include <cassert>
#include <cstdbool>
#include <cstdint>

#include "mt_tts.h"
#include "mt_config.h"

// *****************************************************************************

#include <condition_variable>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <map>
#include <mutex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#ifdef _MSC_VER
    #define NOMINMAX
    #include <windows.h>
#endif

#ifdef _WIN32
    #include <fcntl.h>
    #include <io.h>
#endif

#ifdef __APPLE__
    #include <mach-o/dyld.h>
#endif

#include "piper.hpp"

using namespace std;

// *****************************************************************************

// Initialized by mt_tts_reinit(), deinitialized by mt_tts_deinit():
//
static bool s_is_init = false;
static piper::PiperConfig * s_piper_config = nullptr;
static piper::Voice * s_piper_voice = nullptr;

// *****************************************************************************

static void rawOutputProc(
    ostream& outputStream,
    vector<int16_t>& sharedAudioBuffer,
    mutex& mutAudio,
    condition_variable& cvAudio,
    bool& audioReady,
    bool& audioFinished)
{
    vector<int16_t> internalAudioBuffer;

    while(true)
    {
        {
            unique_lock lockAudio{ mutAudio };

            cvAudio.wait(lockAudio, [&audioReady]
            {
                return audioReady;
            });

            if(sharedAudioBuffer.empty() && audioFinished)
            {
                break;
            }

            copy(
                sharedAudioBuffer.begin(),
                sharedAudioBuffer.end(),
                back_inserter(internalAudioBuffer));

            sharedAudioBuffer.clear();

            if(!audioFinished)
            {
                audioReady = false;
            }
        }

        outputStream.write(
            (char const *)internalAudioBuffer.data(),
            sizeof(int16_t) * internalAudioBuffer.size());
        outputStream.flush();
        internalAudioBuffer.clear();
    }
}

static void initialize_tts(
    mt_config const & config,
    piper::PiperConfig& piper_config,
    piper::Voice& voice)
{
    assert(!s_is_init);
    assert(s_piper_config == nullptr);
    assert(s_piper_voice == nullptr);

    optional<piper::SpeakerId> speakerId = nullopt;

    // Seconds of extra silence to insert after a single phoneme:
    //
    optional<std::map<piper::Phoneme, float>> phonemeSilenceSeconds = nullopt;

//#ifdef _WIN32
//    SetConsoleOutputCP(CP_UTF8); // Required on Windows to show IPA symbols.
//#endif

    loadVoice(
        piper_config,
        config.model_path.string(),
        config.model_config_path.string(),
        voice,
        speakerId,
        config.use_cuda);

    // Get the path to the executable so we can locate espeak-ng-data, etc. next
    // to it:
    //
#ifdef _MSC_VER
    auto exePath =
        []()
    {
        wchar_t moduleFileName[MAX_PATH] = { 0 };

        GetModuleFileNameW(nullptr, moduleFileName, std::size(moduleFileName));

        return filesystem::path(moduleFileName);
    }();
#else //_MSC_VER
    #ifdef __APPLE__
        auto exePath =
            []()
        {
            char moduleFileName[PATH_MAX] = { 0 };
            uint32_t moduleFileNameSize = std::size(moduleFileName);

            _NSGetExecutablePath(moduleFileName, &moduleFileNameSize);

            return filesystem::path(moduleFileName);
        }();
    #else //__APPLE__
        auto exePath = filesystem::canonical("/proc/self/exe");
    #endif //__APPLE__
#endif //_MSC_VER

    if(voice.phonemizeConfig.phonemeType == piper::eSpeakPhonemes)
    {
        if(config.espeak_data_path)
        {
            // User provided path:

            piper_config.eSpeakDataPath =
                config.espeak_data_path.value().string();
        }
        else
        {
            // Assume next to executable:

            piper_config.eSpeakDataPath = std::filesystem::absolute(
                exePath.parent_path().append("espeak-ng-data")).string();
        }
    }
    else
    {
        piper_config.useESpeak = false; // Not using eSpeak.
    }

    // Enable libtashkeel for Arabic, if necessary:
    //
    if(voice.phonemizeConfig.eSpeak.voice == "ar")
    {
        piper_config.useTashkeel = true;

        if(config.tashkeel_model_path)
        {
            // User provided path:
            piper_config.tashkeelModelPath =
                config.tashkeel_model_path.value().string();
        }
        else
        {
            // Assume next to executable:

            piper_config.tashkeelModelPath = std::filesystem::absolute(
                exePath.parent_path().append("libtashkeel_model.ort"))
                .string();
        }
    }

    piper::initialize(piper_config);

    if(config.noise_scale)
    {
        voice.synthesisConfig.noiseScale = config.noise_scale.value();
    }

    if(config.length_scale)
    {
        voice.synthesisConfig.lengthScale = config.length_scale.value();
    }

    if(config.noise_w)
    {
        voice.synthesisConfig.noiseW = config.noise_w.value();
    }

    if(config.sentence_silence_seconds)
    {
        voice.synthesisConfig.sentenceSilenceSeconds =
            config.sentence_silence_seconds.value();
    }

    if(phonemeSilenceSeconds)
    {
        if(!voice.synthesisConfig.phonemeSilenceSeconds)
        {
            // Overwrite:

            voice.synthesisConfig.phonemeSilenceSeconds = phonemeSilenceSeconds;
        }
        else
        {
            // Merge:

            for(const auto &[phoneme, silenceSeconds] : *phonemeSilenceSeconds)
            {
                voice.synthesisConfig.phonemeSilenceSeconds->try_emplace(
                    phoneme, silenceSeconds);
            }
        }
    }
}

// *****************************************************************************

MT_EXPORT_TTS_API void __stdcall mt_tts_deinit(void)
{
    if(!s_is_init)
    {
        assert(s_piper_config == nullptr);
        assert(s_piper_voice == nullptr);
        return; // Nothing to do.
    }

    assert(s_piper_config != nullptr);
    assert(s_piper_voice != nullptr);

    piper::terminate(*s_piper_config);

    delete s_piper_config;
    s_piper_config = nullptr;
    delete s_piper_voice;
    s_piper_voice = nullptr;
    s_is_init = false;
}

MT_EXPORT_TTS_API void __stdcall mt_tts_stream_to_stream_raw(
    void * const input_stream, void * const output_stream)
{
    assert(s_is_init);
    assert(s_piper_config != nullptr);
    assert(s_piper_voice != nullptr);

    assert(input_stream != nullptr);
    assert(output_stream != nullptr);

    istream * const in = static_cast<std::istream*>(input_stream);
    ostream * const out = static_cast<std::ostream*>(output_stream);
    string line;

    while(getline(*in, line))
    {
        piper::SynthesisResult result;
        mutex mutAudio;
        condition_variable cvAudio;
        bool audioReady = false;
        bool audioFinished = false;
        vector<int16_t> audioBuffer;
        vector<int16_t> sharedAudioBuffer;

//#ifdef _WIN32
//        // Needed on Windows to avoid terminal conversions:
//
//        _setmode(_fileno(stdout), O_BINARY);
//        _setmode(_fileno(stdin), O_BINARY);
//#endif

        thread rawOutputThread(
            rawOutputProc,
            ref(*out),
            ref(sharedAudioBuffer),
            ref(mutAudio),
            ref(cvAudio),
            ref(audioReady),
            ref(audioFinished));

        auto audioCallback =
            [
                &audioBuffer,
                &sharedAudioBuffer,
                &mutAudio,
                &cvAudio,
                &audioReady
            ]()
        {
            // Signal thread that audio is ready:
            {
                unique_lock lockAudio(mutAudio);

                copy(
                    audioBuffer.begin(),
                    audioBuffer.end(),
                    back_inserter(sharedAudioBuffer));

                audioReady = true;
                cvAudio.notify_one();
            }
        };

        piper::textToAudio(
            *s_piper_config,
            *s_piper_voice,
            line,
            audioBuffer,
            result,
            audioCallback);

        // Signal thread that there is no more audio:
        //
        {
            unique_lock lockAudio(mutAudio);

            audioReady = true;
            audioFinished = true;
            cvAudio.notify_one();
        }

        rawOutputThread.join(); // Waits for audio output to finish.
    }
}

MT_EXPORT_TTS_API void __stdcall mt_tts_free_raw(int16_t * const samples)
{
    assert(samples != nullptr);

    free(samples);
}

MT_EXPORT_TTS_API int16_t* __stdcall mt_tts_to_raw(
    char const * const text, int * const out_sample_count)
{
    assert(s_is_init);
    assert(s_piper_config != nullptr);
    assert(s_piper_voice != nullptr);

    assert(text != nullptr);
    assert(out_sample_count != nullptr);

    int16_t* ret_val = nullptr;
    piper::SynthesisResult result;
    vector<int16_t> audioBuffer;

    piper::textToAudio(
        *s_piper_config,
        *s_piper_voice,
        text,
        audioBuffer,
        result,
        nullptr);

    assert(!audioBuffer.empty());

    *out_sample_count = static_cast<int>(audioBuffer.size());

    size_t const byte_count =
        static_cast<size_t>(*out_sample_count) * sizeof * ret_val;

    ret_val = reinterpret_cast<int16_t*>(malloc(byte_count));
    if(ret_val == nullptr)
    {
        assert(false);
        return nullptr;
    }
    std::memcpy(ret_val, audioBuffer.data(), byte_count);
    return ret_val;
}

MT_EXPORT_TTS_API void __stdcall mt_tts_to_wav_file(
    char const * const text, char const * const wav_path)
{
    assert(s_is_init);
    assert(s_piper_config != nullptr);
    assert(s_piper_voice != nullptr);

    assert(text != nullptr);
    assert(wav_path != nullptr);

    piper::SynthesisResult result;
    ofstream audioFile(wav_path, ios::binary);

    piper::textToWavFile(
        *s_piper_config, *s_piper_voice, text, audioFile, result);
}

MT_EXPORT_TTS_API void __stdcall mt_tts_reinit(
    char const * const model_onnx_path, char const * const model_json_path)
{
    assert(model_onnx_path != nullptr);
    assert(model_json_path != nullptr);

    mt_config c;
    piper::PiperConfig* piper_config = nullptr;
    piper::Voice* piper_voice = nullptr;

    if(s_is_init)
    {
        mt_tts_deinit();
    }

    assert(!s_is_init);
    assert(s_piper_config == nullptr);
    assert(s_piper_voice == nullptr);

    c.model_path = filesystem::path(model_onnx_path);
    c.model_config_path = filesystem::path(model_json_path);

    // These are currently hard-coded, here:
    //
    c.use_cuda = false;
    c.noise_scale = nullopt;
    c.length_scale = nullopt;
    c.noise_w = nullopt;
    c.sentence_silence_seconds = nullopt;
    c.espeak_data_path = nullopt;
    c.tashkeel_model_path = nullopt;

    piper_config = new piper::PiperConfig();
    piper_voice = new piper::Voice();

    initialize_tts(c, *piper_config, *piper_voice);

    s_piper_config = piper_config;
    s_piper_voice = piper_voice;
    s_is_init = true;
}
