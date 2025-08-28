#!/bin/bash

# Kill any existing tmux session named 'webif' before starting
tmux has-session -t webif 2>/dev/null
if [ $? -eq 0 ]; then
    echo "[webif] Existing tmux session 'webif' found. Killing it..."
    tmux kill-session -t webif
fi

# Kill any running backend or frontend npm processes
echo "[webif] Checking for existing backend or frontend npm processes..."
pkill -f "npm start" 2>/dev/null
pkill -f "npm run start" 2>/dev/null
pkill -f "node app.js" 2>/dev/null

# strongSwan style debug print
echo "[webif] Starting OpenAirInterface5G Web Interface backend and frontend in tmux (vertical split)..."

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

if ! command -v tmux >/dev/null 2>&1; then
    echo "[webif] ERROR: tmux is not installed. Please install tmux."
    exit 1
fi

if ! command -v npm >/dev/null 2>&1; then
    echo "[webif] ERROR: npm is not installed. Please install Node.js and npm."
    exit 1
fi

SESSION="webif"
BACKEND_DIR="$SCRIPT_DIR/back-end"
FRONTEND_DIR="$SCRIPT_DIR/front-end"

# Kill existing session if exists
tmux has-session -t $SESSION 2>/dev/null
if [ $? -eq 0 ]; then
    echo "[webif] Existing tmux session '$SESSION' found. Killing it..."
    tmux kill-session -t $SESSION
fi

echo "[webif] Creating new tmux session '$SESSION'..."
tmux new-session -d -s $SESSION -c "$BACKEND_DIR"

# Enable mouse support in tmux session
tmux set-option -t $SESSION mouse on

# Backend pane
echo "[webif] Installing backend dependencies (if needed)..."
tmux send-keys -t $SESSION "npm install" C-m
echo "[webif] Launching backend server..."
tmux send-keys -t $SESSION "npm start" C-m

# Split window horizontally for frontend
tmux split-window -v -t $SESSION:0 -c "$FRONTEND_DIR"

# Frontend pane
echo "[webif] Installing frontend dependencies (if needed)..."
tmux send-keys -t $SESSION:0.1 "npm install" C-m
echo "[webif] Launching frontend server..."
tmux send-keys -t $SESSION:0.1 "npm start" C-m

echo "[webif] Attach to the tmux session with: tmux attach-session -t $SESSION"
tmux attach-session -t $SESSION
