#include "daisy_pod.h"
#include "daisy_seed.h"
#include "fatfs.h"
#include <string.h>

using namespace daisy;

// GLOBALS
DaisyPod       hw;
SdmmcHandler   sdmmc;
FatFSInterface fsi;
FIL            file;

// 🔥 BIG BUFFER (SDRAM safe)
DSY_SDRAM_BSS float audio_buffer[48000]; // 1 sec max
int audio_length = 0;

int playback_pos = 0;
bool playing = false;

// 🔊 Audio callback
void AudioCallback(AudioHandle::InterleavingInputBuffer  in,
                   AudioHandle::InterleavingOutputBuffer out,
                   size_t                                size)
{
    for(size_t i = 0; i < size; i += 2)
    {
        float sample = 0.0f;

        if(playing && playback_pos < audio_length)
        {
            sample = audio_buffer[playback_pos++] * 2.0f;
        }

        out[i]     = sample;
        out[i + 1] = sample;
    }
}

// 🔍 Find data chunk
uint32_t FindDataStart(FIL* file)
{
    f_lseek(file, 12);

    while(1)
    {
        char chunk_id[4];
        UINT br;

        f_read(file, chunk_id, 4, &br);

        uint32_t chunk_size;
        f_read(file, &chunk_size, 4, &br);

        if(memcmp(chunk_id, "data", 4) == 0)
        {
            return f_tell(file);
        }

        // skip chunk (aligned)
        f_lseek(file, f_tell(file) + ((chunk_size + 1) & ~1));
    }
}

// 🔥 Load WAV into memory
bool LoadWav()
{
    if(f_open(&file, "TEST.WAV", FA_READ) != FR_OK)
        return false;

    uint32_t data_start = FindDataStart(&file);
    f_lseek(&file, data_start);

    int16_t temp[256];
    UINT br;
    int total = 0;

    while(total < 48000)
    {
        f_read(&file, temp, sizeof(temp), &br);

        int samples = br / 2;

        if(samples == 0)
            break;

        for(int i = 0; i < samples; i++)
        {
            audio_buffer[total++] = temp[i] / 32768.0f;
        }
    }

    audio_length = total;

    f_close(&file);
    return (audio_length > 0);
}

int main(void)
{
    hw.Init();

    // Startup blink
    for(int i = 0; i < 3; i++)
    {
        hw.seed.SetLed(true);
        System::Delay(200);
        hw.seed.SetLed(false);
        System::Delay(200);
    }

    // SD init
    SdmmcHandler::Config sd_cfg;
    sd_cfg.Defaults();
    sdmmc.Init(sd_cfg);

    fsi.Init(FatFSInterface::Config::MEDIA_SD);
    FATFS& fs = fsi.GetSDFileSystem();
    f_mount(&fs, "/", 1);

    bool loaded = LoadWav();

    if(loaded)
        hw.led1.Set(0.0f, 1.0f, 0.0f); // green
    else
        hw.led1.Set(1.0f, 0.0f, 0.0f); // red

    hw.UpdateLeds();

    // Audio
    hw.SetAudioBlockSize(4);
    hw.StartAudio(AudioCallback);

    bool last = false;

    while(1)
    {
        hw.ProcessDigitalControls();
        bool current = hw.button1.Pressed();

        // 🔥 EDGE DETECTION
        if(current && !last)
        {
            playback_pos = 0;
            playing = true;
            hw.led1.Set(0.0f, 0.0f, 1.0f);
        }

        if(!current)
        {
            playing = false;
            hw.led1.Set(0.0f, 0.0f, 0.0f);
        }

        last = current;

        hw.UpdateLeds();
        System::Delay(10);
    }
}