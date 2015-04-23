./compile.sh

if [[ $1 = '1' ]]; then
	gnome-terminal --command=python\ ./Quoridor090910/TkBoard.py
elif [[ $1 = '2' ]]; then
	gnome-terminal --command=python\ ./Quoridor111112/TkBoard.py
elif [[ $1 = '3' ]]; then
	gnome-terminal --command=python\ ./Quoridor131314/TkBoard.py
else
	echo "ullu sai argument daal"
	exit
fi	

if [[ $3 = '1' ]]; then
    echo "Chutiye game chal gayi aur tera bot pehle chalra hai"
    gnome-terminal --command=./client_ai\ 127.0.0.1\ 12345
    gnome-terminal --command=./$2\ 127.0.0.1\ 12345
elif [[ $3 = '2' ]]; then
    echo "Chutiye game chal gayi aur tera bot baad me chalra hai"
    gnome-terminal --command=./$2\ 127.0.0.1\ 12345
    gnome-terminal --command=./client_ai\ 127.0.0.1\ 12345
else
	echo "ullu ye to bata pehle kon chalega"
	exit
fi
