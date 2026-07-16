using System;
using System.Collections.Concurrent;
using System.IO;
using System.Net.Sockets;
using System.Text;
using System.Threading;

namespace Zappy.Net
{
    /// <summary>
    /// Raw TCP client for one manually driven trantorian — exactly what netcat would
    /// do: WELCOME handshake, team name out, then newline commands out and server
    /// lines in. Lines are read on a background thread and drained on the main
    /// thread via TryReceive.
    /// </summary>
    public sealed class PlayerClient : IDisposable
    {
        public enum Phase
        {
            Connecting,
            Joined,
            Refused,
            Dead,
            Lost
        }

        private readonly ConcurrentQueue<string> _lines = new();
        private TcpClient _socket;
        private NetworkStream _stream;
        private volatile Phase _phase = Phase.Connecting;
        private volatile string _error;
        private string _team;

        public Phase State => _phase;

        public string Error => _error;

        public string Team => _team;

        public void Connect(string host, int port, string team)
        {
            _team = team;
            var reader = new Thread(() => Run(host, port)) { IsBackground = true };
            reader.Start();
        }

        public bool TryReceive(out string line) => _lines.TryDequeue(out line);

        /// <summary>Send one AI command (no newline needed).</summary>
        public void Send(string command)
        {
            try
            {
                byte[] payload = Encoding.ASCII.GetBytes(command + "\n");
                _stream.Write(payload, 0, payload.Length);
            }
            catch (Exception exception)
            {
                _error = exception.Message;
                _phase = Phase.Lost;
            }
        }

        private void Run(string host, int port)
        {
            try
            {
                _socket = new TcpClient();
                _socket.Connect(host, port);
                _stream = _socket.GetStream();
                var reader = new StreamReader(_stream, Encoding.ASCII);
                bool welcomed = false;
                int handshakeLines = 0;
                string line;
                while ((line = reader.ReadLine()) != null)
                {
                    if (!welcomed)
                    {
                        if (line == "WELCOME")
                        {
                            welcomed = true;
                            Send(_team);
                        }
                        continue;
                    }
                    if (_phase == Phase.Connecting)
                    {
                        if (line == "ko")
                        {
                            _error = "Team refused (no slot or no egg)";
                            _phase = Phase.Refused;
                            return;
                        }
                        // Accepted: "<free slots>" then "<X Y>".
                        if (++handshakeLines == 2)
                            _phase = Phase.Joined;
                        continue;
                    }
                    _lines.Enqueue(line);
                    if (line == "dead")
                    {
                        _phase = Phase.Dead;
                        return;
                    }
                }
                if (_phase == Phase.Joined)
                {
                    _error = "Server closed the connection";
                    _phase = Phase.Lost;
                }
            }
            catch (Exception exception)
            {
                if (_phase == Phase.Connecting || _phase == Phase.Joined)
                {
                    _error = exception.Message;
                    _phase = Phase.Lost;
                }
            }
        }

        public void Dispose()
        {
            try
            {
                _socket?.Close();
            }
            catch
            {
                // Already dead; shutdown must not throw.
            }
        }
    }
}
