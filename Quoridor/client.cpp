#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h> 
#include <bits/stdc++.h>
#include <vector>

using namespace std;
// Complete the function below to print 1 integer which will be your next move

int N,M,K, time_left, player;

class playerPosition {
public:
    int id, row, col;
};

class tile {

public:
    int id, row, col;
    vector<int> adjList;

    tile() {}

    tile( int row, int col, vector<int> adjList ) {
        this->id = N*(row-1) + col;
        this->row = row;
        this->col = col;
        this->adjList = adjList;
        cout << "id row col -" << this->id << " " << row << " " << col << endl;
        for_each( adjList.begin(), adjList.end(), [](int i){cout<<i;});
        cout << endl;
    }

    static int getIdFromRowCol( int row, int col ) {
        return N*(row-1) + col;
    }

    static int getRowFromId( int id ) {
        return (id / N) + 1;
    }
    static int getColFromId( int id ) {
        return (id % N) + 1;
    }

}boardMap[15][15];

int main(int argc, char *argv[])
{
    srand (time(NULL));
    int sockfd = 0, n = 0;
    char recvBuff[1024];
    char sendBuff[1025];
    struct sockaddr_in serv_addr; 

    if(argc != 3)
    {
        printf("\n Usage: %s <ip of server> <port no> \n",argv[0]);
        return 1;
    } 

    if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("\n Error : Could not create socket \n");
        return 1;
    } 

    memset(&serv_addr, '0', sizeof(serv_addr)); 

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(atoi(argv[2])); 

    if(inet_pton(AF_INET, argv[1], &serv_addr.sin_addr)<=0)
    {
        printf("\n inet_pton error occured\n");
        return 1;
    } 

    if( connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
       printf("\n Error : Connect Failed \n");
       return 1;
    } 

    cout<<"Quoridor will start..."<<endl;

    memset(recvBuff, '0',sizeof(recvBuff));
    n = read(sockfd, recvBuff, sizeof(recvBuff)-1);
    recvBuff[n] = 0;
    sscanf(recvBuff, "%d %d %d %d %d", &player, &N, &M, &K, &time_left);

    cout<< "Player " << player << endl;
    cout<< "Time " << time_left << endl;
    cout<< "Board size " << N << "x" << M << " :" << K << endl;
    float TL;
    int om,oro,oco;
    int m,r,c;
	int d=3;
    char s[100];
	int x=1;

//    start the game NOW
//    our code starts here

//    initialize the adjacency list of the 9X9 graph
    for( int i = 1; i <= N; i++ ) {
        for( int j = 1; j <= M; j++ ) {
            cout << "i j - " << i << " " << j << endl;
            int myId = tile::getIdFromRowCol( i , j );
            vector<int> tempAdjList;

            if( i != 1 )
                tempAdjList.push_back( tile::getIdFromRowCol( i-1 , j ) );
            if( i != N )
                tempAdjList.push_back( tile::getIdFromRowCol( i+1 , j ) );
            if( j != 1 )
                tempAdjList.push_back( tile::getIdFromRowCol( i , j-1 ) );
            if( j != M )
                tempAdjList.push_back( tile::getIdFromRowCol( i , j+1 ) );

            boardMap[i][j] = *(new tile(i,j,tempAdjList));
        }
    }

//    both player 1 and player 2 will look at the game from the same perspective
//    there will be a few mapping for player 2 to interpret the opponent's move and my move
//    both players have to reach id 73-81

//    player 1 will have the first move
    if(player == 1)
    {
        memset(sendBuff, '0', sizeof(sendBuff)); 
        string temp;
        cin >> m >> r >> c;
        
        snprintf(sendBuff, sizeof(sendBuff), "%d %d %d", m, r , c);
        write(sockfd, sendBuff, strlen(sendBuff));

        memset(recvBuff, '0',sizeof(recvBuff));
        n = read(sockfd, recvBuff, sizeof(recvBuff)-1);
        recvBuff[n] = 0;
        sscanf(recvBuff, "%f %d", &TL, &d);
        cout << TL << " " << d << endl;
        if( d == 1 )
        {
            cout << "You win!! Yayee!! :D ";
            x=0;
        }
        else if( d == 2 )
        {
            cout << "Loser :P ";
            x=0;
        }
    }

//    play the game
    while(x)
    {
        cout << "Inside while\n";
        memset(recvBuff, '0',sizeof(recvBuff));
        n = read(sockfd, recvBuff, sizeof(recvBuff)-1);
        recvBuff[n] = 0;

//        what the opponent did is stored here
        sscanf(recvBuff, "%d %d %d %d", &om,&oro,&oco,&d);
        cout << om << " " << oro << " " << oco << " " << d << endl;

//        transformation of what the opponent did
        if( player != 1 ) {
            oro = N + 1 - oro;
            oco = M + 1 - oco;

//            if a wall has been placed, mapping is a bit changed in this case, investigate the board
            if( om != 0 ) {
                oro++;
                oco++;
            }
        }

        if( d == 1 )
        {
            cout<<"You win!! Yayee!! :D ";
            break;
        }
        else if( d == 2 )
        {
            cout<<"Loser :P ";
            break;
        }

        memset(sendBuff, '0', sizeof(sendBuff)); 
        string temp;

//        my move
        cin >> m >> r >> c;

//        transformation of what I am doing
        if( player != 1 ) {
            r = N + 1 - r;
            c = M + 1 - c;

//            if a wall has been placed, mapping is a bit changed in this case, investigate the board
            if( m != 0 ) {
                r++;
                c++;
            }
        }

        snprintf(sendBuff, sizeof(sendBuff), "%d %d %d", m, r , c);
        write(sockfd, sendBuff, strlen(sendBuff));

        memset(recvBuff, '0',sizeof(recvBuff));
        n = read(sockfd, recvBuff, sizeof(recvBuff)-1);
        recvBuff[n] = 0;

//        some unknown measure returned by the server after I played my turn
        sscanf(recvBuff, "%f %d", &TL, &d);//d=3 indicates game continues.. d=2 indicates lost game, d=1 means game won.
        cout<< TL << " " << d << endl;

        if( d == 1 )
        {
            cout<<"You win!! Yayee!! :D ";
            break;
        }
        else if( d == 2 )
        {
            cout<<"Loser :P ";
            break;
        }
    }

    cout << endl << "The End" << endl;
    return 0;
}
