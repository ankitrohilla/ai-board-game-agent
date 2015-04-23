./compile.sh

if [[ $1 = '1' ]]; then
	gnome-terminal --command=python\ ./Quoridor090910/TkBoard.py
elif [[ $1 = '2' ]]; then
	gnome-terminal --command=python\ ./Quoridor111112/TkBoard.py
elif [[ $1 = '3' ]]; then
	gnome-terminal --command=python\ ./Quoridor131314/TkBoard.py
else
	echo "Chutie sai argument daal"
	exit
fi	

gnome-terminal --command=./client_ai\ 127.0.0.1\ 12345
gnome-terminal --command=./client_ai\ 127.0.0.1\ 12345
