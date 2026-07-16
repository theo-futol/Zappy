using System;
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.Net.WebSockets;
using System.Text;
using System.Threading;
using System.Threading.Tasks;

namespace Zappy.Net
{
    /// <summary>
    /// WebSocket client for the Node bridge (bonus/GUI/bridge/bridge.js). The bridge
    /// speaks the GRAPHIC protocol to zappy_server and pushes JSON events to us; its
    /// first frame (WELCOME) carries the full game state. Frames are parsed on a
    /// background task and drained on the main thread via TryReceive.
    /// </summary>
    public sealed class BridgeClient : IDisposable
    {
        private readonly ConcurrentQueue<Dictionary<string, object>> _events = new();
        private ClientWebSocket _socket;
        private CancellationTokenSource _cancel;
        private volatile bool _connected;
        private volatile string _error;

        public bool Connected => _connected;

        /// <summary>Non-null once the connection failed or closed.</summary>
        public string Error => _error;

        public void Connect(string host, int port)
        {
            _cancel = new CancellationTokenSource();
            Task.Run(() => RunAsync(host, port, _cancel.Token));
        }

        public bool TryReceive(out Dictionary<string, object> evt) => _events.TryDequeue(out evt);

        private async Task RunAsync(string host, int port, CancellationToken token)
        {
            var buffer = new byte[64 * 1024];
            var message = new StringBuilder();
            try
            {
                _socket = new ClientWebSocket();
                await _socket.ConnectAsync(new Uri($"ws://{host}:{port}"), token);
                _connected = true;

                while (!token.IsCancellationRequested && _socket.State == WebSocketState.Open)
                {
                    message.Clear();
                    WebSocketReceiveResult result;
                    do
                    {
                        result = await _socket.ReceiveAsync(new ArraySegment<byte>(buffer), token);
                        if (result.MessageType == WebSocketMessageType.Close)
                        {
                            _error = "Bridge closed the connection";
                            return;
                        }
                        message.Append(Encoding.UTF8.GetString(buffer, 0, result.Count));
                    } while (!result.EndOfMessage);

                    if (MiniJson.Parse(message.ToString()) is Dictionary<string, object> evt)
                        _events.Enqueue(evt);
                }
            }
            catch (OperationCanceledException)
            {
                // Normal shutdown.
            }
            catch (Exception exception)
            {
                _error = exception.InnerException?.Message ?? exception.Message;
            }
            finally
            {
                _connected = false;
                _error ??= token.IsCancellationRequested ? null : "Connection lost";
            }
        }

        public void Dispose()
        {
            _cancel?.Cancel();
            try
            {
                _socket?.Abort();
                _socket?.Dispose();
            }
            catch
            {
                // The socket may already be dead; shutdown must not throw.
            }
        }
    }
}
