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

#define NO_PATH -10
#define INVALID_STATE -100000
#define CUTOFF_DEPTH 1
#define DESIRABILITY_CUTOFF 0
#define PASS 1000
#define TRICKY_PATH_BLOCK 2300

long statesPruned = 0;
long statesExplored = 0;
long statesProcessed = 0;

int marginOfVictory = 0;

enum who {
    me,
    op,
    none,
};

who whoWon = (who)none;

using namespace std;
// Complete the function below to print 1 integer which will be your next move

int N,M,K, time_left, player;

class tile {

public:
    int id, row, col;
    vector<int> adjList;

//    i cannot place wall here - remember vertical wall and horizontal wall and their row col
    bool wallPresent = false;

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
    int oldId, oldRow, oldCol, oldPath;
    int wallsLeft;

    static int playersConstructed;

//    this will be my move, opPosition will not use them
    int m, r, c;

    who whoMI;

    tile boardMap[15][15];

//    this vector will containt shortest path to goal where goal is the Nth row
    vector<int> pathToGoal;
    vector<int> goalTiles;

    playerPosition() {
        playersConstructed++;
    }

    playerPosition( int row, int col, bool isEnemy ) {
        this->id = tile::getIdFromRowCol( row, col );
        this->row = row;
        this->col = col;
        this->oldId = tile::getIdFromRowCol( row, col );
        this->oldRow = row;
        this->oldCol = col;

        this->wallsLeft = K;

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

        update( 0, row, col, whoMI, *this );
        playersConstructed++;
    }

//    copy constructor used for minimax
    playerPosition(const playerPosition& player ) {

        this->id = player.id;
        this->row = player.row;
        this->col = player.col;
        this->oldId = player.oldId;
        this->oldRow = player.oldRow;
        this->oldCol = player.oldCol;
        this->oldPath = player.oldPath;
        this->wallsLeft = player.wallsLeft;

//        initialize the adjacency list of the NXM graph
        for( int i = 1; i <= N; i++ ) {
            for( int j = 1; j <= M; j++ )
                this->boardMap[i][j] = player.boardMap[i][j];
        }

        this->whoMI = player.whoMI;
        this->pathToGoal = player.pathToGoal;
        this->goalTiles = player.goalTiles;
        playersConstructed++;
    }

    ~playerPosition() {
        playersConstructed--;
    }

//    uses BFS to find the path to goal
    int findShortestPath() {
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

//        no shortest path to goal
        cout << "no path\n";
        return NO_PATH;

    }

//    returns true IFF opponent is blocking my path and I am unable to get a path to goal
//    only gets called when opponent moves
    bool checkPathBlock( int r, int c ) {
        if( findShortestPath() == NO_PATH )
            return true;
    }

    void updateMap( int m, int r, int c, who whoCalled, playerPosition otherPlayer );

    void updatePosition( int row, int col ) {

        this->oldId = this->id;
        this->oldRow = this->row;
        this->oldCol = this->col;
        this->oldPath = this->findShortestPath();

        this->id = tile::getIdFromRowCol( row, col );
        this->row = row;
        this->col = col;

        cout << "oldId oldRow oldCol id row col - " << oldId << " " << oldRow << " " << oldCol << " " << id << " " << row << " "<< col << "\n";

    }

    void declareWinner() {

//        if someone has already won
//        else check if I won
        if( whoWon != none ) {
            if( whoWon == whoMI ) {
                marginOfVictory++;
                cout << "margin of victory -> " << marginOfVictory << endl;
            }
        }
        else if( find( goalTiles.begin(), goalTiles.end(), id) != goalTiles.end() ) {
            whoWon = whoMI;
            cout << whoWon+1 << " won\n";
        }
    }

//    the order for this function is very important, first positions are updated, then graph of map is updated, then shortest path is obtained
//    using the updated graph of map
//    this otherPlayer is the one whose turn invoked this player's update()
//    whoCalled = me -> otherPlayer = myPosition
//    whoCalled = op -> otherPlayer = opPosition
    void update( int m, int r, int c, who whoCalled, playerPosition otherPlayer ) {

        cout << (int)whoMI+1 << "'s turn - ";

//        PASS
        if( m == 0 && r == 0 && c == 0 )
            return;

        if( m == 0 && whoMI == whoCalled )
            updatePosition( r, c );

        if( m != 0 )
            boardMap[r][c].wallPresent = true;

//        if I placed a wall, my walls left is decremented
        if( m != 0 && whoCalled == whoMI )
            wallsLeft--;

        updateMap( m, r, c, whoCalled, otherPlayer );

        findShortestPath();

        printPath();
    }

    void printPath() {
        cout << "According to " << whoMI+1 << " :-\n";
        for_each( pathToGoal.begin(), pathToGoal.end(), [](int i){ cout<<"["<<tile::getRowFromId(i)<<","<<tile::getColFromId(i)<<"]"; });
        cout << endl;
    }

    void printAdjList() {
        cout << "adjList printing - \n";

        for( int i = 1; i <= N; i++ ) {
            for( int j = 1; j <= M; j++ ) {

                cout << "id i j -> " << tile::getIdFromRowCol(i,j) << "[" << i << " , " << j << "] -> ";

                for_each( boardMap[i][j].adjList.begin(), boardMap[i][j].adjList.end(), [](int i){cout<<"["<<tile::getRowFromId(i)<<" , "<<tile::getColFromId(i)<<"] ";});
                cout << endl;

            }
        }

    }

    void minimax( playerPosition myPosition, playerPosition opPosition );

}myPosition, opPosition;

int playerPosition::playersConstructed = 0;

class state {
public:

    playerPosition myPosition, opPosition;

    static int statesCreated;

//    after this action from the previous state, we came to this state
    int m, r, c;

//    objFunction without checking out children
    int objFunction;

//    returned by best child and will be transferred up
    int utilityFunction;

//    sequence of m r and c with which we reached this state
    vector<int> ms, rs, cs;

    state() {
    }

    state( playerPosition myPosition, playerPosition opPosition ) {
        this->myPosition = myPosition;
        this->opPosition = opPosition;
        findObj();
    }

    ~state() {
    }

    void findObj() {
        if( opPosition.findShortestPath() != NO_PATH && myPosition.findShortestPath() != NO_PATH )
            objFunction = opPosition.findShortestPath() - myPosition.findShortestPath();
        else if( whoWon == op )
            objFunction = -myPosition.findShortestPath();
        else
            objFunction = INVALID_STATE;
    }

    void printStatePaths() {
        myPosition.printPath();
        opPosition.printPath();
    }

    vector<state> exploreStates( who whoCalled );

//    when both players have reached the destination
//    bool isLeaf

};

int state::statesCreated = 0;

typedef vector<state>::iterator vecSIt;

state minVal(state current, int alpha, int beta, int depth, int fatherObj);
state maxVal(state current, int alpha, int beta, int depth, int fatherObj);

vector<state> state::exploreStates(who whoCalled ) {

    vector<state> children;

    playerPosition myPosition = this->myPosition;
    playerPosition opPosition = this->opPosition;

    cout << "exploreStates called\n";

//    caller has already won, he can't move
    if( whoWon == whoCalled ) {
        cout << "declared that " << whoWon+1 << " has won the match\n";
        goto placeWAlls;
    }

    if( whoCalled == me ) {

//        this is true when there is a path without DEAD END
        bool pathFound = false;

        cout << "My adj list - ";
        for_each( myPosition.boardMap[myPosition.row][myPosition.col].adjList.begin(), myPosition.boardMap[myPosition.row][myPosition.col].adjList.end(), [&](int i) {cout<<i<<" ";} );

//        I move
        for_each( myPosition.boardMap[myPosition.row][myPosition.col].adjList.begin(), myPosition.boardMap[myPosition.row][myPosition.col].adjList.end(), [&](int i) {

            cout << "\npushing to state\n";
            state *t = (new state(myPosition, opPosition));
            state temp = *t;

            temp.myPosition.update( 0, tile::getRowFromId(i), tile::getColFromId(i), me, temp.myPosition );
            temp.opPosition.update( 0, tile::getRowFromId(i), tile::getColFromId(i), me, temp.myPosition );

//            avoid oscillating
//            BUT consider this state only if other paths are DEAD END, this thing is handled after this for_each loop
            if( temp.myPosition.row == myPosition.oldRow && temp.myPosition.col == myPosition.oldCol && temp.myPosition.findShortestPath() == myPosition.oldPath )
                goto afterPush;

//            check for dead end : BEGIN

//            path to goal from this state goes through my current location
            if( temp.myPosition.pathToGoal[1] != myPosition.id )
                pathFound = true;

//            check for dead end : END

            temp.m = 0;
            temp.r = tile::getRowFromId(i);
            temp.c = tile::getColFromId(i);
            temp.findObj();
            cout << "State m r c objFunction -> " << temp.m << " " << temp.r << " " << temp.c << " " << temp.objFunction << "\n";

//            check for TRICKY PATH BLOCK when my movement blocks other person's path
            if( temp.objFunction == INVALID_STATE ) {
                cout << "the state mentioned above has been marked as TRICKY_PATH_BLOCK\n";
                temp.objFunction = TRICKY_PATH_BLOCK;
            }

            statesExplored++;
            children.push_back(temp);

            afterPush:

            delete t;

        } );

//        check for oscillating move, will have to go back to where I was
        if( !pathFound ) {

            cout << "pushing to state\n";
            state *t = (new state(myPosition, opPosition));
            state temp = *t;

            temp.myPosition.update( 0, myPosition.oldRow, myPosition.oldCol, me, temp.myPosition );
            temp.opPosition.update( 0, myPosition.oldRow, myPosition.oldCol, me, temp.myPosition );

            temp.m = 0;
            temp.r = myPosition.oldRow;
            temp.c = myPosition.oldCol;
            temp.findObj();
            cout << "State m r c objFunction -> " << temp.m << " " << temp.r << " " << temp.c << " " << temp.objFunction << "\n";
            statesExplored++;
            children.push_back(temp);

            delete t;
        }

    } else {

//        Opponent move
        for_each( opPosition.boardMap[opPosition.row][opPosition.col].adjList.begin(), opPosition.boardMap[opPosition.row][opPosition.col].adjList.end(), [&](int i) {

            cout << "pushing to state\n";
            state *t = (new state(myPosition, opPosition));
            state temp = *t;

            temp.opPosition.update( 0, tile::getRowFromId(i), tile::getColFromId(i), op, temp.opPosition );
            temp.myPosition.update( 0, tile::getRowFromId(i), tile::getColFromId(i), op, temp.opPosition );

            temp.m = 0;
            temp.r = tile::getRowFromId(i);
            temp.c = tile::getColFromId(i);
            temp.findObj();
            cout << "State m r c objFunction -> " << temp.m << " " << temp.r << " " << temp.c << " " << temp.objFunction << "\n";
            statesExplored++;
            children.push_back(temp);

            delete t;

        } );

    }

//    I won't consider placing walls if my opponent has already won
    if( me == whoCalled && op == whoWon || op == whoCalled && me == whoWon )
        return children;

//    now I will consider placing walls

    placeWAlls:

//    no walls left
    if( whoCalled == me && myPosition.wallsLeft <= 0 )
        return children;
    if( whoCalled == op && opPosition.wallsLeft <= 0 )
        return children;

//    how many horizontal walls can be placed
    for( int i = 2; i <= N; i++ ) {
        for( int j = 2; j <= M; j++ ) {

//            smartly skip placing wall based on the if condition
            if( whoCalled == me ) {
                if( abs(opPosition.row - i) > 3 )
                    continue;
            } else {
                if( abs(myPosition.row - i) > 3 )
                    continue;
            }

//            don't place wall if already present
            if( myPosition.boardMap[i][j].wallPresent || opPosition.boardMap[i][j].wallPresent )
                continue;

            vector<int> tlAdjList1 = myPosition.boardMap[i-1][j-1].adjList;
            vector<int> tlAdjList2 = opPosition.boardMap[i-1][j-1].adjList;
            vector<int> blAdjList1 = myPosition.boardMap[i][j-1].adjList;
            vector<int> blAdjList2 = opPosition.boardMap[i][j-1].adjList;
            vector<int> trAdjList1 = myPosition.boardMap[i-1][j].adjList;
            vector<int> trAdjList2 = opPosition.boardMap[i-1][j].adjList;
            vector<int> brAdjList1 = myPosition.boardMap[i][j].adjList;
            vector<int> brAdjList2 = opPosition.boardMap[i][j].adjList;

            int tlId = tile::getIdFromRowCol(i-1,j-1);
            int blId = tile::getIdFromRowCol(i,j-1);
            int trId = tile::getIdFromRowCol(i-1,j);
            int brId = tile::getIdFromRowCol(i,j);

            bool val1 = find( tlAdjList1.begin(), tlAdjList1.end(), blId) != tlAdjList1.end() ? true : false;
            bool val2 = find( tlAdjList2.begin(), tlAdjList2.end(), blId) != tlAdjList2.end() ? true : false;
            bool val3 = find( blAdjList1.begin(), blAdjList1.end(), tlId) != blAdjList1.end() ? true : false;
            bool val4 = find( blAdjList2.begin(), blAdjList2.end(), tlId) != blAdjList2.end() ? true : false;
            bool val5 = find( trAdjList1.begin(), trAdjList1.end(), brId) != trAdjList1.end() ? true : false;
            bool val6 = find( trAdjList2.begin(), trAdjList2.end(), brId) != trAdjList2.end() ? true : false;
            bool val7 = find( brAdjList1.begin(), brAdjList1.end(), trId) != brAdjList1.end() ? true : false;
            bool val8 = find( brAdjList2.begin(), brAdjList2.end(), trId) != brAdjList2.end() ? true : false;

//            wall can be placed if true
            if( (val1 || val2 || val3 || val4) && (val5 || val6 || val7 || val8) ) {

                state *t = (new state(myPosition, opPosition));
                state temp = *t;

//                last parameter does not matter here
                temp.myPosition.update( 1, i, j, me, temp.myPosition );
                temp.opPosition.update( 1, i, j, me, temp.opPosition );

                temp.m = 1;
                temp.r = i;
                temp.c = j;
                temp.findObj();

//                no path to goal in this state, check for another location
                if( temp.objFunction == INVALID_STATE ) {
                    delete t;
                    continue;
                }

                if( whoCalled == me ) {
                    int stateDesirability = temp.objFunction - this->objFunction;
                    if( stateDesirability <= DESIRABILITY_CUTOFF ) {
                        delete t;
                        continue;
                    }
                } else {
                    int stateDesirability = this->objFunction - temp.objFunction;
                    if( stateDesirability <= DESIRABILITY_CUTOFF ) {
                        delete t;
                        continue;
                    }
                }

                cout << "pushing to state\n";
                cout << "State m r c objFunction -> " << temp.m << " " << temp.r << " " << temp.c << " " << temp.objFunction << "\n";
                statesExplored++;
                children.push_back(temp);

                delete t;

            }

        }
    }

//    how many vertical walls can be placed
    for( int i = 2; i <= N; i++ ) {
        for( int j = 2; j <= M; j++ ) {

//            smartly skip placing wall based on the if condition
            if( whoCalled == me ) {
                if( abs(opPosition.row - i) > 3 )
                    continue;
            } else {
                if( abs(myPosition.row - i) > 3 )
                    continue;
            }

//            don't place wall if already present
            if( myPosition.boardMap[i][j].wallPresent || opPosition.boardMap[i][j].wallPresent )
                continue;

            vector<int> tlAdjList1 = myPosition.boardMap[i-1][j-1].adjList;
            vector<int> tlAdjList2 = opPosition.boardMap[i-1][j-1].adjList;
            vector<int> blAdjList1 = myPosition.boardMap[i][j-1].adjList;
            vector<int> blAdjList2 = opPosition.boardMap[i][j-1].adjList;
            vector<int> trAdjList1 = myPosition.boardMap[i-1][j].adjList;
            vector<int> trAdjList2 = opPosition.boardMap[i-1][j].adjList;
            vector<int> brAdjList1 = myPosition.boardMap[i][j].adjList;
            vector<int> brAdjList2 = opPosition.boardMap[i][j].adjList;

            int tlId = tile::getIdFromRowCol(i-1,j-1);
            int blId = tile::getIdFromRowCol(i,j-1);
            int trId = tile::getIdFromRowCol(i-1,j);
            int brId = tile::getIdFromRowCol(i,j);

            bool val1 = find( tlAdjList1.begin(), tlAdjList1.end(), trId) != tlAdjList1.end() ? true : false;
            bool val2 = find( tlAdjList2.begin(), tlAdjList2.end(), trId) != tlAdjList2.end() ? true : false;
            bool val3 = find( blAdjList1.begin(), blAdjList1.end(), brId) != blAdjList1.end() ? true : false;
            bool val4 = find( blAdjList2.begin(), blAdjList2.end(), brId) != blAdjList2.end() ? true : false;
            bool val5 = find( trAdjList1.begin(), trAdjList1.end(), tlId) != trAdjList1.end() ? true : false;
            bool val6 = find( trAdjList2.begin(), trAdjList2.end(), tlId) != trAdjList2.end() ? true : false;
            bool val7 = find( brAdjList1.begin(), brAdjList1.end(), blId) != brAdjList1.end() ? true : false;
            bool val8 = find( brAdjList2.begin(), brAdjList2.end(), blId) != brAdjList2.end() ? true : false;

//            wall can be placed if true
            if( (val1 || val2 || val5 || val6) && (val3 || val4 || val7 || val8) ) {

                state *t = (new state(myPosition, opPosition));
                state temp = *t;

//                last parameter does not matter here
                temp.myPosition.update( 2, i, j, me, temp.myPosition );
                temp.opPosition.update( 2, i, j, me, temp.opPosition );

                temp.m = 2;
                temp.r = i;
                temp.c = j;
                temp.findObj();

//                no path to goal in this state, check for another location
                if( temp.objFunction == INVALID_STATE ) {
                    delete t;
                    continue;
                }

                if( whoCalled == me ) {
                    int stateDesirability = temp.objFunction - this->objFunction;
                    if( stateDesirability <= DESIRABILITY_CUTOFF ) {
                        delete t;
                        continue;
                    }
                } else {
                    int stateDesirability = this->objFunction - temp.objFunction;
                    if( stateDesirability <= DESIRABILITY_CUTOFF ) {
                        delete t;
                        continue;
                    }
                }

                cout << "pushing to state\n";
                cout << "State m r c objFunction -> " << temp.m << " " << temp.r << " " << temp.c << " " << temp.objFunction << "\n";
                statesExplored++;
                children.push_back(temp);

                delete t;

            }
        }
    }

//    all states have been explored
    return children;
}

// this otherPlayer is the one whose turn invoked this player's updateMap
// whoCalled = me -> otherPlayer = myPosition
// whoCalled = op -> otherPlayer = opPosition
void playerPosition::updateMap( int m, int r, int c, who whoCalled, playerPosition otherPlayer ) {

//    me will update the map if op moved to take into account the tricky moves that me can make
//    AND VICE VERSA
    if( m == 0 && whoCalled != whoMI ) {

//        all ids and rows and cols here belong to whoCalled and NOT whoMI
        int id, row, col;
        int oldId, oldRow, oldCol;

        id = otherPlayer.id;
        row = otherPlayer.row;
        col = otherPlayer.col;
        oldId = otherPlayer.oldId;
        oldRow = otherPlayer.oldRow;
        oldCol = otherPlayer.oldCol;

//        cout << "Inside updateMap, whereabouts of the otherPlayer - \n" << id << " " << row << " " << col << " " << oldId << " " << oldRow << " "<< oldCol << "\n";

        vector<int> oldPosAdjList = boardMap[ oldRow ][ oldCol ].adjList;
        vector<int> newPosAdjList = boardMap[ row ][ col ].adjList;

        bool pushed;
        vector<int> temp;

//        top
        if( oldRow > 1 ) {
            vector<int> oldPosTopAdjList = boardMap[ oldRow-1 ][ oldCol ].adjList;

            for_each( oldPosTopAdjList.begin(), oldPosTopAdjList.end(), [&](int i){

                if( (i == oldId+M || i == oldId-1 || i == oldId+1) && !pushed ) {
                    temp.push_back( oldId );
                    pushed = true;
                } else if( !(i == oldId+M || i == oldId-1 || i == oldId+1) )
                    temp.push_back(i);

            });
            pushed = false;
            boardMap[ oldRow-1 ][ oldCol ].adjList = temp;
            temp.clear();
        }
//        lef
        if( oldCol > 1 ) {
            vector<int> oldPosLefAdjList = boardMap[ oldRow ][ oldCol-1l ].adjList;

            for_each( oldPosLefAdjList.begin(), oldPosLefAdjList.end(), [&](int i){

                if( (i == oldId+1 || i == oldId-M || i == oldId+M) && !pushed ) {
                    temp.push_back( oldId );
                    pushed = true;
                } else if( !(i == oldId+1 || i == oldId-M || i == oldId+M) )
                    temp.push_back(i);

            });
            pushed = false;
            boardMap[ oldRow ][ oldCol-1 ].adjList = temp;
            temp.clear();
        }
//        rig
        if( oldCol < M ) {
            vector<int> oldPosRigAdjList = boardMap[ oldRow ][ oldCol+1 ].adjList;

            for_each( oldPosRigAdjList.begin(), oldPosRigAdjList.end(), [&](int i){

                if( (i == oldId-1 || i == oldId-M || i == oldId+M) && !pushed ) {
                    temp.push_back( oldId );
                    pushed = true;
                } else if( !(i == oldId-1 || i == oldId-M || i == oldId+M) )
                    temp.push_back(i);

            });
            pushed = false;
            boardMap[ oldRow ][ oldCol+1 ].adjList = temp;
            temp.clear();
        }
//        bot
        if( oldRow < N ) {
            vector<int> oldPosBotAdjList = boardMap[ oldRow+1 ][ oldCol ].adjList;

            for_each( oldPosBotAdjList.begin(), oldPosBotAdjList.end(), [&](int i){

                if( (i == oldId-M || i == oldId-1 || i == oldId+1) && !pushed ) {
                    temp.push_back( oldId );
                    pushed = true;
                } else if( !(i == oldId-M || i == oldId-1 || i == oldId+1)  )
                    temp.push_back(i);

            });
            pushed = false;
            boardMap[ oldRow+1 ][ oldCol ].adjList = temp;
            temp.clear();

        }

//        top
        if( row > 1 ) {
            vector<int> newPosTopAdjList = boardMap[ row-1 ][ col ].adjList;

            for_each( newPosTopAdjList.begin(), newPosTopAdjList.end(), [&](int i){

                if( i == id ) {

//                    if current location has no wall below or current location is not at Nth row
//                    (oldRow==row+1) ensures the above comment's condition, look carefully
//                    in the beginning, oldRow and row has problems, hence id+M<82 is placed
//                    else if current location has no wall to left or current location is not at 1st col
//                         if current location has no wall to righ or current location is not at Mth col
                    if( (find( newPosAdjList.begin(), newPosAdjList.end(), id+M ) != newPosAdjList.end() || (oldRow==row+1)) && id+M<82 ) {
                        temp.push_back( id+M );
                    } else {
                        if( find( newPosAdjList.begin(), newPosAdjList.end(), id-1 ) != newPosAdjList.end()|| (oldCol==col-1) )
                            temp.push_back( id-1 );
                        if( find( newPosAdjList.begin(), newPosAdjList.end(), id+1 ) != newPosAdjList.end()|| (oldCol==col+1) )
                            temp.push_back( id+1 );
                    }

                } else
                    temp.push_back(i);

            });
            boardMap[ row-1 ][ col ].adjList = temp;
            temp.clear();
        }
//        lef
        if( col > 1 ) {
            vector<int> newPosLefAdjList = boardMap[ row ][ col-1 ].adjList;

            for_each( newPosLefAdjList.begin(), newPosLefAdjList.end(), [&](int i){

                if( i == id ) {

//                    if current location has no wall to righ or current location is not at Mth col
//                    (oldCol==col+1) along with id-M>0 ensures the above comment's condition, look carefully
//                    else if current location has no wall above or current location is not at 1st row
//                         if current location has no wall below or current location is not at Nth row
                    if( (find( newPosAdjList.begin(), newPosAdjList.end(), id+1 ) != newPosAdjList.end() || (oldCol==col+1)) ) {
                        temp.push_back( id+1 );
                    } else {
                        if( find( newPosAdjList.begin(), newPosAdjList.end(), id-M ) != newPosAdjList.end() || (oldRow==row-1) )
                            temp.push_back( id-M );
                        if( find( newPosAdjList.begin(), newPosAdjList.end(), id+M ) != newPosAdjList.end() || (oldRow==row+1) )
                            temp.push_back( id+M );
                    }

                } else
                    temp.push_back(i);

            });
            boardMap[ row ][ col-1 ].adjList = temp;
            temp.clear();
        }
//        rig
        if( col < M ) {
            vector<int> newPosRigAdjList = boardMap[ row ][ col+1 ].adjList;

            for_each( newPosRigAdjList.begin(), newPosRigAdjList.end(), [&](int i){

                if( i == id ) {

//                    if current location has no wall to left or current location is not at 1st col
//                    (oldCol==col-1) along with id-M>0 ensures the above comment's condition, look carefully
//                    else if current location has no wall above or current location is not at 1st row
//                         if current location has no wall below or current location is not at Nth row
                    if( (find( newPosAdjList.begin(), newPosAdjList.end(), id-1 ) != newPosAdjList.end() || (oldCol==col-1)) ) {
                        temp.push_back( id-1 );
                    } else {
                        if( find( newPosAdjList.begin(), newPosAdjList.end(), id-M ) != newPosAdjList.end() || (oldRow==row-1) )
                            temp.push_back( id-M );
                        if( find( newPosAdjList.begin(), newPosAdjList.end(), id+M ) != newPosAdjList.end() || (oldRow==row+1) )
                            temp.push_back( id+M );
                    }

                } else
                    temp.push_back(i);

            });

            boardMap[ row ][ col+1 ].adjList = temp;
            temp.clear();
        }
//        bot
        if( row < N ) {
            vector<int> newPosBotAdjList = boardMap[ row+1 ][ col ].adjList;

            for_each( newPosBotAdjList.begin(), newPosBotAdjList.end(), [&](int i){

                if( i == id ) {

//                    if current location has no wall above or current location is not at 1st row
//                    (oldRow==row-1) along with id-M>0 ensures the above comment's condition, look carefully
//                    in the beginning, oldRow and row has problems, hence id-M>0 is placed
//                    else if current location has no wall to left or current location is not at 1st col
//                         if current location has no wall to righ or current location is not at Mth col
                    if( (find( newPosAdjList.begin(), newPosAdjList.end(), id-M ) != newPosAdjList.end() || (oldRow==row-1)) && id-M>0 ) {
                        temp.push_back( id-M );
                    } else {
                        if( find( newPosAdjList.begin(), newPosAdjList.end(), id-1 ) != newPosAdjList.end() || (oldCol==col-1) )
                            temp.push_back( id-1 );
                        if( find( newPosAdjList.begin(), newPosAdjList.end(), id+1 ) != newPosAdjList.end() || (oldCol==col+1) )
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
//    cout << "Map has been updated" << endl;
}

state minVal(state current, int alpha, int beta, int depth, int fatherObj){

    current.findObj();

    if( depth > CUTOFF_DEPTH ) {
        cout << "minVal no children\n";
        current.utilityFunction = current.objFunction;
        return current;
    }

    cout << "minVal called\n";

    statesProcessed++;

    vector<state> children;
    children = current.exploreStates( op );

    if( children.size() == 0 ) {
        cout << "minVal no children\n";
        current.utilityFunction = PASS;
        return current;
    }

    int min = INT_MAX;
    state minState;

    for( vecSIt it = children.begin(); (it != children.end()) && !(beta <= alpha); it++) {

        state s = *it;
        s.findObj();

//        this child is blocking opponent i.e. TRICKY_PATH_BLOCK and I was not here before, then come here for sure
        if( s.objFunction == TRICKY_PATH_BLOCK && s.opPosition.findShortestPath() < current.opPosition.findShortestPath() ) {
            s.utilityFunction = -INT_MAX;
            return s;
        }

//        my children will maximize their objective function
        state nextState = maxVal( s, alpha, beta, depth+1, s.objFunction );

//        if the other player PASSED, objective function of s is its utility function
//        else, do normal gaming
        if( nextState.utilityFunction == PASS ) {
            s.utilityFunction = s.objFunction;
            cout << "\n\n\n\n\ns m r c " << s.m << " " << s.r << " " << s.c << endl;

            if( min > s.utilityFunction ) {
                min = s.utilityFunction;
                minState = s;
            }
        } else {
//            update beta value
            if( nextState.utilityFunction < beta )
                beta = nextState.utilityFunction;

            if( min > nextState.utilityFunction ) {
                min = nextState.utilityFunction;
//                choose that children whose children's max value is less than min so far
                minState = s;
                minState.utilityFunction = nextState.utilityFunction;
            }

//            utility function is same
//            tie breaking and favour that child whose objective function is higher
            if( min == nextState.utilityFunction && nextState.objFunction < minState.objFunction ) {
                minState = s;
                minState.utilityFunction = nextState.utilityFunction;
            }
        }

    }

    children.clear();

    return minState;

    /*if(leaf(state))
        return utility(state);
    for(s in chidren(state)){
        chile=maxVal(s,alpha,beta);
        beta=min(beta,child);
        if(alpha>=beta) return child;
        }
     return best child(min);
    */
}

// depth is the depth of the minimax tree
// I will find max out of my children and all my children will find min out of their children
// it will comprise of one depth
state maxVal(state current, int alpha, int beta, int depth, int fatherObj){

    statesProcessed++;

    current.findObj();

    if( depth > CUTOFF_DEPTH ) {
        current.utilityFunction = current.objFunction;
        return current;
    }

    cout << "maxVal called\n";

    vector<state> children;
    children = current.exploreStates( me );

    if( children.size() == 0 ) {
        cout << "maxVal no children\n";
        current.utilityFunction = PASS;
        return current;
    }

    int max = -INT_MAX;
    state maxState;

    for( vecSIt it = children.begin(); (it != children.end()) && !(beta <= alpha); it++) {

        cout << "child number " << it - children.begin() << endl;

        state s = *it;

//        this child is blocking opponent i.e. TRICKY_PATH_BLOCK and I was not here before, then come here for sure
        cout << "\ns.objFunction s.myPosition.findShortestPath() current.myPosition.findShortestPath() " << s.objFunction << " " << s.myPosition.findShortestPath() << " " << current.myPosition.findShortestPath() << endl;
        if( s.objFunction == TRICKY_PATH_BLOCK && s.myPosition.findShortestPath() < current.myPosition.findShortestPath() ) {
            s.utilityFunction = +INT_MAX;
            return s;
        }

        s.findObj();

//        my children will minimize their objective function
        state nextState = minVal( s, alpha, beta, depth, s.objFunction );

//        if the other player PASSED, objective function of s is its utility function
//        else, do normal gaming
        if( nextState.utilityFunction == PASS ) {
            s.utilityFunction = s.objFunction;
            cout << "\n\n\n\n\ns m r c " << s.m << " " << s.r << " " << s.c << endl;

            if( max < s.utilityFunction ) {
                max = s.utilityFunction;
                maxState = s;
            }
        }
        else {
//            update alpha value
            if( nextState.utilityFunction > alpha )
                alpha = nextState.utilityFunction;

            if( max < nextState.utilityFunction ) {
                max =  nextState.utilityFunction;
//                choose that children whose children's min value is more than max so far
                maxState = s;
                maxState.utilityFunction = nextState.utilityFunction;
            }

//            utility function is same
//            tie breaking and favour that child whose objective function is higher
            if( max == nextState.utilityFunction && maxState.objFunction < s.objFunction ) {
                cout << "tie happened\n";
                maxState = s;
                maxState.utilityFunction = nextState.utilityFunction;
            }
        }

    }

    children.clear();

    return maxState;

    /*if(leaf(state))
        return utility(state);

    for(s in chidren(state)){
        chile=minVal(s,alpha,beta);
        alpha=max(beta,child);
        if(alpha>=beta) return child;
        }
     return best child(max);
    */
}

void playerPosition::minimax( playerPosition myPosition, playerPosition opPosition ) {

    time_t beg = clock();

    state start = *(new state(myPosition, opPosition));
    start.findObj();

    state nextState = maxVal( start, -INT_MAX, INT_MAX, 1, start.objFunction );

//    pass
    if( nextState.utilityFunction == PASS ) {
        this->m = 0;
        this->r = 0;
        this->c = 0;
        return;
    }

    cout << "playerPosition::minimax( playerPosition myPosition, playerPosition opPosition ) found nextState with utility Function - " << nextState.utilityFunction << "\n";
    cout << "nextState's path is - \n";
    nextState.myPosition.printPath();
//    this is the motive of minimax
    this->m = nextState.m;
    this->r = nextState.r;
    this->c = nextState.c;

    time_t end = clock();

    cout << "\n\nTime taken is - " << float(end - beg)/CLOCKS_PER_SEC;
    /*v=maxVal(myPosition,opPosition);
    return action(v);
    */
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

    myPosition.updateMap( 0, N, (M+1)/2, op, opPosition);
    opPosition.updateMap( 0, 1, (M+1)/2, me, myPosition);

//    both player 1 and player 2 will look at the game from the same perspective
//    there will be a few mapping for player 2 to interpret the opponent's move and my move
//    both players have to reach id 73-81

//    player 1 will have the first move
    if(player == 1)
    {
        memset(sendBuff, '0', sizeof(sendBuff));
        string temp;

//        this will decide m r c
        myPosition.minimax( myPosition, opPosition );

        m = myPosition.m;
        r = myPosition.r;
        c = myPosition.c;

        cout << "myPosition.minimax( myPosition, opPosition ) gave m r c - " << m << " " << r << " " << c << endl;

        myPosition.update( m, r, c, me, myPosition );
        opPosition.update( m, r, c, me, myPosition );

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

        cout << "\nMy adj list before update\n";
        myPosition.printAdjList();
        cout << "Op adj list before update\n";
        opPosition.printAdjList();

        opPosition.update( om, oro, oco, op, opPosition );
        myPosition.update( om, oro, oco, op, opPosition );

        cout << "My adj list after update\n";
        myPosition.printAdjList();
        cout << "Op adj list after update\n";
        opPosition.printAdjList();

        opPosition.declareWinner();

//        opponent's move blocked me
        if( myPosition.checkPathBlock( oro, oco) == true && om == 0 ) {
            cout << "\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\nMY PATH HAS BEEN BLOCKED";
            cout << "\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n";
            cin.ignore();
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
//        this will decide m r c
        myPosition.minimax( myPosition, opPosition );

        m = myPosition.m;
        r = myPosition.r;
        c = myPosition.c;

        cout << "\nMy adj list before update\n";
        myPosition.printAdjList();
        cout << "Op adj list before update\n";
        opPosition.printAdjList();

        myPosition.update( m, r, c, me, myPosition );
        opPosition.update( m, r, c, me, myPosition );

        cout << "My adj list after update\n";
        myPosition.printAdjList();
        cout << "Op adj list after update\n";
        opPosition.printAdjList();

        myPosition.declareWinner();

        cout << "myPosition.minimax( myPosition, opPosition ) gave m r c - " << m << " " << r << " " << c << endl;

        cout << "my walls left " << myPosition.wallsLeft << endl;

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

        cout << "Waiting for you to press something so that I can continue\n";
        cin.ignore();

        snprintf(sendBuff, sizeof(sendBuff), "%d %d %d", m, r , c);
        write(sockfd, sendBuff, strlen(sendBuff));

        memset(recvBuff, '0',sizeof(recvBuff));
        n = read(sockfd, recvBuff, sizeof(recvBuff)-1);
        recvBuff[n] = 0;

//        time remaining returned by the server after I played my turn
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
