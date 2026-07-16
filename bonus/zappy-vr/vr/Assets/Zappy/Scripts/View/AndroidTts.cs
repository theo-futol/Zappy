using UnityEngine;

namespace Zappy.View
{
    /// <summary>
    /// Thin wrapper over the Android system text-to-speech engine, driven straight
    /// through JNI (no plugin). Ready only if the device ships a TTS engine — the
    /// caller must keep a fallback for when it does not (or in the editor).
    /// Utterances are queued engine-side (QUEUE_ADD), so playback is serialized.
    /// </summary>
    public sealed class AndroidTts
    {
        private const int QueueAdd = 1; // TextToSpeech.QUEUE_ADD

        private AndroidJavaObject _tts;
        private volatile bool _ready;
        private int _utterance;

        public bool Ready => _ready;

#if UNITY_ANDROID && !UNITY_EDITOR
        private class InitListener : AndroidJavaProxy
        {
            private readonly AndroidTts _owner;

            public InitListener(AndroidTts owner) : base("android.speech.tts.TextToSpeech$OnInitListener")
            {
                _owner = owner;
            }

            public void onInit(int status)
            {
                _owner._ready = status == 0; // TextToSpeech.SUCCESS
            }
        }
#endif

        public AndroidTts()
        {
#if UNITY_ANDROID && !UNITY_EDITOR
            try
            {
                using var unityPlayer = new AndroidJavaClass("com.unity3d.player.UnityPlayer");
                AndroidJavaObject activity = unityPlayer.GetStatic<AndroidJavaObject>("currentActivity");
                _tts = new AndroidJavaObject("android.speech.tts.TextToSpeech", activity, new InitListener(this));
            }
            catch (System.Exception exception)
            {
                Debug.LogWarning($"[Zappy] Android TTS unavailable: {exception.Message}");
                _tts = null;
            }
#endif
        }

        public bool Speak(string text)
        {
            if (!_ready || _tts == null)
                return false;
            try
            {
                using var parameters = new AndroidJavaObject("android.os.Bundle");
                int result = _tts.Call<int>("speak", text, QueueAdd, parameters, $"zappy{_utterance++}");
                return result == 0; // TextToSpeech.SUCCESS
            }
            catch (System.Exception exception)
            {
                Debug.LogWarning($"[Zappy] TTS speak failed: {exception.Message}");
                _ready = false;
                return false;
            }
        }

        public void Stop()
        {
            try
            {
                _tts?.Call<int>("stop");
            }
            catch
            {
                // Losing the engine mid-session must not throw.
            }
        }

        public void Dispose()
        {
            try
            {
                _tts?.Call("shutdown");
                _tts?.Dispose();
            }
            catch
            {
                // Already gone.
            }
            _tts = null;
            _ready = false;
        }
    }
}
