#!/bin/bash

cd "$(dirname "$(readlink -f "$0")")" || exit 1

HTTP_DIR="../HTTPServer"
PID_DIR=".pids"
RESTART_DELAY=5

game_server_count() {
    local n=1
    if [ -f servers.txt ]; then
        n=$(awk -F'|' '/^[[:space:]]*add_server\|/ {sum += $4} END {print sum+0}' servers.txt)
    fi
    [ "$n" -gt 0 ] 2>/dev/null || n=1
    echo "$n"
}

is_running() {
    local f="$PID_DIR/$1.pid"
    if [ -f "$f" ]; then
        local pid
        pid=$(cat "$f")
        if [ -n "$pid" ] && kill -0 "$pid" 2>/dev/null; then
            return 0
        fi
    fi
    return 1
}

start_service() {
    local name=$1 dir=$2
    shift 2
    if is_running "$name"; then
        echo "  $name is already running"
        return
    fi

    nohup bash -c '
        dir=$1; delay=$2; name=$3; pid_dir=$4; shift 4
        cd "$dir" || exit 1
        while true; do
            echo "[$(date "+%F %T")] starting: $*"
            "$@" &
            child_pid=$!
            echo "$child_pid" > "$pid_dir/$name.pid"
            wait "$child_pid"
            code=$?
            echo "[$(date "+%F %T")] stopped (exit code $code), restarting in ${delay}s"
            sleep "$delay"
        done
    ' _ "$dir" "$RESTART_DELAY" "$name" "$PID_DIR" "$@" >/dev/null 2>&1 &

    sleep 1
    echo "  started $name"
}

stop_service() {
    local name=$1 f="$PID_DIR/$1.pid"
    [ -f "$f" ] || return
    local pid
    pid=$(cat "$f")
    if [ -n "$pid" ] && kill -0 "$pid" 2>/dev/null; then
        kill "$pid" 2>/dev/null
        for _ in $(seq 1 20); do
            kill -0 "$pid" 2>/dev/null || break
            sleep 0.5
        done
        kill -9 "$pid" 2>/dev/null
        echo "  stopped $name"
    fi
    pkill -f "_ $dir .* $name " 2>/dev/null
    rm -f "$f"
}

build_http() {
    if [ ! -f "$HTTP_DIR/main.go" ]; then
        echo "HTTPServer/main.go not found."
        return 1
    fi
    if [ ! -x "$HTTP_DIR/httpserver" ] || [ "$HTTP_DIR/main.go" -nt "$HTTP_DIR/httpserver" ]; then
        echo "  building HTTP server..."
        (cd "$HTTP_DIR" && go build -o httpserver main.go) || return 1
    fi
}

do_start() {
    local count i
    count=$(game_server_count)
    mkdir -p "$PID_DIR"

    for bin in Master GameServer; do
        if [ ! -x "./$bin" ]; then
            echo "./$bin not found. Did you run the compile script?"
            exit 1
        fi
    done
    build_http || exit 1

    echo "Starting GTopia ($count game server(s))..."
    start_service http "$HTTP_DIR" ./httpserver
    sleep 2
    start_service master . ./Master
    sleep 3
    for ((i = 1; i <= count; i++)); do
        start_service "game$i" . ./GameServer --id "$i"
        sleep 1
    done
    echo "Done. Check with 'sudo ./run_servers.sh status'."
}

do_stop() {
    echo "Stopping GTopia..."
    for f in $(ls "$PID_DIR"/game*.pid 2>/dev/null | sort -V); do
        stop_service "$(basename "$f" .pid)"
    done
    stop_service master
    stop_service http
    rmdir "$PID_DIR" 2>/dev/null
}

do_status() {
    local found=0
    for f in "$PID_DIR"/*.pid; do
        [ -e "$f" ] || continue
        found=1
        name=$(basename "$f" .pid)
        if is_running "$name"; then 
            echo "  $name: running (PID $(cat "$f"))"
        else 
            echo "  $name: NOT running"
        fi
    done
    [ $found -eq 0 ] && echo "  nothing is running"
}

cmd=${1:-start}

if [ "$EUID" -ne 0 ]; then
    echo "Please run with root: sudo ./run_servers.sh $cmd"
    exit 1
fi

case "$cmd" in
    start)   do_start ;;
    stop)    do_stop ;;
    restart) do_stop; do_start ;;
    status)  do_status ;;
    *) echo "Usage: sudo ./run_servers.sh [start|stop|restart|status]"; exit 1 ;;
esac