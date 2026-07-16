using System.Collections.Generic;
using UnityEngine;
using Zappy.Model;

namespace Zappy.View
{
    /// <summary>
    /// Gives nearby broadcasts a voice: when the sender is within earshot (4 tiles on
    /// the wrapped map) of the player the user inhabits or follows, the exact message
    /// is spoken aloud through the Android system text-to-speech engine. If the
    /// device has no TTS engine (or in the editor), it falls back to procedural robot
    /// chatter. Either way utterances are queued — never spoken over each other —
    /// and the HUD's VOICE button mutes everything.
    /// </summary>
    public class BroadcastVoice : MonoBehaviour
    {
        private const int EarshotTiles = 4;
        private const int MaxQueued = 4;
        private const int MaxSpokenLength = 120;
        private const int SampleRate = 22050;
        private const float Volume = 0.5f;

        /// <summary>HUD toggle: false mutes and drops everything.</summary>
        public bool Listening { get; private set; } = true;

        private AndroidTts _tts;

        public void SetListening(bool listening)
        {
            Listening = listening;
            if (listening)
                return;
            _queue.Clear();
            if (_source != null && _source.isPlaying)
                _source.Stop();
            _tts?.Stop();
        }

        /// <summary>Whose ears are used: manual player, else followed player, else -1 (deaf).</summary>
        public int ListenerPlayer = -1;

        private GameState _state;
        private AudioSource _source;
        private readonly Queue<AudioClip> _queue = new();
        private long _lastSequence = -1;
        private bool _primed;

        public void Bind(GameState state)
        {
            _state = state;
        }

        private void Start()
        {
            _source = gameObject.AddComponent<AudioSource>();
            _source.loop = false;
            _source.volume = Volume;
            _source.spatialBlend = 0f;
            _tts = new AndroidTts();
        }

        private void Update()
        {
            if (_state == null || _source == null)
                return;
            if (!_primed)
            {
                // Swallow broadcasts that happened before we joined.
                foreach (BroadcastEntry broadcast in _state.Broadcasts)
                    _lastSequence = broadcast.Sequence;
                _primed = true;
            }
            foreach (BroadcastEntry broadcast in _state.Broadcasts)
            {
                if (broadcast.Sequence <= _lastSequence)
                    continue;
                _lastSequence = broadcast.Sequence;
                ConsiderVoicing(broadcast);
            }

            // One utterance at a time; the finished clip is destroyed, not leaked.
            if (!_source.isPlaying && _queue.Count > 0)
            {
                if (_source.clip != null)
                    Destroy(_source.clip);
                _source.clip = _queue.Dequeue();
                _source.Play();
            }
        }

        private void ConsiderVoicing(BroadcastEntry broadcast)
        {
            if (!Listening || ListenerPlayer < 0 || _queue.Count >= MaxQueued)
                return;
            if (!_state.Players.TryGetValue(broadcast.PlayerId, out PlayerData sender)
                || !_state.Players.TryGetValue(ListenerPlayer, out PlayerData listener))
                return;
            if (TorusDistance(sender, listener) > EarshotTiles)
                return;
            // Speak the exact message; the AI's token separators become pauses so the
            // engine reads words instead of punctuation soup.
            if (_tts != null && _tts.Ready && _tts.Speak(Speakable(broadcast.Message)))
                return; // The engine queues utterances itself.
            _queue.Enqueue(BuildUtterance(broadcast.Message, broadcast.PlayerId));
        }

        private static string Speakable(string message)
        {
            string text = message.Replace('|', ' ').Replace('+', ' ').Replace(':', ' ').Replace('_', ' ');
            return text.Length > MaxSpokenLength ? text.Substring(0, MaxSpokenLength) : text;
        }

        /// <summary>Chebyshev distance on the wrapped (toroidal) map.</summary>
        private int TorusDistance(PlayerData a, PlayerData b)
        {
            int dx = Mathf.Abs(a.X - b.X);
            int dy = Mathf.Abs(a.Y - b.Y);
            if (_state.Width > 0)
                dx = Mathf.Min(dx, _state.Width - dx);
            if (_state.Height > 0)
                dy = Mathf.Min(dy, _state.Height - dy);
            return Mathf.Max(dx, dy);
        }

        private void OnDestroy()
        {
            _tts?.Dispose();
        }

        /// <summary>One chirp per word, melody from the word hashes, pitch from the sender.</summary>
        private static AudioClip BuildUtterance(string message, int senderId)
        {
            string[] words = message.Split(' ', System.StringSplitOptions.RemoveEmptyEntries);
            int syllables = Mathf.Clamp(words.Length, 2, 12);
            const float syllableSeconds = 0.11f;
            const float gapSeconds = 0.035f;
            int total = (int)((syllableSeconds + gapSeconds) * syllables * SampleRate) + SampleRate / 10;
            var data = new float[total];

            float baseFrequency = 250f + (senderId * 37 % 6) * 35f;
            for (int s = 0; s < syllables; s++)
            {
                int hash = 0;
                foreach (char c in words[s % words.Length])
                    hash = (hash * 31 + c) & 0x7fffffff;
                float frequency = baseFrequency * (1f + ((hash + s) % 7 - 3) * 0.07f);
                int start = (int)(s * (syllableSeconds + gapSeconds) * SampleRate);
                int span = (int)(syllableSeconds * SampleRate);
                for (int i = 0; i < span && start + i < total; i++)
                {
                    float t = (float)i / SampleRate;
                    float envelope = Mathf.Clamp01(t / 0.012f) * Mathf.Exp(-6f * t);
                    float vibrato = 1f + 0.02f * Mathf.Sin(2f * Mathf.PI * 24f * t);
                    double phase = 2.0 * System.Math.PI * frequency * vibrato * t;
                    float sample = (float)(System.Math.Sin(phase) + 0.35 * System.Math.Sin(2.0 * phase));
                    data[start + i] += 0.55f * envelope * sample;
                }
            }
            var clip = AudioClip.Create("Utterance", total, 1, SampleRate, false);
            clip.SetData(data, 0);
            return clip;
        }
    }
}
