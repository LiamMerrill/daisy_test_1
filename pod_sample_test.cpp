#include "daisy_seed.h"
#include "clap_sample.h"  // Include the clapping audio

using namespace daisy;

DaisySeed hw;
GPIO button;

// Playback state
int playback_position = 0;
bool is_playing = false;

void AudioCallback(AudioHandle::InputBuffer in, 
                   AudioHandle::OutputBuffer out, 
                   size_t size)
{
    for(size_t i = 0; i < size; i++)
    {
        float sample = 0.0f;
        
        // If playing, output clap sample
        if(is_playing && playback_position < clap_sample_length)
        {
            sample = clap_sample[playback_position];
            playback_position++;
            
            // Stop when we reach the end
            if(playback_position >= clap_sample_length)
            {
                is_playing = false;
                playback_position = 0;
            }
        }
        
        // Output to both channels
        out[0][i] = sample;
        out[1][i] = sample;
    }
}

int main(void)
{
    hw.Init();
    
    // Initialize button on Pod's Button 1
    button.Init(hw.GetPin(27), GPIO::Mode::INPUT, GPIO::Pull::PULLUP);
    
    hw.SetAudioBlockSize(4);
    hw.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_48KHZ);
    hw.StartAudio(AudioCallback);
    
    bool last_button_state = true;
    
    while(1) 
    {
        // Read button (active low - pressed = false)
        bool button_state = button.Read();
        
        // Detect button press (transition from high to low)
        if(last_button_state == true && button_state == false)
        {
            // Trigger playback
            is_playing = true;
            playback_position = 0;
            hw.SetLed(true);  // LED on while playing
        }
        
        last_button_state = button_state;
        
        // Turn off LED when not playing
        if(!is_playing)
        {
            hw.SetLed(false);
        }
        
        System::Delay(10);
    }
}