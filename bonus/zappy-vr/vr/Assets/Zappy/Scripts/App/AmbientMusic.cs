using UnityEngine;

namespace Zappy.App
{
    /// <summary>
    /// Procedurally synthesized ambient bed (the project ships no audio assets):
    /// four soft chords crossfading over a 24-second seamless loop, sparse pentatonic
    /// plucks on top, a slow breathing LFO, and stereo width from a short Haas delay.
    /// Every frequency is snapped to a whole number of cycles per loop so the wrap
    /// point is click-free. Built once at startup into a looping AudioSource.
    /// </summary>
    public class AmbientMusic : MonoBehaviour
    {
        private const int SampleRate = 22050;
        private const float LoopSeconds = 24f;
        private const float ChordSeconds = 6f;
        private const float CrossfadeSeconds = 2.2f;
        private const float Volume = 0.22f;
        private const int HaasSamples = 400; // ~18 ms right-channel delay.

        // A-minor mood: Am, Fmaj, C, G — one low root and three upper voices each.
        private static readonly float[][] Chords =
        {
            new[] { 110.00f, 220.00f, 261.63f, 329.63f },
            new[] { 87.31f, 174.61f, 261.63f, 349.23f },
            new[] { 130.81f, 196.00f, 329.63f, 392.00f },
            new[] { 98.00f, 196.00f, 293.66f, 392.00f }
        };

        // A-minor pentatonic for the sparse melody plucks.
        private static readonly float[] Pentatonic = { 440.00f, 523.25f, 587.33f, 659.26f, 783.99f };

        private void Start()
        {
            var source = gameObject.AddComponent<AudioSource>();
            source.clip = BuildLoop();
            source.loop = true;
            source.volume = Volume;
            source.spatialBlend = 0f; // Non-positional background bed.
            source.Play();
        }

        private static AudioClip BuildLoop()
        {
            int sampleCount = (int)(SampleRate * LoopSeconds);
            var mono = new float[sampleCount];

            // Pad: each chord fades in/out around its slot; neighbours overlap.
            for (int chord = 0; chord < Chords.Length; chord++)
            {
                foreach (float voice in Chords[chord])
                {
                    double step = 2.0 * System.Math.PI * SnapToLoop(voice) / SampleRate;
                    for (int i = 0; i < sampleCount; i++)
                    {
                        float envelope = ChordEnvelope((float)i / SampleRate, chord);
                        if (envelope <= 0f)
                            continue;
                        mono[i] += 0.16f * envelope * (float)System.Math.Sin(step * i);
                    }
                }
            }

            // Plucks: one every 1.5 s, decayed well before the loop point.
            for (int note = 0; ; note++)
            {
                float start = note * 1.5f;
                if (start > LoopSeconds - 3f)
                    break;
                double step = 2.0 * System.Math.PI * SnapToLoop(Pentatonic[(note * 3) % Pentatonic.Length]) / SampleRate;
                int first = (int)(start * SampleRate);
                int span = (int)(1.4f * SampleRate);
                for (int i = 0; i < span && first + i < sampleCount; i++)
                {
                    float dt = (float)i / SampleRate;
                    float envelope = Mathf.Exp(-3.5f * dt) * Mathf.Clamp01(dt / 0.02f);
                    mono[first + i] += 0.10f * envelope * (float)System.Math.Sin(step * i);
                }
            }

            // Interleave: slow breathing (2 cycles per loop), Haas-delayed right
            // channel (loop-safe: the signal is periodic), gentle soft clip.
            var data = new float[sampleCount * 2];
            for (int i = 0; i < sampleCount; i++)
            {
                float lfo = 0.85f + 0.15f * (float)System.Math.Sin(2.0 * System.Math.PI * 2.0 * i / sampleCount);
                data[i * 2] = SoftClip(mono[i] * lfo);
                data[i * 2 + 1] = SoftClip(mono[(i + sampleCount - HaasSamples) % sampleCount] * lfo);
            }

            var clip = AudioClip.Create("ZappyAmbient", sampleCount, 2, SampleRate, false);
            clip.SetData(data, 0);
            return clip;
        }

        /// <summary>Raised-cosine window around the chord's slot, wrapping over the loop.</summary>
        private static float ChordEnvelope(float t, int chord)
        {
            float local = Mathf.Repeat(t - chord * ChordSeconds + CrossfadeSeconds, LoopSeconds);
            float active = ChordSeconds + 2f * CrossfadeSeconds;
            if (local >= active)
                return 0f;
            float fadeIn = Smooth(Mathf.Clamp01(local / (2f * CrossfadeSeconds)));
            float fadeOut = Smooth(Mathf.Clamp01((active - local) / (2f * CrossfadeSeconds)));
            return fadeIn * fadeOut;
        }

        private static float Smooth(float x) => x * x * (3f - 2f * x);

        /// <summary>Whole cycles per loop, so the waveform is continuous at the wrap.</summary>
        private static float SnapToLoop(float frequency) => Mathf.Round(frequency * LoopSeconds) / LoopSeconds;

        private static float SoftClip(float sample) => (float)System.Math.Tanh(sample);
    }
}
