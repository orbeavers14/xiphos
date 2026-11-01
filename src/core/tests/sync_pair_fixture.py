# SPDX-License-Identifier: GPL-2.0-or-later
"""Two independent FIFO workers and real BibleSync SDKs; loopback relay only.

This proves protocol/application behavior, not multicast discovery or LAN routing.
All module roots are empty disposable profiles; no Bible text is needed.
"""
import pathlib
import selectors
import socket
import subprocess
import sys
import tempfile
import threading
import time


def check(condition, message):
    if not condition:
        raise RuntimeError(message)


def main():
    executable, parent = sys.argv[1:]
    availability = subprocess.run([executable, "--available"], timeout=8, check=False)
    if availability.returncode == 77:
        print("Safe BibleSync SDK disabled; two-client networking fixture skipped")
        return 77
    check(availability.returncode == 0, "Client availability probe")
    pathlib.Path(parent).mkdir(parents=True, exist_ok=True)
    clients = []
    stopped = threading.Event()
    relay = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    relay.bind(("127.0.0.1", 0))
    relay.settimeout(0.1)
    # Keep both reservations until the ports have been selected, then hand them
    # to the child sockets. Bind failures fail the fixture rather than sharing a
    # real user's port. The SDK's fixed port is never bound in this fixture.
    reservations = [socket.socket(socket.AF_INET, socket.SOCK_DGRAM) for _ in range(2)]
    for reservation in reservations:
        reservation.bind(("127.0.0.1", 0))
    ports = [reservation.getsockname()[1] for reservation in reservations]

    def forward():
        while not stopped.is_set():
            try:
                packet, _ = relay.recvfrom(65536)
                for port in ports:
                    relay.sendto(packet, ("127.0.0.1", port))
            except socket.timeout:
                continue
            except OSError:
                return

    def read(client):
        with selectors.DefaultSelector() as selector:
            selector.register(client.stdout, selectors.EVENT_READ)
            check(selector.select(8), "Client response deadline")
        result = client.stdout.readline().strip()
        check(result, "Client exited before response")
        return result

    def command(index, text):
        client = clients[index]
        client.stdin.write(text + "\n")
        client.stdin.flush()
        return read(client)

    def configure(index, mode, listen=2, phrase="Original-session"):
        check(command(index, f"configure {mode} {listen} {phrase}") == "OK", "Configure client")

    def settle():
        result = []
        for _ in range(11):
            time.sleep(1.06)
            result = [command(index, "poll") for index in range(2)]
        return result

    def passage(sender, receiver, reference, expected=True):
        check(command(sender, "send " + reference) == "SENT 1", "Sender transmits")
        time.sleep(1.06)
        output = command(receiver, "poll")
        check(("NAV:" + reference + ":2" in output) == expected,
              "Unknown module remains reviewable, or filtered: " + output)

    thread = threading.Thread(target=forward, daemon=True)
    try:
        with tempfile.TemporaryDirectory(prefix="sync-pair-", dir=parent) as folder:
            for index, reservation in enumerate(reservations):
                root = pathlib.Path(folder) / str(index)
                (root / "mods.d").mkdir(parents=True)
                reservation.close()
                client = subprocess.Popen([executable, str(root), str(ports[index]),
                    str(relay.getsockname()[1]), "Original-client-" + str(index)],
                    stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                    text=True, bufsize=1)
                clients.append(client)
                ready = read(client)
                if ready == "UNAVAILABLE":
                    return 77
                check(ready == "READY", "Client startup")
            thread.start()
            configure(0, 1)
            configure(1, 1)
            settle()
            passage(0, 1, "John.3.16")
            passage(1, 0, "John.3.17")
            check(command(1, "echo John.3.18") == "SENT 0", "Received navigation never retransmits")
            configure(0, 2)  # Speaker can transmit; audience cannot.
            configure(1, 3)
            settle()
            passage(0, 1, "John.3.19")
            check(command(1, "send John.3.20") == "SENT 0", "Audience cannot broadcast")
            configure(1, 3, phrase="Different-session")
            settle()
            passage(0, 1, "John.3.21", False)
            configure(1, 3, listen=0)
            settle()
            passage(0, 1, "John.3.22", False)
            configure(1, 0)
            check(command(1, "poll").startswith("POLL 0"), "Disconnect is explicit")
            configure(1, 3)
            settle()
            passage(0, 1, "John.3.23")
            check(command(0, "fail") == "OK", "Inject send failure")
            check(command(0, "send John.3.24") == "ERROR", "Send failure is reported")
            check(command(0, "poll").startswith("POLL 0"), "Failed conduit stops")
            check(command(0, "repair") == "OK", "Repair simulated interface")
            configure(0, 2)
            settle()
            passage(0, 1, "John.3.25")
            for index, client in enumerate(clients):
                check(command(index, "quit") == "BYE", "Clean disconnect")
                check(client.wait(timeout=8) == 0, "Client shutdown")
            print("Two-client real-SDK loopback sync, modes, isolation, reconnect and failure checks passed")
    finally:
        stopped.set()
        if thread.is_alive():
            thread.join(timeout=2)
        relay.close()
        for reservation in reservations:
            reservation.close()
        for client in clients:
            if client.poll() is None:
                client.kill()
            client.wait(timeout=8)
            client.stdin.close()
            client.stdout.close()
            client.stderr.close()
    return 0


if __name__ == "__main__":
    sys.exit(main())
