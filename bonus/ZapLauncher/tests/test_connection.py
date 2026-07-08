"""Functional tests for ServerConnection / GraphicConnection against the fake server."""

import time

import pytest

from multiagent import ServerConnection, GraphicConnection


def _connect(fake_server, team="REDS") -> ServerConnection:
    return ServerConnection("127.0.0.1", fake_server.port, team)


def test_handshake_parses_slots_and_map(fake_server):
    fake_server.start_called = True
    conn = _connect(fake_server)
    assert conn.slots == "3"
    assert conn.map_size == "10 10"
    conn.close()


def test_handshake_rejected_team_raises(fake_server):
    with pytest.raises(ConnectionError):
        _connect(fake_server, team="NOT_A_TEAM")


def test_send_command_returns_reply(fake_server):
    conn = _connect(fake_server)
    assert conn.send_command("Forward") == "ok"
    assert ("REDS", "Forward") in fake_server.received
    conn.close()


def test_broadcast_messages_are_queued_not_returned(fake_server):
    fake_server.responses["Forward"] = ["message 3, hello_there", "ok"]
    conn = _connect(fake_server)
    assert conn.send_command("Forward") == "ok"
    msgs = conn.drain_messages()
    assert msgs == ["message 3, hello there"]  # underscores decoded
    assert conn.drain_messages() == []  # drained
    conn.close()


def test_waiting_lines_are_skipped_by_send_command(fake_server):
    fake_server.responses["Look"] = ["waiting 2000", "[player, , , ]"]
    conn = _connect(fake_server)
    assert conn.send_command("Look") == "[player, , , ]"
    conn.close()


def test_dead_reply_raises_and_marks_dead(fake_server):
    fake_server.responses["Forward"] = "dead"
    conn = _connect(fake_server)
    with pytest.raises(ConnectionError):
        conn.send_command("Forward")
    assert not conn.is_alive()
    with pytest.raises(ConnectionError):
        conn.send_command("Forward")


def test_poll_waiting_detects_notification_without_consuming(fake_server):
    conn = _connect(fake_server)
    assert conn.poll_waiting() is None
    fake_server.push_to_ai("waiting 1500")
    deadline = time.time() + 2
    ms = None
    while time.time() < deadline and ms is None:
        ms = conn.poll_waiting()
        time.sleep(0.01)
    assert ms == 1500
    # poll must not consume: the line is still readable
    assert conn._recv_line() == "waiting 1500"
    conn.close()


def test_is_alive_true_on_open_connection(fake_server):
    conn = _connect(fake_server)
    assert conn.is_alive()
    conn.close()


def test_graphic_connection_team_names(fake_server):
    conn = GraphicConnection("127.0.0.1", fake_server.port)
    assert conn.get_team_names() == ["REDS"]
    conn.close()
