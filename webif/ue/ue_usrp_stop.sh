PID="$1"

if [ -z "$PID" ]; then
    echo "[UE STOP] No PID provided"
    exit 1
fi

echo "[UE STOP] Stopping UE process tree with PID $PID"

# Set up sudo command - no password when called from backend
if [ "$EUID" -ne 0 ]; then
    # Check if called from backend by checking if we're in a non-interactive environment
    if [ -n "$NODE_ENV" ] || [ -t 0 ] && [ -t 1 ] && [ -t 2 ]; then
        # Interactive terminal - use password
        SUDO_CMD="echo 'fcpsutd' | sudo -S"
        echo "[UE STOP] Running as user, will use sudo with password"
    else
        # Non-interactive (likely from backend) - use non-interactive sudo
        SUDO_CMD="sudo -n"
        echo "[UE STOP] Running from backend, using non-interactive sudo"
    fi
else
    SUDO_CMD=""
    echo "[UE STOP] Running as root, no sudo needed"
fi

# First, try to send SIGINT to all children of the main process
$SUDO_CMD pkill -INT -P "$PID" 2>/dev/null
if [ $? -eq 0 ]; then
    echo "[UE STOP] Sent SIGINT to child processes of PID $PID"
else
    echo "[UE STOP] No child processes or failed to send SIGINT to children of PID $PID"
fi

# Then, send SIGINT to the main process itself
$SUDO_CMD kill -INT "$PID" 2>/dev/null
if [ $? -eq 0 ]; then
    echo "[UE STOP] Sent SIGINT to main process PID $PID"
else
    echo "[UE STOP] No such process or failed to send SIGINT to main process PID $PID"
fi

# Also, send SIGINT to any processes that might be related to this process tree
ps -ef | grep "$PID" | grep -v grep | awk '{print $2}' | xargs -r -I{} $SUDO_CMD kill -INT {} 2>/dev/null
echo "[UE STOP] Sent SIGINT to any remaining related processes for PID $PID"

# Wait up to 5 seconds for the process to terminate, then send SIGILL if still running
for i in {1..5}; do
    sleep 1
    if ! ps -p "$PID" > /dev/null 2>&1; then
        echo "[UE STOP] Process $PID has exited after SIGINT"
        exit 0
    fi
done

echo "[UE STOP] Process $PID did not exit after SIGINT, sending SIGKILL"
$SUDO_CMD kill -KILL "$PID" 2>/dev/null
if [ $? -eq 0 ]; then
    echo "[UE STOP] Sent SIGKILL to process PID $PID"
else
    echo "[UE STOP] No such process or failed to send SIGKILL to process PID $PID"
fi
