#include "daisy_pod.h"
#include "daisy_seed.h"
#include "fatfs.h"
#include <string.h>
#include <math.h>

using namespace daisy;

// GLOBALS
DaisyPod       hw;
SdmmcHandler   sdmmc;
FatFSInterface fsi;
FIL            file;

DSY_SDRAM_BSS float audio_buffer[48000];
int audio_length = 0;

int playback_pos = 0;
bool playing = false;

// 🔊 Audio callback
void AudioCallback(AudioHandle::InterleavingInputBuffer  in,
                   AudioHandle::InterleavingOutputBuffer out,
                   size_t                                size)
{
    static float phase = 0.0f;
    
    for(size_t i = 0; i < size; i++)
    {
        float sample = 0.0f;

        if(playing && playback_pos < audio_length)
        {
            sample = audio_buffer[playback_position] * 5.0f;
            playback_position++;
            
            if(playback_position >= audio_length)
            {
                is_playing = false;
                playback_position = 0;
            }
        }
        else
        {
            sample = 0.05f * sinf(phase);
            phase += 2.0f * 3.14159f * 440.0f / 48000.0f;
            if(phase > 2.0f * 3.14159f) phase -= 2.0f * 3.14159f;
        }
        
        out[0][i] = sample;
        out[1][i] = sample;
    }
}

// 🔍 Find data chunk
uint32_t FindDataStart(FIL* file)
{
    FIL file;
    UINT bytes_read;
    FRESULT res;
    
    res = f_open(&file, filename, FA_READ);
    if(res != FR_OK)
    {
        audio_length = 9;
        return false;
    }

    uint8_t header[44];
    res = f_read(&file, header, 44, &bytes_read);
    if(res != FR_OK || bytes_read != 44)
    {
        f_close(&file);
        audio_length = 8;
        return false;
    }

    uint16_t num_channels    = *(uint16_t*)(header + 22);
    uint16_t bits_per_sample = *(uint16_t*)(header + 34);

    if(bits_per_sample != 16)
    {
        f_close(&file);
        audio_length = 7;
        return false;
    }

    // 🔥 Start scanning from correct position
    f_lseek(&file, 12);

    uint32_t data_size = 0;

    while(true)
    {
        uint8_t chunk_header[8];
        UINT br;

        if(f_read(&file, chunk_header, 8, &br) != FR_OK || br != 8)
        {
            f_close(&file);
            audio_length = 6;
            return false;
        }

        uint32_t chunk_size = *(uint32_t*)(chunk_header + 4);

        if(memcmp(chunk_header, "data", 4) == 0)
        {
            data_size = chunk_size;
            break;
        }

        // 🔥 Properly skip chunk (handles padding)
        f_lseek(&file, f_tell(&file) + ((chunk_size + 1) & ~1));
    }

    if(data_size == 0 || data_size > 2000000)
    {
        f_close(&file);
        audio_length = 5;
        return false;
    }

    uint32_t bytes_per_sample = bits_per_sample / 8;
    uint32_t total_samples = data_size / bytes_per_sample;

    if(num_channels == 2)
        total_samples /= 2;

    if(total_samples > 48000)
        total_samples = 48000;

    audio_length = total_samples;

    int16_t sample_buffer[256];
    uint32_t samples_read = 0;

    while(samples_read < total_samples)
    {
        uint32_t to_read = (total_samples - samples_read > 256) ? 256 : (total_samples - samples_read);

        if(num_channels == 1)
        {
            res = f_read(&file, sample_buffer, to_read * 2, &bytes_read);
            if(res != FR_OK)
            {
                f_close(&file);
                return false;
            }

            for(uint32_t i = 0; i < to_read; i++)
                audio_buffer[samples_read + i] = sample_buffer[i] / 32768.0f;
        }
        else
        {
            int16_t stereo_buffer[512];
            res = f_read(&file, stereo_buffer, to_read * 4, &bytes_read);
            if(res != FR_OK)
            {
                f_close(&file);
                return false;
            }

            for(uint32_t i = 0; i < to_read; i++)
                audio_buffer[samples_read + i] = stereo_buffer[i * 2] / 32768.0f;
        }

        samples_read += to_read;
    }

    f_close(&file);
    return (audio_length > 0);
}

int main(void)
{
    hw.Init();
    hw.StartLog();
    
    // 🔥 Confirm firmware
    for(int i = 0; i < 10; i++)
    {
        hw.SetLed(true);
        System::Delay(100);
        hw.SetLed(false);
        System::Delay(100);
    }
    
    FatFSInterface::Config sd_config;
    sd_config.media = FatFSInterface::Config::MEDIA_SD;
    fsi.Init(sd_config);
    
    f_mount(&fsi.GetSDFileSystem(), "/", 1);
    
    bool loaded = LoadWavFromSD("/CLAP1SEC.WAV");
    
    if(loaded)
    {
        // success
        for(int i = 0; i < 3; i++)
        {
            hw.SetLed(true);
            System::Delay(100);
            hw.SetLed(false);
            System::Delay(100);
        }
    }
    else
    {
        // 🔥 Blink error code
        for(int i = 0; i < audio_length; i++)
        {
            hw.SetLed(true);
            System::Delay(300);
            hw.SetLed(false);
            System::Delay(300);
        }
    }
    
    button.Init(hw.GetPin(27), GPIO::Mode::INPUT, GPIO::Pull::PULLUP);
    
    hw.SetAudioBlockSize(4);
    hw.StartAudio(AudioCallback);

    bool last = false;

    while(1)
    {
        bool button_state = button.Read();
        
        if(last_button_state && !button_state)
        {
            if(loaded)
            {
                is_playing = true;
                playback_position = 0;
            }
        }
        
        last_button_state = button_state;
        System::Delay(10);
    }
}