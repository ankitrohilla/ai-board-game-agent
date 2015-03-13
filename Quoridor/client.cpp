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
#include <queue>

enum who {
    me,
    op,
};

using namespace std;
// Complete the function below to print 1 integer which will be your next move

int N,M,K, time_left, player;

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
    }

    static int getIdFromRowCol( int row, int col ) {
        return N*(row-1) + col;
    }

    static int getRowFromId( int id ) {
        return ((id-1) / N) + 1;
    }
    static int getColFromId( int id ) {
        return ((id-1) % N) + 1;
    }

};

class playerPosition {
public:
    int id, row, col;
    int oldId, oldRow, oldCol;

    who whoMI;

    tile boardMap[15][15];

//    this vector will containt shortest path to goal where goal is the Nth row
    vector<int> pathToGoal;
    vector<int> goalTiles;

    playerPosition() {}

    playerPosition( int row, int col, bool isEnemy ) {
        this->id = tile::getIdFromRowCol( row, col );
        this->row = row;
        this->row = col;

        if( !isEnemy ) {
            this->whoMI = me;
            for( int i = 1; i <= M; i++ )
                goalTiles.push_back( tile::getIdFromRowCol(N,i) );
        } else {
            this->whoMI = op;
            for( int i = 1; i <= M; i++ )
                goalTiles.push_back( tile::getIdFromRowCol(1,i) );
        }

//        initialize the adjacency list of the NXM graph
        for( int i = 1; i <= N; i++ ) {
            for( int j = 1; j <= M; j++ ) {
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

//        printAdjList();

        update( 0, row, col, whoMI );

    }

//    uses BFS to find the path to goal
    int findShortestPath() {
        cout << "According to " << whoMI+1 << " :-\n";
        queue<int> q;
        q.push( id );

        int current = q.front();

        vector<int> currentAdjList;
        vector<bool> visited( N*M, false);
        vector<int> parent( N*M, 0);

        visited[current] = true;

        while( !q.empty() ) {
            q.pop();

            currentAdjList = boardMap[ tile::getRowFromId(current) ][ tile::getColFromId(current) ].adjList;

//            cout << "id and adjList -> " << current << " and ";
//            for_each( currentAdjList.begin(), currentAdjList.end(), [](int i){cout<<i<<" ";});
//            cout << endl;

//            add all unvisited vertices to the queue
            for_each( currentAdjList.begin(), currentAdjList.end(), [&](int i) {
                if( !visited[i] ) {
                    q.push(i);
                    visited[i] = true;
                    parent[i] = current;
//                    cout << "Parent of " << i << " is " << current << endl;
                }
            });

            if( q.empty() )
                break;

            current = q.front();

//            if the new current is goal, stop here and find the path using parent vector
            if( find( goalTiles.begin(), goalTiles.end(), current ) != goalTiles.end() ) {
                int tileParent;
                pathToGoal.clear();
                pathToGoal.push_back( current );

                while( pathToGoal.back() != id ) {
                    tileParent = parent[ pathToGoal.back() ];
                    pathToGoal.push_back( tileParent );
                }

                reverse( pathToGoal.begin(), pathToGoal.end() );

                return pathToGoal.size();
            }
        }

    }

    void updateMap( int m, int r, int c, who whoCalled );

    void updatePosition( int row, int col ) {

        this->oldId = id;
        this->oldRow = this->row;
        this->oldCol = this->col;

        this->id = tile::getIdFromRowCol( row, col );
        this->row = row;
        this->col = col;

    }

//    the order for this function is very important, first positions are updated, then graph of map is updated, then shortest path is obtained
//    using the updated graph of map
    void update( int m, int r, int c, who whoCalled ) {

        if( m == 0 && whoMI == whoCalled )
            updatePosition( r, c );

        updateMap( m, r, c, whoCalled );
        findShortestPath();
        cout << (int)whoMI+1 << "'s shortest path - ";
        printPath();
    }

    void printPath() {
        for_each( pathToGoal.begin(), pathToGoal.end(), [](int i){ cout<<i<<" "; });
        cout << endl;
    }

    void printAdjList() {
//        initialize the adjacency list of the NXM graph
        for( int i = 1; i <= N; i++ ) {
            for( int j = 1; j <= M; j++ ) {

                cout << "id i j -> " << tile::getIdFromRowCol(i,j) << " " << i << " " << j << " -> ";

                for_each( boardMap[i][j].adjList.begin(), boardMap[i][j].adjList.end(), [](int i){cout<<i<<" ";});
                cout << endl;

            }
        }

    }

}myPosition, opPosition;

void playerPosition::updateMap( int m, int r, int c, who whoCalled ) {

//        me will update the map if op moved to take into account the tricky moves that me can make
//        AND VICE VERSA
    if( m == 0 && whoCalled != whoMI ) {

//            all ids and rows and cols here belong to whoCalled and NOT whoMI
        int id, row, col;
        int oldId, oldRow, oldCol;
        if( whoCalled == me ) {
            id = myPosition.id;
            row = myPosition.row;
            col = myPosition.col;
            oldId = myPosition.oldId;
            oldRow = myPosition.oldRow;
            oldCol = myPosition.oldCol;
        } else {
            id = opPosition.id;
            row = opPosition.row;
            col = opPosition.col;
            oldId = opPosition.oldId;
            oldRow = opPosition.oldRow;
            oldCol = opPosition.oldCol;
        }

        vector<int> oldPosAdjList = boardMap[ oldRow ][ oldCol ].adjList;
        vector<int> newPosAdjList = boardMap[ row ][ col ].adjList;

        bool pushed;
        vector<int> temp;

//            top
        if( oldRow > 1 ) {
            vector<int> oldPosTopAdjList = boardMap[ oldRow-1 ][ oldCol ].adjList;

            for_each( oldPosTopAdjList.begin(), oldPosTopAdjList.end(), [&](int i){

                if( (i == oldId+M || i == oldId-1 || i == oldId+1) && !pushed ) {
                    temp.push_back( oldId );
                    pushed = true;
                } else
                    temp.push_back(i);

            });
            pushed = false;
            boardMap[ oldRow-1 ][ oldCol ].adjList = temp;
            temp.clear();
        }
        if( row > 1 ) {
            vector<int> newPosTopAdjList = boardMap[ row-1 ][ col ].adjList;

            for_each( newPosTopAdjList.begin(), newPosTopAdjList.end(), [&](int i){

                if( i == id ) {

//                        if current location has no wall below or current location is not at Nth row
//                        else if current location has no wall to left or current location is not at 1st col
//                             if current location has no wall to righ or current location is not at Mth col
                    if( find( newPosAdjList.begin(), newPosAdjList.end(), id+M ) != newPosAdjList.end() ) {
                        temp.push_back( id+M );
                    } else {
                        if( find( newPosAdjList.begin(), newPosAdjList.end(), id-1 ) != newPosAdjList.end() )
                            temp.push_back( id-1 );
                        if( find( newPosAdjList.begin(), newPosAdjList.end(), id+1 ) != newPosAdjList.end() )
                            temp.push_back( id+1 );
                    }

                } else
                    temp.push_back(i);

            });
            boardMap[ row-1 ][ col ].adjList = temp;
            temp.clear();
        }

//            lef
        if( oldCol > 1 ) {
            vector<int> oldPosLefAdjList = boardMap[ oldRow ][ oldCol-1l ].adjList;

            for_each( oldPosLefAdjList.begin(), oldPosLefAdjList.end(), [&](int i){

                if( (i == oldId+1 || i == oldId-M || i == oldId+M) && !pushed ) {
                    temp.push_back( oldId );
                    pushed = true;
                } else
                    temp.push_back(i);

            });
            pushed = false;
            boardMap[ oldRow ][ oldCol-1 ].adjList = temp;
            temp.clear();
        }
        if( col > 1 ) {
            vector<int> newPosLefAdjList = boardMap[ row ][ col-1 ].adjList;

            for_each( newPosLefAdjList.begin(), newPosLefAdjList.end(), [&](int i){

                if( i == id ) {

//                        if current location has no wall to righ or current location is not at Mth col
//                        else if current location has no wall above or current location is not at 1st row
//                             if current location has no wall below or current location is not at Nth row
                    if( find( newPosAdjList.begin(), newPosAdjList.end(), id+1 ) != newPosAdjList.end() ) {
                        temp.push_back( id+1 );
                    } else {
                        if( find( newPosAdjList.begin(), newPosAdjList.end(), id-M ) != newPosAdjList.end() )
                            temp.push_back( id-M );
                        if( find( newPosAdjList.begin(), newPosAdjList.end(), id+M ) != newPosAdjList.end() )
                            temp.push_back( id+M );
                    }

                } else
                    temp.push_back(i);

            });
            boardMap[ row ][ col-1 ].adjList = temp;
            temp.clear();
        }

//            rig
        if( oldCol < M ) {
            vector<int> oldPosRigAdjList = boardMap[ oldRow ][ oldCol+1 ].adjList;

            for_each( oldPosRigAdjList.begin(), oldPosRigAdjList.end(), [&](int i){

                if( (i == oldId-1 || i == oldId-M || i == oldId+M) && !pushed ) {
                    temp.push_back( oldId );
                    pushed = true;
                } else
                    temp.push_back(i);

            });
            pushed = false;
            boardMap[ oldRow ][ oldCol+1 ].adjList = temp;
            temp.clear();
        }
        if( col < M ) {
            vector<int> newPosRigAdjList = boardMap[ row ][ col+1 ].adjList;

            for_each( newPosRigAdjList.begin(), newPosRigAdjList.end(), [&](int i){

                if( i == id ) {

//                        if current location has no wall to left or current location is not at 1st col
//                        else if current location has no wall above or current location is not at 1st row
//                             if current location has no wall below or current location is not at Nth row
                    if( find( newPosAdjList.begin(), newPosAdjList.end(), id-1 ) != newPosAdjList.end() ) {
                        temp.push_back( id-1 );
                    } else {
                        if( find( newPosAdjList.begin(), newPosAdjList.end(), id-M ) != newPosAdjList.end() )
                            temp.push_back( id-M );
                        if( find( newPosAdjList.begin(), newPosAdjList.end(), id+M ) != newPosAdjList.end() )
                            temp.push_back( id+M );
                    }

                } else
                    temp.push_back(i);

            });
            boardMap[ row ][ col+1 ].adjList = temp;
            temp.clear();
        }

//            bot
        if( oldRow < N ) {
            vector<int> oldPosBotAdjList = boardMap[ oldRow+1 ][ oldCol ].adjList;

            for_each( oldPosBotAdjList.begin(), oldPosBotAdjList.end(), [&](int i){

                if( (i == oldId-M || i == oldId-1 || i == oldId+1) && !pushed ) {
                    temp.push_back( oldId );
                    pushed = true;
                } else
                    temp.push_back(i);

            });
            pushed = false;
            boardMap[ oldRow+1 ][ oldCol ].adjList = temp;
            temp.clear();
        }
        if( row < N ) {
            vector<int> newPosBotAdjList = boardMap[ row+1 ][ col ].adjList;

            for_each( newPosBotAdjList.begin(), newPosBotAdjList.end(), [&](int i){

                if( i == id ) {

//                        if current location has no wall above or current location is not at 1st row
//                        else if current location has no wall to left or current location is not at 1st col
//                             if current location has no wall to righ or current location is not at Mth col
                    if( find( newPosAdjList.begin(), newPosAdjList.end(), id-M ) != newPosAdjList.end() ) {
                        temp.push_back( id-M );
                    } else {
                        if( find( newPosAdjList.begin(), newPosAdjList.end(), id-1 ) != newPosAdjList.end() )
                            temp.push_back( id-1 );
                        if( find( newPosAdjList.begin(), newPosAdjList.end(), id+1 ) != newPosAdjList.end() )
                            temp.push_back( id+1 );
                    }

                } else
                    temp.push_back(i);

            });
            boardMap[ row+1 ][ col ].adjList = temp;
            temp.clear();
        }


    } else if( m == 1 ) {

        vector<int> temp;

//        top left
        for_each( boardMap[r-1][c-1].adjList.begin(), boardMap[r-1][c-1].adjList.end(), [&](int i) {
//            if any of the 4 possibility occurs, don't push it because of the wall in between
            if( i != boardMap[r-1][c-1].id+M && i != boardMap[r-1][c-1].id+M+M && i != boardMap[r-1][c-1].id+M-1 && i != boardMap[r-1][c-1].id+M+1 )
                temp.push_back(i);
        });
        boardMap[r-1][c-1].adjList = temp;
        temp.clear();

//        top right
        for_each( boardMap[r-1][c].adjList.begin(), boardMap[r-1][c].adjList.end(), [&](int i) {
//            if any of the 4 possibility occurs, don't push it because of the wall in between
            if( i != boardMap[r-1][c].id+M && i != boardMap[r-1][c].id+M+M && i != boardMap[r-1][c].id+M-1 && i != boardMap[r-1][c].id+M+1 )
                temp.push_back(i);
        });
        boardMap[r-1][c].adjList = temp;
        temp.clear();

//        bottom left
        for_each( boardMap[r][c-1].adjList.begin(), boardMap[r][c-1].adjList.end(), [&](int i) {
//            if any of the 4 possibility occurs, don't push it because of the wall in between
            if( i != boardMap[r][c-1].id-M && i != boardMap[r][c-1].id-M-M && i != boardMap[r][c-1].id-M-1 && i != boardMap[r][c-1].id-M+1 )
                temp.push_back(i);
        });
        boardMap[r][c-1].adjList = temp;
        temp.clear();

//        bottom right
        for_each( boardMap[r][c].adjList.begin(), boardMap[r][c].adjList.end(), [&](int i) {
//            if any of the 4 possibility occurs, don't push it because of the wall in between
            if( i != boardMap[r][c].id-M && i != boardMap[r][c].id-M-M && i != boardMap[r][c].id-M-1 && i != boardMap[r][c].id-M+1 )
                temp.push_back(i);
        });
        boardMap[r][c].adjList = temp;

    } else if( m == 2 ) {

        vector<int> temp;

//            top left
        for_each( boardMap[r-1][c-1].adjList.begin(), boardMap[r-1][c-1].adjList.end(), [&](int i) {
//            if any of the 4 possibility occurs, don't push it because of the wall in between
            if( i != boardMap[r-1][c-1].id+1 && i != boardMap[r-1][c-1].id+1+1 && i != boardMap[r-1][c-1].id+1-M && i != boardMap[r-1][c-1].id+1+M )
                temp.push_back(i);
        });
        boardMap[r-1][c-1].adjList = temp;
        temp.clear();

//            bottom left
        for_each( boardMap[r][c-1].adjList.begin(), boardMap[r][c-1].adjList.end(), [&](int i) {
//            if any of the 4 possibility occurs, don't push it because of the wall in between
            if( i != boardMap[r][c-1].id+1 && i != boardMap[r][c-1].id+1+1 && i != boardMap[r][c-1].id+1-M && i != boardMap[r][c-1].id+1+M )
                temp.push_back(i);
        });
        boardMap[r][c-1].adjList = temp;
        temp.clear();

//            top right
        for_each( boardMap[r-1][c].adjList.begin(), boardMap[r-1][c].adjList.end(), [&](int i) {
//            if any of the 4 possibility occurs, don't push it because of the wall in between
            if( i != boardMap[r-1][c].id-1 && i != boardMap[r-1][c].id-1-1 && i != boardMap[r-1][c].id-1-M && i != boardMap[r-1][c].id-1+M )
                temp.push_back(i);
        });
        boardMap[r-1][c].adjList = temp;
        temp.clear();

//            bottom right
        for_each( boardMap[r][c].adjList.begin(), boardMap[r][c].adjList.end(), [&](int i) {
//            if any of the 4 possibility occurs, don't push it because of the wall in between
            if( i != boardMap[r][c].id-1 && i != boardMap[r][c].id-1-1 && i != boardMap[r][c].id-1-M && i != boardMap[r][c].id-1+M )
                temp.push_back(i);
        });
        boardMap[r][c].adjList = temp;

    }
    cout << "Map has been updated" << endl;
//    printAdjList();
}


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

    myPosition = *( new playerPosition( 1, (M+1)/2, false ) );
    opPosition = *( new playerPosition( N, (M+1)/2, true ) );


//    both player 1 and player 2 will look at the game from the same perspective
//    there will be a few mapping for player 2 to interpret the opponent's move and my move
//    both players have to reach id 73-81

//    player 1 will have the first move
    if(player == 1)
    {
        memset(sendBuff, '0', sizeof(sendBuff)); 
        string temp;
        cin >> m >> r >> c;
        
        myPosition.update( m, r, c, me );
        opPosition.update( m, r, c, me );

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

        opPosition.update( om, oro, oco, op );
        myPosition.update( om, oro, oco, op );

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

        myPosition.update( m, r, c, me );
        opPosition.update( m, r, c, me );

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
