"""Bridge from local Python to a running Unreal Editor via Python remote execution.

Usage:
  python ue_remote.py <script.py> [--timeout 60]

Prints the remote result (captured stdout) to our stdout and exits non-zero on failure.
The remote script should print a final line 'RESULT:<json>' for structured data.
"""
import os
import sys
import time

ENGINE_PYTHON = r"C:\Program Files\Epic Games\UE_5.8\Engine\Plugins\Experimental\PythonScriptPlugin\Content\Python"
sys.path.insert(0, ENGINE_PYTHON)

import remote_execution as rex  # noqa: E402


def connect(discovery_timeout=15.0):
    config = rex.RemoteExecutionConfig()
    # Defaults: multicast group 239.0.0.1:6766, command port 6776.
    remote = rex.RemoteExecution(config)
    remote.start()
    deadline = time.time() + discovery_timeout
    node = None
    while time.time() < deadline:
        nodes = remote.remote_nodes
        if nodes:
            node = nodes[0]
            break
        time.sleep(0.5)
    if node is None:
        remote.stop()
        raise RuntimeError("No remote Unreal Editor node discovered. Is the editor running with Python remote execution enabled?")
    remote.open_command_connection(node["node_id"])
    return remote


def main():
    if len(sys.argv) < 2:
        print("usage: ue_remote.py <script.py> [--timeout N]")
        return 2
    script = os.path.abspath(sys.argv[1])
    timeout = 120.0
    if "--timeout" in sys.argv:
        timeout = float(sys.argv[sys.argv.index("--timeout") + 1])
    remote = connect()
    try:
        result = remote.run_command(script, unattended=True, exec_mode=rex.MODE_EXEC_FILE)
        print(result)
        return 0
    finally:
        try:
            remote.close_command_connection()
        finally:
            remote.stop()


if __name__ == "__main__":
    sys.exit(main())
