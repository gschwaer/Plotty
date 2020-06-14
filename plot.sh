#!/bin/bash
serial_baud=115200
plotter_tty=/dev/ttyACM0
pipe_path=/tmp/plotter_pipe


set -e

if [[ ! -e "$1" ]]; then
	echo "Usage: $0 path/to/file.gcode"
	exit 1
fi

stty -F "$plotter_tty" $serial_baud raw -echo

if [[ ! -p "$pipe_path" ]]; then
	mkfifo "$pipe_path"
fi

cat <"$plotter_tty" >"$pipe_path" &
PID=$!

trap "echo G1 F1000 > "'"'"$plotter_tty"'"'";"\
"echo G28 > "'"'"$plotter_tty"'"'";"\
"rm -f $pipe_path;"\
"kill $PID" EXIT

echo "Sending G-Code:"
while read -r line; do
	echo -n "$line ... "
	echo "$line" > "$plotter_tty"
	read response < "$pipe_path"
	response="$(echo -n $response | tr -d '\r')"
	if ! [[ "$response" == "OK" || "$response" == "K" || "$response" == "O" ]]; then
		echo
		echo "ERROR: '$response'"
		exit 1
	fi
	echo "OK"
done <<< $(cat "$1" | sed "/^;.*/d" | sed "/^[[:space:]]\+$/d" | sed "s/[[:space:]]*;.*$//" | sed "s/\r//" )

echo "Done."
