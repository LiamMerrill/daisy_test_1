#include "daisy_seed.h"

using namespace daisy;

DaisySeed hw;
float volume = 1.0f;  // Start at full volume
bool audio_detected = false;

void AudioCallback(AudioHandle::InputBuffer in, 
                   AudioHandle::OutputBuffer out, 
                   size_t size)
{
    for(size_t i = 0; i < size; i++)
    {
        if(in[0][i] > 0.01f || in[0][i] < -0.01f)
        {
            audio_detected = true;
        }
        
        // Guitar passthrough with volume control
        out[0][i] = in[0][i] * volume;
        out[1][i] = in[0][i] * volume;
    }
}

int main(void)
{
    hw.Init();
    hw.SetAudioBlockSize(4);
    hw.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_48KHZ);
    hw.StartAudio(AudioCallback);
    
    while(1) 
    {
        // LED shows audio detection
        if(audio_detected)
        {
            hw.SetLed(true);
            audio_detected = false;
        }
        else
        {
            hw.SetLed(false);
        }
        
        System::Delay(10);
    }
}