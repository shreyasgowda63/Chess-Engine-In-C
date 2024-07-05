// io.c

#include "stdio.h"
#include "defs.h"

char *PrSq(const int sq) {

	static char SqStr[3];

	int file = FilesBrd[sq];
	int rank = RanksBrd[sq];

	sprintf(SqStr, "%c%c", ('a'+file), ('1'+rank));

	return SqStr;

}

char *PrMove(const int move) {

	static char MvStr[6];

	int ff = FilesBrd[FROMSQ(move)];
	int rf = RanksBrd[FROMSQ(move)];
	int ft = FilesBrd[TOSQ(move)];
	int rt = RanksBrd[TOSQ(move)];

	int promoted = PROMOTED(move);

	if(promoted) {
		char pchar = 'q';
		if(IsKn(promoted)) {
			pchar = 'n';
		} else if(IsRQ(promoted) && !IsBQ(promoted))  {
			pchar = 'r';
		} else if(!IsRQ(promoted) && IsBQ(promoted))  {
			pchar = 'b';
		}
		sprintf(MvStr, "%c%c%c%c%c", ('a'+ff), ('1'+rf), ('a'+ft), ('1'+rt), pchar);
	} else {
		sprintf(MvStr, "%c%c%c%c", ('a'+ff), ('1'+rf), ('a'+ft), ('1'+rt));
	}

	return MvStr;
}

int ParseMove(char *ptrChar, S_BOARD *pos) {
	//this is the function that parses the move from the input
	ASSERT(CheckBoard(pos));//assert that the board is valid

	if(ptrChar[1] > '8' || ptrChar[1] < '1') return NOMOVE;//if the second character is not a valid rank return no move
    if(ptrChar[3] > '8' || ptrChar[3] < '1') return NOMOVE;//if the fourth character is not a valid rank return no move
    if(ptrChar[0] > 'h' || ptrChar[0] < 'a') return NOMOVE;//if the first character is not a valid file return no move
    if(ptrChar[2] > 'h' || ptrChar[2] < 'a') return NOMOVE;//if the third character is not a valid file return no move

    int from = FR2SQ(ptrChar[0] - 'a', ptrChar[1] - '1');
	//get the from square from the first two characters
    int to = FR2SQ(ptrChar[2] - 'a', ptrChar[3] - '1');
	//get the to square from the last two characters

	ASSERT(SqOnBoard(from) && SqOnBoard(to));//assert that the squares are on the board

	S_MOVELIST list[1];//create a move list to store the moves
    GenerateAllMoves(pos,list);//generate all the moves in the position
    int MoveNum = 0;//the move number
	int Move = 0;
	int PromPce = EMPTY;//to get the promoted peice 

	for(MoveNum = 0; MoveNum < list->count; ++MoveNum) {//traverse for all the moves in the list
		Move = list->moves[MoveNum].move;//get the move
		if(FROMSQ(Move)==from && TOSQ(Move)==to) {//if the from square and to square match
			PromPce = PROMOTED(Move);//get the promoted piece
			if(PromPce!=EMPTY) {//if the promoted piece is not empty
				if(IsRQ(PromPce) && !IsBQ(PromPce) && ptrChar[4]=='r') {//if it is rook and not bishop and the fifth character is r which is the promoted piece rook
					return Move;//return the move
				} else if(!IsRQ(PromPce) && IsBQ(PromPce) && ptrChar[4]=='b') {//if it is bishop and not rook and the fifth character is b which is the promoted piece bishop
					return Move;
				} else if(IsRQ(PromPce) && IsBQ(PromPce) && ptrChar[4]=='q') {//if it is rook,queen and bishop,queen and the fifth character is q which is the promoted piece queen
					return Move;
				} else if(IsKn(PromPce)&& ptrChar[4]=='n') {//if it is knight and the fifth character is n which is the promoted piece knight
					return Move;//return the move
				}
				continue;
			}
			return Move;// if no promotion return the created move
		}
    }

    return NOMOVE;//return no move if the move is not found
}

void PrintMoveList(const S_MOVELIST *list) {
	int index = 0;
	int score = 0;
	int move = 0;
	printf("MoveList:\n");

	for(index = 0; index < list->count; ++index) {

		move = list->moves[index].move;
		score = list->moves[index].score;

		printf("Move:%d > %s (score:%d)\n",index+1,PrMove(move),score);
	}
	printf("MoveList Total %d Moves:\n\n",list->count);
}














