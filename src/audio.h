static float sound_time = 0.0f;
static int sound_active = 0;
static float audio_time = 0.0f;

void trigger_sound() {
    sound_time = 0.0f;
    sound_active = 1;
}

float generate_sound(float time) {
    if (time > 0.12f) return 0.0f;

    float env = expf(-time * 25.0f) * 0.5;
    float crack = sinf(2.0f * M_PI * 3000.0f * time) * env * 0.4f;
    float ring = sinf(2.0f * M_PI * 1200.0f * time) * env * 0.3f;
    float thunk = sinf(2.0f * M_PI * 400.0f * time) * env * 0.2f;
    float noise = ((float)rand() / RAND_MAX * 2.0f - 1.0f) * env * 0.3f;
    float impulse = time < 0.01f ? 1.0f : 0.0f;

    return (ring + thunk + noise * 0.2f) * env;
}

void game_audio(float* buffer, int frame_count) {
    const float samplePeriod = 1.0f / SAMPLE_RATE;

    for (int frameIndex = 0; frameIndex < frame_count; frameIndex++)
    {
        float amplitude = 0.0f;

        if (sound_active) {
            amplitude += generate_sound(sound_time);
            sound_time += samplePeriod;

            if (sound_time > 0.08f) {
                sound_active = 0;
            }
        }

        if (amplitude > 1.0f) amplitude = 1.0f;
        if (amplitude < -1.0f) amplitude = -1.0f;

        *buffer++ = amplitude;
        *buffer++ = amplitude;

        audio_time += samplePeriod;
        if (audio_time > 1.0f) {
            audio_time -= 1.0f;
        }
    }
}