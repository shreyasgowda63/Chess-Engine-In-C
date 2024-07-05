// perft.c

#include "defs.h"
#include "stdio.h"

long leafNodes;//this is the number of leaf nodes
//Perft testing is a procedure to debug the move generation logic in a chess engine. 
//The term "Perft" stands for "Performance Test". It involves generating all possible moves from a given position up to a certain depth and counting them.
void Perft(int depth, S_BOARD *pos) {//this is the perft function
//depth is the depth up to which we want to generate moves. For example, if depth is 2, we generate all possible moves for the current position, 
//then for each of these moves, we generate all possible responses.

//Here's an example:

// Let's say the current position is the starting position of a chess game. The current player (white) has 20 possible moves (16 pawn moves and 4 knight moves).

// For each of these 20 moves, we then generate all legal responses by black. Let's say, for simplicity, that black also has 20 legal responses to each of white's moves.

// Therefore, the total number of move sequences of depth 2 is 20 (white's moves) * 20 (black's responses) = 400
    ASSERT(CheckBoard(pos));  

	if(depth == 0) {//if the depth is 0
        leafNodes++;//increment the leaf nodes
        return;//return
    }	

    S_MOVELIST list[1];//create a move list which consist of a list of moves
    GenerateAllMoves(pos,list);//generate all the moves in that position
      
    int MoveNum = 0;
	for(MoveNum = 0; MoveNum < list->count; ++MoveNum) {//for all the moves in the list	
       
        if ( !MakeMove(pos,list->moves[MoveNum].move))  {//if the move is not legal 
            continue;//continue
        }
        Perft(depth - 1, pos);//call the perft function with a depth of depth - 1
        TakeMove(pos);//take the move when the function returns
    }

    return;//return
}


void PerftTest(int depth, S_BOARD *pos) {

    ASSERT(CheckBoard(pos));

	PrintBoard(pos);
	printf("\nStarting Test To Depth:%d\n",depth);	
	leafNodes = 0;
	int start = GetTimeMs();
    S_MOVELIST list[1];
    GenerateAllMoves(pos,list);	
    
    int move;	    
    int MoveNum = 0;
	for(MoveNum = 0; MoveNum < list->count; ++MoveNum) {
        move = list->moves[MoveNum].move;
        if ( !MakeMove(pos,move))  {
            continue;
        }
        long cumnodes = leafNodes;
        Perft(depth - 1, pos);
        TakeMove(pos);        
        long oldnodes = leafNodes - cumnodes;
        printf("move %d : %s : %ld\n",MoveNum+1,PrMove(move),oldnodes);
    }
	
	printf("\nTest Complete : %ld nodes visited in %dms\n",leafNodes,GetTimeMs() - start);

    return;
}












