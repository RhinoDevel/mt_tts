
// Marcel Timm, RhinoDevel, 2025mar09

// This is meant to be a pure-C interface.

#ifndef MT_TTS
#define MT_TTS

#ifdef _WIN32
    #define WIN32_LEAN_AND_MEAN // Excl. rarely-used stuff from Windows headers.
    
    #ifdef MT_EXPORT_TTS
        #define MT_EXPORT_TTS_API __declspec(dllexport)
    #else //MT_EXPORT_TTS
        #define MT_EXPORT_TTS_API __declspec(dllimport)
    #endif //MT_EXPORT_TTS
#else //_WIN32
    #define MT_EXPORT_TTS_API
    #ifndef __stdcall
        #define __stdcall
    #endif //__stdcall
#endif //_WIN32

#ifdef __cplusplus
    #include <cstdint>
#else //__cplusplus
    #include <stdint.h>
#endif //__cplusplus

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

MT_EXPORT_TTS_API void __stdcall mt_tts_deinit(void);

/**
 * - Just assumes that initialization was done!
 */
MT_EXPORT_TTS_API void __stdcall mt_tts_stream_to_stream_raw(
    void * const input_stream, void * const output_stream);

/** Call this to free the pointer to samples created via mt_tts_to_raw().
 */
MT_EXPORT_TTS_API void __stdcall mt_tts_free_raw(int16_t * const samples);

/**
 * - Just assumes that initialization was done!
 * - Caller takes ownership of return value, but the returned object should be
 *   freed with mt_tts_free_raw()!
 */
MT_EXPORT_TTS_API int16_t* __stdcall mt_tts_to_raw(
    char const * const text, int * const out_sample_count);

/**
 * - Just assumes that initialization was done!
 */
MT_EXPORT_TTS_API void __stdcall mt_tts_to_wav_file(
    char const * const text, char const * const wav_path);

MT_EXPORT_TTS_API void __stdcall mt_tts_reinit(
    char const * const model_onnx_path, char const * const model_json_path);

#ifdef __cplusplus
}
#endif //__cplusplus

#endif //MT_TTS
