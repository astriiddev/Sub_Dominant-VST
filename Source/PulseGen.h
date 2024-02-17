/*
  ==============================================================================

    PulseGen.h
    Created: 12 Jan 2024 10:15:50pm
    Author:  _astriid_

  ==============================================================================
*/

#pragma once

class PulseGen
{
public:
    PulseGen() {};
    ~PulseGen() {};

    void incPulseCounter(const float* samp)
    {
        if ((*samp >= 0) == (lastSamp >= 0) || (lastSamp == 0.f && *samp == 0.f))
        {
            lastSamp = *samp;
            return;
        }

        counter ^= 1;

        if (counter) state ^= 1;

        lastSamp = *samp;
    }

    float generatePulseWave() const
    {
        return lastSamp != 0.f ? state ? 1.f : -1.f : 0.f;
    }

private:
    int counter = 0;
    int state = 1;

    float lastSamp = 0.f;
};
