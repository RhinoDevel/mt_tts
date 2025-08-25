# mt_tts

*Marcel Timm, RhinoDevel, 2025*

**mt_tts** is a C++ library for Linux and Windows that offers a pure C interface
to the awesome text-to-speech system called
[Piper](https://github.com/rhasspy/piper) by Michael Hansen.

**mt_tts** supports:
- Text to wave file.
- Text to raw audio samples.
- Text to raw audio stream-to-stream conversion.

## How To

Clone the **mt_tts** repository:

`git clone https://github.com/RhinoDevel/mt_tts.git`

Enter the created folder:

`cd mt_tts`

Get the [Piper](https://github.com/rhasspy/piper) submodule content:

`git submodule update --init --recursive`

If you want to build [Piper](https://github.com/rhasspy/piper) in debug instead
of release mode, you need to modify the following in the file `CMakeLists.txt`:

`fmt` and `spdlog` under `target_link_libraries(` (two times) into `fmtd` and
`spdlogd`.

## Linux

No details for Linux here, yet, but you can take a look at the Windows
instructions below and at the [Makefile](./mt_tts/Makefile).

## Windows

### Build [Piper](https://github.com/rhasspy/piper)

- Open Visual Studio developer commandline.
- Go to the [Piper](https://github.com/rhasspy/piper) submodule folder:
  `cd mt_tts\piper`
- Create build folder: `mkdir build`
- Enter build folder: `cd build`
- Prepare the build: `cmake ..`
- Start the build (this will also download stuff Piper needs from the internet):
  `cmake --build . --config Release` (or `cmake --build .` for debug mode)

### Test [Piper](https://github.com/rhasspy/piper) (without mt_tts)

Copy the following (from different output directories in the build folder) into
a new folder:
- `build\pi\share\espeak-ng-data` (the whole folder)
- `build\pi\bin\espeak-ng.dll`
- `build\pi\bin\piper_phonemize.dll`
- `build\pi\lib\onnxruntime.dll`
- `build\Release\piper.exe` (or `build\Debug\piper.exe`)

Download a voice and its configuration, e.g. one for speech output in German language by [Thorsten Müller](https://github.com/thorstenMueller/Thorsten-Voice):

- [Model file (ONNX)](https://huggingface.co/rhasspy/piper-voices/resolve/v1.0.0/de/de_DE/thorsten/high/de_DE-thorsten-high.onnx?download=true)
- [Model configuration file (JSON)](https://huggingface.co/rhasspy/piper-voices/resolve/v1.0.0/de/de_DE/thorsten/high/de_DE-thorsten-high.onnx.json?download=true.json)

Store these files in the same, new folder.

Create a WAV file:

`echo "Ich bin ein Mensch, Du auch?" | piper --model de_DE-thorsten-high.onnx --config de_DE-thorsten-high.onnx.json`

Output directly to speakers with [ffmpeg](https://ffmpeg.org/)
([ffmpeg](https://ffmpeg.org/) parameters may not be optimal, in this example):

`echo "Hallo, ich bin kein Mensch, was man auch einigermaßen leicht heraushören kann, meinst Du nicht auch? Trotzdem ein tolles TTS-System!" | piper --model de_DE-thorsten-high.onnx --config de_DE-thorsten-high.onnx.json --output_raw | ffplay.exe -f s16le -ar 22050 -`

### Build mt_tts

- Open solution `mt_tts.sln` with Visual Studio (tested with 2022).
- Compile in release or debug mode.

### Test mt_tts

- Get the DLL and LIB files resulting from the build, e.g. for release mode
  `x64\Release\mt_tts.dll` and `x64\Release\mt_tts.lib`, copy them to a new
  folder.
- Also copy the file `mt_tts\mt_tts.h` to that new folder.
- Copy the following stuff from [Piper](https://github.com/rhasspy/piper) to the
  new folder, too:
  - `build\pi\share\espeak-ng-data` (the whole folder)
  - `build\pi\bin\espeak-ng.dll`
  - `build\pi\bin\piper_phonemize.dll`
  - `build\pi\lib\onnxruntime.dll`
- Also copy a [voice model file](https://huggingface.co/rhasspy/piper-voices/resolve/v1.0.0/de/de_DE/thorsten/high/de_DE-thorsten-high.onnx?download=true) and its [configuration file](https://huggingface.co/rhasspy/piper-voices/resolve/v1.0.0/de/de_DE/thorsten/high/de_DE-thorsten-high.onnx.json?download=true.json) to the same new folder.
- Open `x64 Native Tools Command Prompt for VS 2022` commandline.
- Go to the new folder and create a file `main.c` with the following code:

```
#include "mt_tts.h"

int main()
{
    mt_tts_reinit("de_DE-thorsten-high.onnx", "de_DE-thorsten-high.onnx.json");

    mt_tts_to_wav_file(
        "Hallo, nun testen wir dieses kleine Hilfsmodul.", "output.wav");

    mt_tts_deinit();

    return 0;
}
```

- Compile via `cl main.c mt_tts.lib`.
- Run `main.exe`, which will create the WAV file `output.wav`.