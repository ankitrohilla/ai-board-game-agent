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
#define STANDARD_CUTOFF_DEPTH 2
#define STANDARD_DESIRABILITY_CUTOFF 0
#define PASS 1000
#define TRICKY_PATH_BLOCK 2300
#define MAX_ID (M*N+1)
#define IMPROVEMENT 1

float TL;
int moveWt=100,horizontalWallWt=100,verticalWallWt=100;
int DESIRABILITY_CUTOFF = STANDARD_DESIRABILITY_CUTOFF;

int cutoffDepth = STANDARD_CUTOFF_DEPTH;

int turnNumber = 0;

long statesPruned = 0;
long statesExplored = 0;
long statesProcessed = 0;

int marginOfVictory = 0;

float avgBranchingFactor, maxBranchingFactor, cumulativeBranchingFactor = 0;
float avgMinimaxTime, maxMinimaxTime, cumulativeMinimaxTime = 0.0;

// number of nodes whose children have been explored
int totalExpandedNodes = 0, totalMinimaxCalled = 0;

class moves{
public:
    int m,r,c;
    void updateWeight(int opm,int opr,int opc){
        if( opm!=m ){
            if( opm ==0){
                moveWt+=IMPROVEMENT;
                horizontalWallWt-=IMPROVEMENT;
                verticalWallWt-=IMPROVEMENT;

            }
            else if( opm == 1 ){
                moveWt-=IMPROVEMENT;
                horizontalWallWt+=IMPROVEMENT;
                verticalWallWt-=IMPROVEMENT;
            }
            else{
                moveWt-=IMPROVEMENT;
                horizontalWallWt-=IMPROVEMENT;
                verticalWallWt+=IMPROVEMENT;
            }
        }
        else{
            if( opr != r || opc != c ){
                if(opm==0)
                    moveWt-=IMPROVEMENT;
                else if(opm==1)
                    horizontalWallWt-=IMPROVEMENT;
                else
                    verticalWallWt-=IMPROVEMENT;

            }
        }
    }
}expectedMove;

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

        update( 0, row, col, whoMI, *this, *this );
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

        pathToGoal.clear();

//        I am already at the goal tile
        if( find( goalTiles.begin(), goalTiles.end(), id) != goalTiles.end() ) {
            pathToGoal.push_back(id);
            return 1;
        }

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
//        cout << "no path\n";
        return NO_PATH;

    }

//    returns true IFF opponent is blocking my path and I am unable to get a path to goal
//    only gets called when opponent moves
    bool checkPathBlock( int r, int c ) {
        if( findShortestPath() == NO_PATH )
            return true;
    }

    void updateMap(int m, int r, int c, who whoCalled, playerPosition callerPlayer , playerPosition otherPlayer);

    void updatePosition( int row, int col ) {

        this->oldId = this->id;
        this->oldRow = this->row;
        this->oldCol = this->col;
        this->oldPath = this->findShortestPath();

        this->id = tile::getIdFromRowCol( row, col );
        this->row = row;
        this->col = col;

//        cout << "oldId oldRow oldCol id row col - " << oldId << " " << oldRow << " " << oldCol << " " << id << " " << row << " "<< col << "\n";

    }

    void declareWinner();

//    the other player has won and will be TOTALLY removed
    void removeOtherPlayer( int row, int col ) {

        int id = tile::getIdFromRowCol(row,col);

        vector<int> temp;
        bool pushed = false;

//        checking for the cell to the lef of the winner
        if( col > 1 ) {
            for_each( boardMap[row][col-1].adjList.begin(), boardMap[row][col-1].adjList.end(), [&](int i){

                if( i == id+1 || i == id-M || i == id+M ) {
                    if( !pushed ) {
                        temp.push_back( id );
                        pushed = true;
                    }
                } else {
                    temp.push_back( i );
                }

            });
            boardMap[row][col-1].adjList = temp;
        }
//        checking for the cell to the rig of the winner
        if( col < M ) {
            for_each( boardMap[row][col+1].adjList.begin(), boardMap[row][col+1].adjList.end(), [&](int i){

                if( i == id-1 || i == id-M || i == id+M ) {
                    if( !pushed ) {
                        temp.push_back( id );
                        pushed = true;
                    }
                } else {
                    temp.push_back( i );
                }

            });
            boardMap[row][col+1].adjList = temp;
        }
//        checking for the cell to the top of the winner
        if( row > 1 ) {
            for_each( boardMap[row-1][col].adjList.begin(), boardMap[row-1][col].adjList.end(), [&](int i){

                if( i == id+M || i == id-1 || i == id+1 ) {
                    if( !pushed ) {
                        temp.push_back( id );
                        pushed = true;
                    }
                } else {
                    temp.push_back( i );
                }

            });
            boardMap[row-1][col].adjList = temp;
        }
//        checking for the cell to the bot of the winner
        if( row < N ) {
            for_each( boardMap[row+1][col].adjList.begin(), boardMap[row+1][col].adjList.end(), [&](int i){

                if( i == id-M || i == id-1 || i == id+1 ) {
                    if( !pushed ) {
                        temp.push_back( id );
                        pushed = true;
                    }
                } else {
                    temp.push_back( i );
                }

            });
            boardMap[row+1][col].adjList = temp;
        }

    }

//    the order for this function is very important, first positions are updated, then graph of map is updated, then shortest path is obtained
//    using the updated graph of map
//    this callerPlayer is the one whose turn invoked this player's update()
//    whoCalled = me -> callerPlayer = myPosition
//    whoCalled = op -> callerPlayer = opPosition
//    if whoMI = me, otherPlayer = opPosition and vice-versa
    void update( int m, int r, int c, who whoCalled, playerPosition callerPlayer, playerPosition otherPlayer ) {

//        cout << (int)whoMI+1 << "'s turn - ";

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

//        otherPlayer is useful to check the position of this object's opposite to handle overjumping when a wall is placed
        updateMap( m, r, c, whoCalled, callerPlayer, otherPlayer );

        findShortestPath();

//        printPath();
    }

    void printPath() {
        //cout << "According to " << whoMI+1 << " :- ";
//        for_each( pathToGoal.begin(), pathToGoal.end(), [](int i){ //cout<<"["<<tile::getRowFromId(i)<<","<<tile::getColFromId(i)<<"]"; });
        //cout << endl;
    }

    void printAdjList() {
        //cout << "adjList printing - \n";

        for( int i = 1; i <= N; i++ ) {
            for( int j = 1; j <= M; j++ ) {

                //cout << "id i j -> " << tile::getIdFromRowCol(i,j) << "[" << i << " , " << j << "] -> ";

//                for_each( boardMap[i][j].adjList.begin(), boardMap[i][j].adjList.end(), [](int i){//cout<<"["<<tile::getRowFromId(i)<<" , "<<tile::getColFromId(i)<<"] ";});
                //cout << endl;

            }
        }

    }

    void minimax( playerPosition myPosition, playerPosition opPosition );

}myPosition, opPosition;

void playerPosition::declareWinner() {

//        if someone has already won
//        else check if I won
    if( whoWon != none && whoWon == whoMI ) {
        marginOfVictory++;
        //cout << "margin of victory -> " << marginOfVictory << endl;
    } else if( find( goalTiles.begin(), goalTiles.end(), id) != goalTiles.end() ) {
        whoWon = whoMI;
        //cout << whoWon+1 << " won\n";
    } else if( whoWon != none && whoWon != whoMI ) {

//        if op has won
//        else me has won
        if( whoMI == me ) {
            removeOtherPlayer( opPosition.row, opPosition.col);
        } else if( whoMI == op ) {
            removeOtherPlayer( myPosition.row, myPosition.col);
        }

    }
}

int playerPosition::playersConstructed = 0;

class state;
class state {
public:

    playerPosition myPosition, opPosition;

    static int statesCreated;

//    number of children will decide the depth of the game tree
    int branchingFactor = 0;

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

    state( playerPosition myPosition, playerPosition opPosition, int m, int r, int c) {
        this->myPosition = myPosition;
        this->opPosition = opPosition;
        findObj();
        ms.push_back( m );
        rs.push_back( r );
        cs.push_back( c );
    }

    ~state() {
    }

    void findObj() {
        int wt;
        if(m==0){
            wt=moveWt;
        }
        else if(m==1){
            wt=horizontalWallWt;
        }
        else{
            wt=verticalWallWt;
        }
        if( opPosition.findShortestPath() != NO_PATH && myPosition.findShortestPath() != NO_PATH ) {
            objFunction = (opPosition.findShortestPath() - myPosition.findShortestPath())*100;
//            opponent is too close to victory, much bad state
            if( opPosition.findShortestPath() < 3 )
                objFunction -= 1000;
            else if( myPosition.wallsLeft < opPosition.wallsLeft && m != 0 && whoWon == none ) {
                objFunction -= (opPosition.wallsLeft - myPosition.wallsLeft) * 100;
            }

//            if(opPosition.findShortestPath() < 4 && myPosition.wallsLeft < 4){
//                objFunction -= 1000;
//            }
        }
        else if( whoWon == op )
            objFunction = -myPosition.findShortestPath()*wt;
        else
            objFunction = INVALID_STATE;
    }

    void printStateStats() {
        //cout << "Sequence of ms rs and cs with which this state will be taken is - \n";
        for( int i = 0; i < ms.size(); i++ ) {
            //cout << "mrc - " << ms[i] << " " << rs[i] << " " << cs[i] << "\n";
        }
//        myPosition.printPath();
//        opPosition.printPath();
    }

    vector<state> exploreStates( who whoCalled );

};

int state::statesCreated = 0;

typedef vector<state>::iterator vecSIt;

state minVal(state current, int alpha, int beta, int depth, int fatherObj);
state maxVal(state current, int alpha, int beta, int depth, int fatherObj);

vector<state> state::exploreStates(who whoCalled ) {

    vector<state> children;

    playerPosition myPosition = this->myPosition;
    playerPosition opPosition = this->opPosition;

//    //cout << "exploreStates called\n";

//    caller has already won, he can't move
    if( whoWon == whoCalled ) {
        //cout << "declared that " << whoWon+1 << " has won the match\n";
        goto placeWAlls;
    }

    if( whoCalled == me ) {

//        this is true when there is a path without DEAD END
        bool pathFound = false;

//        cout << "My adj list - ";
//        for_each( myPosition.boardMap[myPosition.row][myPosition.col].adjList.begin(), myPosition.boardMap[myPosition.row][myPosition.col].adjList.end(), [&](int i) {cout<<i<<" ";} );

//        I move
        for_each( myPosition.boardMap[myPosition.row][myPosition.col].adjList.begin(), myPosition.boardMap[myPosition.row][myPosition.col].adjList.end(), [&](int i) {

//            cout << "\npushing to state\n";
            state *t = (new state(myPosition, opPosition, 0, tile::getRowFromId(i), tile::getColFromId(i)));
            state temp = *t;

            temp.myPosition.update( 0, tile::getRowFromId(i), tile::getColFromId(i), me, temp.myPosition, temp.opPosition );
            temp.opPosition.update( 0, tile::getRowFromId(i), tile::getColFromId(i), me, temp.myPosition, temp.myPosition );

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
            //cout << "State m r c objFunction -> " << temp.m << " " << temp.r << " " << temp.c << " " << temp.objFunction << "\n";

//            check for TRICKY PATH BLOCK when my movement blocks other person's path
            if( temp.objFunction == INVALID_STATE ) {
//                cout << "the state mentioned above has been marked as TRICKY_PATH_BLOCK\n";
                temp.objFunction = TRICKY_PATH_BLOCK;
            }

            statesExplored++;
            children.push_back(temp);

            afterPush:

            delete t;

        } );

//        check for oscillating move, will have to go back to where I was
        if( !pathFound ) {

//            cout << "pushing to state\n";
            state *t = (new state(myPosition, opPosition, 0, myPosition.oldRow, myPosition.oldCol));
            state temp = *t;

//            last parameter is not really useful here
            temp.myPosition.update( 0, myPosition.oldRow, myPosition.oldCol, me, temp.myPosition, temp.opPosition );
            temp.opPosition.update( 0, myPosition.oldRow, myPosition.oldCol, me, temp.myPosition, temp.myPosition );

            temp.m = 0;
            temp.r = myPosition.oldRow;
            temp.c = myPosition.oldCol;
            temp.findObj();
            //cout << "State m r c objFunction -> " << temp.m << " " << temp.r << " " << temp.c << " " << temp.objFunction << "\n";
            statesExplored++;
            children.push_back(temp);

            delete t;
        }

    } else {

//        Opponent move
        for_each( opPosition.boardMap[opPosition.row][opPosition.col].adjList.begin(), opPosition.boardMap[opPosition.row][opPosition.col].adjList.end(), [&](int i) {

//            cout << "pushing to state\n";
            state *t = (new state(myPosition, opPosition, 0, tile::getRowFromId(i), tile::getColFromId(i)));
            state temp = *t;

//            last parameter is not really useful here
            temp.opPosition.update( 0, tile::getRowFromId(i), tile::getColFromId(i), op, temp.opPosition, temp.myPosition );
            temp.myPosition.update( 0, tile::getRowFromId(i), tile::getColFromId(i), op, temp.opPosition, temp.opPosition );

            temp.m = 0;
            temp.r = tile::getRowFromId(i);
            temp.c = tile::getColFromId(i);
            temp.findObj();
            //cout << "State m r c objFunction -> " << temp.m << " " << temp.r << " " << temp.c << " " << temp.objFunction << "\n";
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
                if( abs(opPosition.row - i) >= 4 || abs(opPosition.col - j) >= 4 )
                    continue;
            } else {
                if( abs(myPosition.row - i) >= 4 || abs(myPosition.col - j) >= 4 )
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

//                temp is the new state and this pointer is referring to current state
                state *t = (new state(myPosition, opPosition, 1, i, j));
                state temp = *t;

//                last parameter is meaningful here
                if( whoCalled == me ) {
                    temp.myPosition.update( 1, i, j, me, temp.myPosition, temp.opPosition );
                    temp.opPosition.update( 1, i, j, me, temp.myPosition, temp.myPosition );
                } else {
                    temp.myPosition.update( 1, i, j, op, temp.opPosition, temp.opPosition  );
                    temp.opPosition.update( 1, i, j, op, temp.opPosition, temp.myPosition  );
                }

                temp.m = 1;
                temp.r = i;
                temp.c = j;
                temp.findObj();

//                no path to goal in this state, check for another location
                if( temp.objFunction == INVALID_STATE ) {
                    delete t;
                    continue;
                }

//                here is the logic for pruning
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

//                cout << "pushing to state\n";
                //cout << "State m r c objFunction -> " << temp.m << " " << temp.r << " " << temp.c << " " << temp.objFunction << "\n";
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
                if( abs(opPosition.row - i) >= 4 || abs(opPosition.col - j) >= 4 )
                    continue;
            } else {
                if( abs(myPosition.row - i) >= 4 || abs(myPosition.col - j) >= 4 )
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

                state *t = (new state(myPosition, opPosition, 2, i, j));
                state temp = *t;

//                last parameter is meaningful here
                if( whoCalled == me ) {
                    temp.myPosition.update( 2, i, j, me, temp.myPosition, temp.opPosition );
                    temp.opPosition.update( 2, i, j, me, temp.myPosition, temp.myPosition );
                } else {
                    temp.myPosition.update( 2, i, j, op, temp.opPosition, temp.opPosition  );
                    temp.opPosition.update( 2, i, j, op, temp.opPosition, temp.myPosition  );
                }

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

//                cout << "pushing to state\n";
                //cout << "State m r c objFunction -> " << temp.m << " " << temp.r << " " << temp.c << " " << temp.objFunction << "\n";
                statesExplored++;
                children.push_back(temp);

                delete t;

            }
        }
    }

//    all states have been explored
    return children;
}

// this callerPlayer is the one whose turn invoked this player's updateMap
// whoCalled = me -> callerPlayer = myPosition
// whoCalled = op -> callerPlayer = opPosition
void playerPosition::updateMap( int m, int r, int c, who whoCalled, playerPosition callerPlayer, playerPosition otherPlayer ) {

//    me will update the map if op moved to take into account the tricky moves that me can make
//    AND VICE VERSA
    if( m == 0 && whoCalled != whoMI ) {

//        all ids and rows and cols here belong to whoCalled and NOT whoMI
        int id, row, col;
        int oldId, oldRow, oldCol;

        id = callerPlayer.id;
        row = callerPlayer.row;
        col = callerPlayer.col;
        oldId = callerPlayer.oldId;
        oldRow = callerPlayer.oldRow;
        oldCol = callerPlayer.oldCol;

//        cin.ignore();

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
//                    in the beginning, oldRow and row has problems, hence id+M<MAX_ID is placed
//                    else if current location has no wall to left or current location is not at 1st col
//                         if current location has no wall to righ or current location is not at Mth col
                    if( (find( newPosAdjList.begin(), newPosAdjList.end(), id+M ) != newPosAdjList.end() || (oldRow==row+1)) && id+M<MAX_ID ) {
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

//        cell towards top left of the r,c
        for_each( boardMap[r-1][c-1].adjList.begin(), boardMap[r-1][c-1].adjList.end(), [&](int i) {
//            if any of the 4 possibility occurs, don't push it because of the wall in between
            if( i != boardMap[r-1][c-1].id+M && i != boardMap[r-1][c-1].id+M+M && i != boardMap[r-1][c-1].id+M-1 && i != boardMap[r-1][c-1].id+M+1 )
                temp.push_back(i);
        });
        boardMap[r-1][c-1].adjList = temp;
        temp.clear();

//        cell towards top right of the r,c
        for_each( boardMap[r-1][c].adjList.begin(), boardMap[r-1][c].adjList.end(), [&](int i) {
//            if any of the 4 possibility occurs, don't push it because of the wall in between
            if( i != boardMap[r-1][c].id+M && i != boardMap[r-1][c].id+M+M && i != boardMap[r-1][c].id+M-1 && i != boardMap[r-1][c].id+M+1 )
                temp.push_back(i);
        });
        boardMap[r-1][c].adjList = temp;
        temp.clear();

//        cell towards bottom left of the r,c
        for_each( boardMap[r][c-1].adjList.begin(), boardMap[r][c-1].adjList.end(), [&](int i) {
//            if any of the 4 possibility occurs, don't push it because of the wall in between
            if( i != boardMap[r][c-1].id-M && i != boardMap[r][c-1].id-M-M && i != boardMap[r][c-1].id-M-1 && i != boardMap[r][c-1].id-M+1 )
                temp.push_back(i);
        });
        boardMap[r][c-1].adjList = temp;
        temp.clear();

//        cell towards bottom right of the r,c
        for_each( boardMap[r][c].adjList.begin(), boardMap[r][c].adjList.end(), [&](int i) {
//            if any of the 4 possibility occurs, don't push it because of the wall in between
            if( i != boardMap[r][c].id-M && i != boardMap[r][c].id-M-M && i != boardMap[r][c].id-M-1 && i != boardMap[r][c].id-M+1 )
                temp.push_back(i);
        });
        boardMap[r][c].adjList = temp;

//        if my opponent is standing on any of the tl tr bl or br, I will adjust the adjacency list of the
//        cell whose over jumping is now obstructed by the new wall here at r,c

//        no matter who placed the wall, I will check for the overjumping for this player's opponent
        int id = otherPlayer.id;

//        consider adjList for cell above tl of r,c
        if( id == tile::getIdFromRowCol(r-1,c-1) && id-M>0) {
            if( find( boardMap[r-2][c-1].adjList.begin(), boardMap[r-2][c-1].adjList.end(), id+M) != boardMap[r-2][c-1].adjList.end() ) {
                vector<int> oldAdjList = boardMap[r-2][c-1].adjList;
                temp.clear();

//                if id has id-1 or id+1 as its neighbour, adjust the adjList of the cell above to consider L moves
                for_each( oldAdjList.begin(), oldAdjList.end(), [&](int i){
                    if( i != id+M )
                        temp.push_back(i);
                    else {
                        if( find( boardMap[r-1][c-1].adjList.begin(), boardMap[r-1][c-1].adjList.end(), id-1) != boardMap[r-1][c-1].adjList.end() )
                            temp.push_back(id-1);
                        if( find( boardMap[r-1][c-1].adjList.begin(), boardMap[r-1][c-1].adjList.end(), id+1) != boardMap[r-1][c-1].adjList.end() )
                            temp.push_back(id+1);
                    }
                });
                boardMap[r-2][c-1].adjList = temp;
            }
        }

//        consider adjList for cell above tr of r,c
        if( id == tile::getIdFromRowCol(r-1,c) && id-M>0) {
//            cout << "id == tile::getIdFromRowCol(r-1,c)\n";
            if( find( boardMap[r-2][c].adjList.begin(), boardMap[r-2][c].adjList.end(), id+M) != boardMap[r-2][c].adjList.end() ) {
                vector<int> oldAdjList = boardMap[r-2][c].adjList;
                temp.clear();

//                if id has id-1 or id+1 as its neighbour, adjust the adjList of the cell above to consider L moves
                for_each( oldAdjList.begin(), oldAdjList.end(), [&](int i){
                    if( i != id+M )
                        temp.push_back(i);
                    else {
                        if( find( boardMap[r-1][c].adjList.begin(), boardMap[r-1][c].adjList.end(), id-1) != boardMap[r-1][c].adjList.end() )
                            temp.push_back(id-1);
                        if( find( boardMap[r-1][c].adjList.begin(), boardMap[r-1][c].adjList.end(), id+1) != boardMap[r-1][c].adjList.end() )
                            temp.push_back(id+1);
                    }
                });
                boardMap[r-2][c].adjList = temp;
            }
        }

//        consider adjList for cell below bl of r,c
        if( id == tile::getIdFromRowCol(r,c-1) && id+M<MAX_ID ) {
            if( find( boardMap[r+1][c-1].adjList.begin(), boardMap[r+1][c-1].adjList.end(), id-M) != boardMap[r+1][c-1].adjList.end() ) {
                vector<int> oldAdjList = boardMap[r+1][c-1].adjList;
                temp.clear();

//                if id has id-1 or id+1 as its neighbour, adjust the adjList of the cell above to consider L moves
                for_each( oldAdjList.begin(), oldAdjList.end(), [&](int i){
                    if( i != id-M )
                        temp.push_back(i);
                    else {
                        if( find( boardMap[r][c-1].adjList.begin(), boardMap[r][c-1].adjList.end(), id-1) != boardMap[r][c-1].adjList.end() )
                            temp.push_back(id-1);
                        if( find( boardMap[r][c-1].adjList.begin(), boardMap[r][c-1].adjList.end(), id+1) != boardMap[r][c-1].adjList.end() )
                            temp.push_back(id+1);
                    }
                });
                boardMap[r+1][c-1].adjList = temp;
            }
        }

//        consider adjList for cell above tl of r,c
        if( id == tile::getIdFromRowCol(r,c) && id+M<MAX_ID ) {
            if( find( boardMap[r+1][c].adjList.begin(), boardMap[r+1][c].adjList.end(), id-M) != boardMap[r+1][c].adjList.end() ) {
                vector<int> oldAdjList = boardMap[r+1][c].adjList;
                temp.clear();

//                if id has id-1 or id+1 as its neighbour, adjust the adjList of the cell above to consider L moves
                for_each( oldAdjList.begin(), oldAdjList.end(), [&](int i){
                    if( i != id-M )
                        temp.push_back(i);
                    else {
                        if( find( boardMap[r][c].adjList.begin(), boardMap[r][c].adjList.end(), id-1) != boardMap[r][c].adjList.end() )
                            temp.push_back(id-1);
                        if( find( boardMap[r][c].adjList.begin(), boardMap[r][c].adjList.end(), id+1) != boardMap[r][c].adjList.end() )
                            temp.push_back(id+1);
                    }
                });
                boardMap[r+1][c].adjList = temp;
            }
        }

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

//        if my opponent is standing on any of the tl tr bl or br, I will adjust the adjacency list of the
//        cell whose over jumping is now obstructed by the new wall here at r,c

//        no matter who placed the wall, I will check for the overjumping for this player's opponent
        int id = otherPlayer.id;

//        consider adjList for cell to left to tl of r,c
        if( id == tile::getIdFromRowCol(r-1,c-1) && c-2 > 0 ) {
            if( find( boardMap[r-1][c-2].adjList.begin(), boardMap[r-1][c-2].adjList.end(), id+1) != boardMap[r-1][c-2].adjList.end() ) {
                vector<int> oldAdjList = boardMap[r-1][c-2].adjList;
                temp.clear();

//                if id has id-M or id+M as its neighbour, adjust the adjList of the cell to left to consider L moves
                for_each( oldAdjList.begin(), oldAdjList.end(), [&](int i){
                    if( i != id+1 )
                        temp.push_back(i);
                    else {
                        if( find( boardMap[r-1][c-1].adjList.begin(), boardMap[r-1][c-1].adjList.end(), id-M) != boardMap[r-1][c-1].adjList.end() )
                            temp.push_back(id-M);
                        if( find( boardMap[r-1][c-1].adjList.begin(), boardMap[r-1][c-1].adjList.end(), id+M) != boardMap[r-1][c-1].adjList.end() )
                            temp.push_back(id+M);
                    }
                });
                boardMap[r-1][c-2].adjList = temp;
            }
        }

//        consider adjList for cell to left to bl of r,c
        if( id == tile::getIdFromRowCol(r,c-1) && c-2 > 0 ) {
            if( find( boardMap[r][c-2].adjList.begin(), boardMap[r][c-2].adjList.end(), id+1) != boardMap[r][c-2].adjList.end() ) {
                vector<int> oldAdjList = boardMap[r][c-2].adjList;
                temp.clear();

//                if id has id-M or id+M as its neighbour, adjust the adjList of the cell to left to consider L moves
                for_each( oldAdjList.begin(), oldAdjList.end(), [&](int i){
                    if( i != id+1 )
                        temp.push_back(i);
                    else {
                        if( find( boardMap[r][c-1].adjList.begin(), boardMap[r][c-1].adjList.end(), id-M) != boardMap[r][c-1].adjList.end() )
                            temp.push_back(id-M);
                        if( find( boardMap[r][c-1].adjList.begin(), boardMap[r][c-1].adjList.end(), id+M) != boardMap[r][c-1].adjList.end() )
                            temp.push_back(id+M);
                    }
                });
                boardMap[r][c-2].adjList = temp;
            }
        }

//        consider adjList for cell to right to tr of r,c
        if( id == tile::getIdFromRowCol(r-1,c) && c+1 < M+1 ) {
            if( find( boardMap[r-1][c+1].adjList.begin(), boardMap[r-1][c+1].adjList.end(), id-1) != boardMap[r-1][c+1].adjList.end() ) {
                vector<int> oldAdjList = boardMap[r-1][c+1].adjList;
                temp.clear();

//                if id has id-M or id+M as its neighbour, adjust the adjList of the cell to left to consider L moves
                for_each( oldAdjList.begin(), oldAdjList.end(), [&](int i){
                    if( i != id-1 )
                        temp.push_back(i);
                    else {
                        if( find( boardMap[r-1][c].adjList.begin(), boardMap[r-1][c].adjList.end(), id-M) != boardMap[r-1][c].adjList.end() )
                            temp.push_back(id-M);
                        if( find( boardMap[r-1][c].adjList.begin(), boardMap[r-1][c].adjList.end(), id+M) != boardMap[r-1][c].adjList.end() )
                            temp.push_back(id+M);
                    }
                });
                boardMap[r-1][c+1].adjList = temp;
            }
        }

//        consider adjList for cell to right to br of r,c
        if( id == tile::getIdFromRowCol(r,c) && c+1 < M+1 ) {
            if( find( boardMap[r][c+1].adjList.begin(), boardMap[r][c+1].adjList.end(), id-1) != boardMap[r][c+1].adjList.end() ) {
                vector<int> oldAdjList = boardMap[r][c+1].adjList;
                temp.clear();

//                if id has id-M or id+M as its neighbour, adjust the adjList of the cell to left to consider L moves
                for_each( oldAdjList.begin(), oldAdjList.end(), [&](int i){
                    if( i != id-1 )
                        temp.push_back(i);
                    else {
                        if( find( boardMap[r][c].adjList.begin(), boardMap[r][c].adjList.end(), id-M) != boardMap[r][c].adjList.end() )
                            temp.push_back(id-M);
                        if( find( boardMap[r][c].adjList.begin(), boardMap[r][c].adjList.end(), id+M) != boardMap[r][c].adjList.end() )
                            temp.push_back(id+M);
                    }
                });
                boardMap[r][c+1].adjList = temp;
            }
        }

    }
//    cout << "Map has been updated" << endl;
}

state minVal(state current, int alpha, int beta, int depth, int fatherObj){

    current.findObj();

    if( depth > cutoffDepth ) {
//        cout << "minVal no children\n";
        current.utilityFunction = current.objFunction;
        return current;
    }

    //cout << "minVal called and ply and current mrc of the caller " << 2*depth << " " << current.m << " "  << current.r;
    //cout << " " << current.c << endl;

    statesProcessed++;

    vector<state> children;
    children = current.exploreStates( op );
    current.branchingFactor = children.size();

    if( cutoffDepth > 1 && current.branchingFactor > 10 && TL < 25 )
        cutoffDepth = 1;

//    cout << "cutoffDepth and branching factor is " << cutoffDepth << " " << current.branchingFactor << endl;

    if( children.size() == 0 ) {
        //cout << "minVal no children\n";
        current.utilityFunction = PASS;
        return current;
    }

    int min = INT_MAX;
    state minState;

    for( vecSIt it = children.begin(); (it != children.end()) && !(beta <= alpha); it++) {

        state s = *it;
        s.findObj();

        //cout << "Before mrc " << s.m << " " << s.r << " " << s.c << ", alpha beta are " << alpha << " " << beta << endl;

//        cout << "\ns.objFunction s.myPosition.findShortestPath() current.myPosition.findShortestPath() " << s.objFunction << " " << s.myPosition.findShortestPath() << " " << current.myPosition.findShortestPath() << endl;
//        cout << "\nm r c " << s.m << " " << s.r << " " << s.c << endl;

//        this child is blocking opponent i.e. TRICKY_PATH_BLOCK and I was not here before, then come here for sure
        if( s.objFunction == TRICKY_PATH_BLOCK && s.opPosition.findShortestPath() < current.opPosition.findShortestPath() ) {
            s.utilityFunction = -INT_MAX;
            return s;
        }

//        my children will maximize their objective function
        state nextState = maxVal( s, alpha, beta, depth+1, s.objFunction );
//        mrc of s's children has been appended to s's ms rs and cs
        s.ms.insert( s.ms.end(), nextState.ms.begin(), nextState.ms.end());
        s.rs.insert( s.rs.end(), nextState.rs.begin(), nextState.rs.end());
        s.cs.insert( s.cs.end(), nextState.cs.begin(), nextState.cs.end());

//        if the other player PASSED, objective function of s is its utility function
//        else, do normal gaming
        if( nextState.utilityFunction == PASS ) {
            s.utilityFunction = s.objFunction;
//            cout << "\n\n\n\n\ns m r c " << s.m << " " << s.r << " " << s.c << endl;

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
//            tie breaking and favour that child whose objective function is lower
            if( min == nextState.utilityFunction && s.objFunction < minState.objFunction ) {
                minState = s;
                minState.utilityFunction = nextState.utilityFunction;
            }
        }
        //cout << "After mrc " << s.m << " " << s.r << " " << s.c << ", alpha beta are " << alpha << " " << beta << endl;
    }

    children.clear();

    //cout << "minVal exiting and ply and current mrc of the caller " << 2*depth << " " << current.m << " "  << current.r;
    //cout << " " << current.c << endl;
    //cout << "the mrc of best child and utilityfunction obtained " << minState.m << " "  << minState.r << " "  << minState.c << " " << minState.utilityFunction << endl;

    return minState;
}

// depth is the depth of the minimax tree
// I will find max out of my children and all my children will find min out of their children
// it will comprise of one depth
state maxVal(state current, int alpha, int beta, int depth, int fatherObj){

    statesProcessed++;

    current.findObj();

    if( depth > cutoffDepth ) {
        current.utilityFunction = current.objFunction;
        return current;
    }

    //cout << "maxVal called and ply and current mrc of the caller " << 2*depth-1 << " " << current.m << " "  << current.r;
    //cout << " " << current.c << endl;


    vector<state> children;
    children = current.exploreStates( me );
    current.branchingFactor = children.size();

    if( cutoffDepth > 1 && current.branchingFactor > 10 && TL < 25 )
        cutoffDepth = 1;

//    cout << "cutoffDepth and branching factor is " << cutoffDepth << " " << current.branchingFactor << endl;

    if( current.branchingFactor > 2 ) {
        totalExpandedNodes++;
        cumulativeBranchingFactor += current.branchingFactor;
        if( maxBranchingFactor < current.branchingFactor )
            maxBranchingFactor = current.branchingFactor;
    }

    if( children.size() == 0 ) {
        //cout << "maxVal no children\n";
        current.utilityFunction = PASS;
        return current;
    }

    int max = -INT_MAX;
    state maxState;

//    for each children  of the caller
    for( vecSIt it = children.begin(); (it != children.end()) && !(beta <= alpha); it++) {

//        cout << "child number " << it - children.begin() << endl;

        state s = *it;

//        this child is blocking opponent i.e. TRICKY_PATH_BLOCK and I was not here before, then come here for sure
        if( s.objFunction == TRICKY_PATH_BLOCK && s.myPosition.findShortestPath() < current.myPosition.findShortestPath() ) {
            s.utilityFunction = +INT_MAX;
            return s;
        }

        s.findObj();

//        cout << "\ns.objFunction s.myPosition.findShortestPath() current.myPosition.findShortestPath() " << s.objFunction << " " << s.myPosition.findShortestPath() << " " << current.myPosition.findShortestPath() << endl;
//        cout << "\nm r c " << s.m << " " << s.r << " " << s.c << endl;

//        my children will minimize their objective function
        state nextState = minVal( s, alpha, beta, depth, s.objFunction );
//        mrc of s's children has been appended to s's ms rs and cs
        s.ms.insert( s.ms.end(), nextState.ms.begin(), nextState.ms.end());
        s.rs.insert( s.rs.end(), nextState.rs.begin(), nextState.rs.end());
        s.cs.insert( s.cs.end(), nextState.cs.begin(), nextState.cs.end());
//        cout << "insert\n";
//        if the other player PASSED, objective function of s is its utility function
//        else, do normal gaming
        if( nextState.utilityFunction == PASS ) {
            s.utilityFunction = s.objFunction;

            if( max < s.utilityFunction ) {
                max = s.utilityFunction;
                maxState = s;
                expectedMove.m=nextState.m;
                expectedMove.r=nextState.r;
                expectedMove.c=nextState.c;
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
                expectedMove.m=nextState.m;
                expectedMove.r=nextState.r;
                expectedMove.c=nextState.c;
            }

//            utility function is same
//            tie breaking and favour that child whose objective function is higher
            if( max == nextState.utilityFunction && maxState.objFunction < s.objFunction ) {
                maxState = s;
                maxState.utilityFunction = nextState.utilityFunction;
                expectedMove.m=nextState.m;
                expectedMove.r=nextState.r;
                expectedMove.c=nextState.c;
            }
        }

    }

    children.clear();

    //cout << "maxVal exiting and ply and current mrc of the caller " << 2*depth-1 << " " << current.m << " "  << current.r;
    //cout << " " << current.c << endl;
    //cout << "the mrc of best child and utilityfunction obtained " << maxState.m << " "  << maxState.r << " "  << maxState.c << " " << maxState.utilityFunction << endl;

    return maxState;
}

void playerPosition::minimax( playerPosition myPosition, playerPosition opPosition ) {

    time_t beg = clock();

    state start = *(new state(myPosition, opPosition, 0, 0, 0));
    start.findObj();

    if( TL < 7.0 )
        cutoffDepth = 1;
    else
        cutoffDepth = STANDARD_CUTOFF_DEPTH;

    state nextState = maxVal( start, -INT_MAX, INT_MAX, 1, start.objFunction );

//    pass
    if( nextState.utilityFunction == PASS ) {
        this->m = 0;
        this->r = 0;
        this->c = 0;
        return;
    }

    totalMinimaxCalled++;

    //cout << "playerPosition::minimax( playerPosition myPosition, playerPosition opPosition ) found nextState with utility Function - " << nextState.utilityFunction << "\n";
//    cout << "nextState's path is - \n";
//    nextState.myPosition.printPath();
//    nextState.opPosition.printPath();

//    this is the motive of minimax
    this->m = nextState.m;
    this->r = nextState.r;
    this->c = nextState.c;

    //cout << "myPosition.minimax( myPosition, opPosition ) gave m r c - " << m << " " << r << " " << c << endl;
    nextState.printStateStats();

    time_t end = clock();

    //cout << "\n\nTime taken is - " << float(end - beg)/CLOCKS_PER_SEC << endl;

    if( maxMinimaxTime < float(end - beg)/CLOCKS_PER_SEC )
        maxMinimaxTime = float(end - beg)/CLOCKS_PER_SEC;
    cumulativeMinimaxTime += float(end - beg)/CLOCKS_PER_SEC;

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
    int om,oro,oco;
    int m,r,c;
    int d=3;
    char s[100];
    int x=1;

//    start the game NOW
//    our code starts here

    myPosition = *( new playerPosition( 1, (M+1)/2, false ) );
    opPosition = *( new playerPosition( N, (M+1)/2, true ) );

    myPosition.updateMap( 0, N, (M+1)/2, op, opPosition, opPosition);
    opPosition.updateMap( 0, 1, (M+1)/2, me, myPosition, myPosition);

//    both player 1 and player 2 will look at the game from the same perspective
//    there will be a few mapping for player 2 to interpret the opponent's move and my move
//    both players have to reach id (M*(N-1)+1) to M*N i.e. the last row

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

        myPosition.update( m, r, c, me, myPosition, opPosition );
        opPosition.update( m, r, c, me, myPosition, myPosition );

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
        turnNumber++;

        cout << "Inside while and " << turnNumber << "th turn\n";
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
        cout<<"Expected move "<<expectedMove.m <<" "<<expectedMove.r<<" "<<expectedMove.c<<endl;
        cout<<"Weights "<<moveWt <<" "<<horizontalWallWt<<" "<<verticalWallWt<<endl;
//        expectedMove.updateWeight(om,oro,oco);
        cout<<"Weights "<<moveWt <<" "<<horizontalWallWt<<" "<<verticalWallWt<<endl;
//        cin.ignore();
//        cout << "BEFORE UPDATE\n";
//        cout << "\nMy adj list before update\n";
//        myPosition.printAdjList();
//        cout << "Op adj list before update\n";
//        opPosition.printAdjList();

//        cin.ignore();

//        the last parameter is used to handle jumpover adjustments if a wall is placed and a person is already standing
//        adjacent to it
        opPosition.update( om, oro, oco, op, opPosition, myPosition );
        myPosition.update( om, oro, oco, op, opPosition, opPosition );

//        cout << "\n\n----------------------------------------------------------------------------------------\n\n";

//        cout << "AFTER UPDATE\n";
//        cout << "My adj list after update\n";
//        myPosition.printAdjList();
//        cout << "Op adj list after update\n";
//        opPosition.printAdjList();

        opPosition.declareWinner();

//        opponent's move blocked me
        if( myPosition.checkPathBlock( oro, oco) == true && om == 0 ) {
//            cin.ignore();
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

//        cout << "\nMy adj list before update\n";
//        myPosition.printAdjList();
//        cout << "Op adj list before update\n";
//        opPosition.printAdjList();

//        the last parameter is used to handle jumpover adjustments if a wall is placed and a person is already standing
//        adjacent to it
        myPosition.update( m, r, c, me, myPosition, opPosition );
        opPosition.update( m, r, c, me, myPosition, myPosition );

//        cout << "My adj list after update\n";
//        myPosition.printAdjList();
//        cout << "Op adj list after update\n";
//        opPosition.printAdjList();

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

        cout << "Waiting for you to press something so that I can send m r c as " << m << " " << r << " " << c << "\n";

//        cin.ignore();

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

    avgBranchingFactor = totalExpandedNodes / cumulativeBranchingFactor;
    avgMinimaxTime     = totalMinimaxCalled / cumulativeMinimaxTime;

    cout << "\n\n";

    cout << "Total nodes whose children has been seen    - " << totalExpandedNodes        << endl;
    cout << "Total time minimax called with high bf      - " << totalMinimaxCalled        << endl;
    cout << "Average minimax time                        - " << avgMinimaxTime            << endl;
    cout << "Average branching factor                    - " << avgBranchingFactor        << endl;
    cout << "Average minimax time                        - " << avgMinimaxTime            << endl;
    cout << "Maximum branching factor                    - " << maxBranchingFactor        << endl;
    cout << "Maximum minimax time                        - " << maxMinimaxTime            << endl;
    cout << "Cumulative branching factor                 - " << cumulativeBranchingFactor << endl;
    cout << "Cumulative minimax time                     - " << cumulativeMinimaxTime     << endl;
    cout << "Time left                                   - " << TL                        << endl;
    cout << "weights                                     - " << moveWt<<" "<<horizontalWallWt<<" "<<verticalWallWt<<endl;
    cout << endl << "The End" << endl;
//    cin.ignore();
    return 0;
}
