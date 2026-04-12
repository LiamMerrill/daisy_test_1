#include "daisy_seed.h"
#include <string.h>

using namespace daisy;

DaisySeed hw;
FatFSInterface fsi;
GPIO button;

// Audio buffer in SDRAM
DSY_SDRAM_BSS float audio_buffer[48000];
int audio_length = 0;

int playback_position = 0;
bool is_playing = false;

void AudioCallback(AudioHandle::InputBuffer in, 
                   AudioHandle::OutputBuffer out, 
                   size_t size)
{
    for(size_t i = 0; i < size; i++)
    {
        float sample = 0.0f;
        
        if(is_playing && playback_position < audio_length)
        {
            sample = audio_buffer[playback_position];
            playback_position++;
            
            if(playback_position >= audio_length)
            {
                is_playing = false;
                playback_position = 0;
            }
        }
        
        out[0][i] = sample;
        out[1][i] = sample;
    }
}

bool LoadWavFromSD(const char* filename)
{
    FIL file;
    UINT bytes_read;
    
    // Open file
    if(f_open(&file, filename, FA_READ) != FR_OK)
    {
        return false;
    }
    
    // Read WAV header (44 bytes)
    uint8_t header[44];
    f_read(&file, header, 44, &bytes_read);
    
    // Get sample count from header
    uint32_t data_size = *(uint32_t*)(header + 40);
    uint16_t bits_per_sample = *(uint16_t*)(header + 34);
    uint16_t num_channels = *(uint16_t*)(header + 22);
    
    // Calculate number of samples
    uint32_t bytes_per_sample = bits_per_sample / 8;
    uint32_t total_samples = data_size / bytes_per_sample;
    
    // If stereo, we'll take left channel only
    if(num_channels == 2)
    {
        total_samples /= 2;
    }
    
    // Limit to buffer size
    if(total_samples > 48000)
    {
        total_samples = 48000;
    }
    
    audio_length = total_samples;
    
    // Read and convert samples
    if(bits_per_sample == 16)
    {
        int16_t sample_buffer[256];
        uint32_t samples_read = 0;
        
        while(samples_read < total_samples)
        {
            uint32_t to_read = (total_samples - samples_read > 256) ? 256 : (total_samples - samples_read);
            
            if(num_channels == 1)
            {
                // Mono - read directly
                f_read(&file, sample_buffer, to_read * 2, &bytes_read);
                
                for(uint32_t i = 0; i < to_read; i++)
                {
                    audio_buffer[samples_read + i] = sample_buffer[i] / 32768.0f;
                }
            }
            else
            {
                // Stereo - read and take left channel
                int16_t stereo_buffer[512];
                f_read(&file, stereo_buffer, to_read * 4, &bytes_read);
                
                for(uint32_t i = 0; i < to_read; i++)
                {
                    audio_buffer[samples_read + i] = stereo_buffer[i * 2] / 32768.0f;
                }
            }
            
            samples_read += to_read;
        }
    }
    
    f_close(&file);
    return true;
}

int main(void)
{
    hw.Configure();
    hw.Init();
    hw.StartLog();
    
    // Initialize SD card
    FatFSInterface::Config sd_config;
    sd_config.media = FatFSInterface::Config::MEDIA_SD;
    fsi.Init(sd_config);
    
    // Mount SD card
    f_mount(&fsi.GetSDFileSystem(), "/", 1);
    
    // Load WAV file from SD card
    bool loaded = LoadWavFromSD("/clap_1sec.wav");
    
    if(loaded)
    {
        // Blink LED to show success
        for(int i = 0; i < 3; i++)
        {
            hw.SetLed(true);
            System::Delay(100);
            hw.SetLed(false);
            System::Delay(100);
        }
    }
    
    button.Init(hw.GetPin(27), GPIO::Mode::INPUT, GPIO::Pull::PULLUP);
    
    hw.SetAudioBlockSize(4);
    hw.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_48KHZ);
    hw.StartAudio(AudioCallback);
    
    bool last_button_state = true;
    
    while(1) 
    {
        bool button_state = button.Read();
        
        if(last_button_state == true && button_state == false)
        {
            if(loaded)
            {
                is_playing = true;
                playback_position = 0;
                hw.SetLed(true);
            }
        }
        
        last_button_state = button_state;
        
        if(!is_playing)
        {
            hw.SetLed(false);
        }
        
        System::Delay(10);
    }
}