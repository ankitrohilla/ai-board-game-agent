./compile.sh
gnome-terminal --command=python\ TkBoard.py
gnome-terminal --command=./client_input\ 127.0.0.1\ 12345\ <\ client_moves.txt
gnome-terminal --command=./client_ai\ 127.0.0.1\ 12345
