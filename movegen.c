// movegen.c

#include "stdio.h"
#include "defs.h"
//Move ordering in the context of a chess engine refers to the sequence in which potential moves are evaluated during the search process.
#define MOVE(f,t,ca,pro,fl) ( (f) | ((t) << 7) | ( (ca) << 14 ) | ( (pro) << 20 ) | (fl))
#define SQOFFBOARD(sq) (FilesBrd[(sq)]==OFFBOARD)

const int LoopSlidePce[8] = {//loop slide pieces like the rook, queen, bishop
 wB, wR, wQ, 0, bB, bR, bQ, 0
};

const int LoopNonSlidePce[6] = {//loop non slide pieces like the knight, king
 wN, wK, 0, bN, bK, 0
};

const int LoopSlideIndex[2] = { 0, 4 };//loop slide index starts at 0 for white and 4 for black
const int LoopNonSlideIndex[2] = { 0, 3 };

const int PceDir[13][8] = {// this gives us the direction of movement for each pawn type
	{ 0, 0, 0, 0, 0, 0, 0 }, //  EMPTY
	{ 0, 0, 0, 0, 0, 0, 0 },//wp
	{ -8, -19,	-21, -12, 8, 19, 21, 12 },//wN
	{ -9, -11, 11, 9, 0, 0, 0, 0 },//wB
	{ -1, -10,	1, 10, 0, 0, 0, 0 },//wR
	{ -1, -10,	1, 10, -9, -11, 11, 9 },//wQ
	{ -1, -10,	1, 10, -9, -11, 11, 9 },//wK
	{ 0, 0, 0, 0, 0, 0, 0 },//bP
	{ -8, -19,	-21, -12, 8, 19, 21, 12 },//bN
	{ -9, -11, 11, 9, 0, 0, 0, 0 },//bB
	{ -1, -10,	1, 10, 0, 0, 0, 0 },//bR
	{ -1, -10,	1, 10, -9, -11, 11, 9 },//bQ
	{ -1, -10,	1, 10, -9, -11, 11, 9 }//bK
};

const int NumDir[13] = {
 0, 0, 8, 4, 4, 8, 8, 0, 8, 4, 4, 8, 8
};

/*
PV Move
Cap -> MvvLVA
Killers
HistoryScore

*/
const int VictimScore[13] = { 0, 100, 200, 300, 400, 500, 600, 100, 200, 300, 400, 500, 600 };
//this is the score that we get for capturing a piece of a certain type
static int MvvLvaScores[13][13];
//this is the most valuable victim least valuable attacker score
void InitMvvLva() {
	int Attacker;
	int Victim;
	for(Attacker = wP; Attacker <= bK; ++Attacker) {
		for(Victim = wP; Victim <= bK; ++Victim) {
			MvvLvaScores[Victim][Attacker] = VictimScore[Victim] + 6 - ( VictimScore[Attacker] / 100);
			//the score is the score of the victim plus 6 minus the score of the attacker divided by 100
		}
	}
}

int MoveExists(S_BOARD *pos, const int move) {//check if the move exists in the position

	S_MOVELIST list[1];//create a move list
    GenerateAllMoves(pos,list);//generate all the moves in the position and store them in the list

    int MoveNum = 0;//the move number
	for(MoveNum = 0; MoveNum < list->count; ++MoveNum) {//traverse for all the moves in the list

        if ( !MakeMove(pos,list->moves[MoveNum].move))  {//if u can't make the move continue
            continue;
        }
        TakeMove(pos);//take the move back
		if( list->moves[MoveNum].move == move) {//if the move is found return true
			return TRUE;//return true
		}
    }
	return FALSE; //return false if the move is not found
}
// The AddQuietMove function in the provided 
// code snippet is designed to add non-capturing (quiet) moves to a move list within a chess engine
// This array is used to store "killer moves" for each ply (depth) of the search. Killer moves are quiet moves (non-captures) that
// have caused a beta cutoff in the search algorithm at a certain depth but are not captures or promotions.
// Imagine a scenario in a chess game where the engine is evaluating moves at a certain depth (ply). It encounters a quiet move (let's say moving a knight to an unoccupied square) that unexpectedly leads to a favorable position, causing a beta cutoff. This move is not a capture or a promotion, so its intrinsic value isn't immediately apparent from the board position alone. 
// The engine stores this move in the searchKillers array for the current ply.
// Later, when the engine is evaluating other positions at the same depth, it checks if any potential quiet move matches a move stored in the searchKillers array. If there's a match, this move is given a high score (900000 for the first killer move, 800000 for the second), ensuring it's evaluated early in the move ordering process. This is because the move has previously led to a beta cutoff, 
// indicating it's likely a strong move that could lead to favorable outcomes.
static void AddQuietMove(const S_BOARD *pos,int move, S_MOVELIST *list ) {

	ASSERT(SqOnBoard(FROMSQ(move)));
	ASSERT(SqOnBoard(TOSQ(move)));
	ASSERT(CheckBoard(pos));
	ASSERT(pos->ply >=0 && pos->ply < MAXDEPTH);

	list->moves[list->count].move = move;
	//list->count: This is an integer value that keeps track of how many moves have been added to list->moves. It serves both as a count of the moves and as the index for the next move to be inserted.
	// Before a move is added, list->count points to the next available slot in the moves array.
	if(pos->searchKillers[0][pos->ply] == move) {
		list->moves[list->count].score = 900000;
	} else if(pos->searchKillers[1][pos->ply] == move) {
		list->moves[list->count].score = 800000;
	} else {
		list->moves[list->count].score = pos->searchHistory[pos->pieces[FROMSQ(move)]][TOSQ(move)];
	}
	list->count++;//increment the count which is the number of moves
}

static void AddCaptureMove( const S_BOARD *pos, int move, S_MOVELIST *list ) {

	ASSERT(SqOnBoard(FROMSQ(move)));
	ASSERT(SqOnBoard(TOSQ(move)));
	ASSERT(PieceValid(CAPTURED(move)));
	ASSERT(CheckBoard(pos));

	list->moves[list->count].move = move;
	list->moves[list->count].score = MvvLvaScores[CAPTURED(move)][pos->pieces[FROMSQ(move)]] + 1000000;
	list->count++;
}

static void AddEnPassantMove( const S_BOARD *pos, int move, S_MOVELIST *list ) {

	ASSERT(SqOnBoard(FROMSQ(move)));//check if the from square is on the board
	ASSERT(SqOnBoard(TOSQ(move)));//check if the to square is on the board
	ASSERT(CheckBoard(pos));//check if the board is valid
	ASSERT((RanksBrd[TOSQ(move)]==RANK_6 && pos->side == WHITE) || (RanksBrd[TOSQ(move)]==RANK_3 && pos->side == BLACK));
	//check if the rank of the to square is 6 and the side is white or the rank of the to square is 3 and the side is black
	list->moves[list->count].move = move;
	list->moves[list->count].score = 105 + 1000000;
	list->count++;//increment the count which is the number of moves
}

static void AddWhitePawnCapMove( const S_BOARD *pos, const int from, const int to, const int cap, S_MOVELIST *list ) {

	ASSERT(PieceValidEmpty(cap));
	ASSERT(SqOnBoard(from));
	ASSERT(SqOnBoard(to));
	ASSERT(CheckBoard(pos));

	if(RanksBrd[from] == RANK_7) {
		AddCaptureMove(pos, MOVE(from,to,cap,wQ,0), list);
		AddCaptureMove(pos, MOVE(from,to,cap,wR,0), list);
		AddCaptureMove(pos, MOVE(from,to,cap,wB,0), list);
		AddCaptureMove(pos, MOVE(from,to,cap,wN,0), list);
	} else {
		AddCaptureMove(pos, MOVE(from,to,cap,EMPTY,0), list);
	}
}

static void AddWhitePawnMove( const S_BOARD *pos, const int from, const int to, S_MOVELIST *list ) {

	ASSERT(SqOnBoard(from));
	ASSERT(SqOnBoard(to));
	ASSERT(CheckBoard(pos));

	if(RanksBrd[from] == RANK_7) {
		AddQuietMove(pos, MOVE(from,to,EMPTY,wQ,0), list);
		AddQuietMove(pos, MOVE(from,to,EMPTY,wR,0), list);
		AddQuietMove(pos, MOVE(from,to,EMPTY,wB,0), list);
		AddQuietMove(pos, MOVE(from,to,EMPTY,wN,0), list);
	} else {
		AddQuietMove(pos, MOVE(from,to,EMPTY,EMPTY,0), list);
	}
}

static void AddBlackPawnCapMove( const S_BOARD *pos, const int from, const int to, const int cap, S_MOVELIST *list ) {

	ASSERT(PieceValidEmpty(cap));
	ASSERT(SqOnBoard(from));
	ASSERT(SqOnBoard(to));
	ASSERT(CheckBoard(pos));

	if(RanksBrd[from] == RANK_2) {
		AddCaptureMove(pos, MOVE(from,to,cap,bQ,0), list);
		AddCaptureMove(pos, MOVE(from,to,cap,bR,0), list);
		AddCaptureMove(pos, MOVE(from,to,cap,bB,0), list);
		AddCaptureMove(pos, MOVE(from,to,cap,bN,0), list);
	} else {
		AddCaptureMove(pos, MOVE(from,to,cap,EMPTY,0), list);
	}
}

static void AddBlackPawnMove( const S_BOARD *pos, const int from, const int to, S_MOVELIST *list ) {

	ASSERT(SqOnBoard(from));
	ASSERT(SqOnBoard(to));
	ASSERT(CheckBoard(pos));

	if(RanksBrd[from] == RANK_2) {
		AddQuietMove(pos, MOVE(from,to,EMPTY,bQ,0), list);
		AddQuietMove(pos, MOVE(from,to,EMPTY,bR,0), list);
		AddQuietMove(pos, MOVE(from,to,EMPTY,bB,0), list);
		AddQuietMove(pos, MOVE(from,to,EMPTY,bN,0), list);
	} else {
		AddQuietMove(pos, MOVE(from,to,EMPTY,EMPTY,0), list);
	}
}

void GenerateAllMoves(const S_BOARD *pos, S_MOVELIST *list) {//generate all the moves in the position
	ASSERT(CheckBoard(pos));//check the board is valid

	list->count = 0;//set the count to 0

	int pce = EMPTY;//set the piece to empty
	int side = pos->side;//set the side to the position side
	int sq = 0; int t_sq = 0;//set the square and target square initially to 0 this is the location of the piece
	int pceNum = 0;//set the piece number to 0 this is the number of pieces
	int dir = 0;//set the direction to 0
	int index = 0;//set the index to 0
	int pceIndex = 0;//set the piece index to 0

	if(side == WHITE) {//if the side is white

		for(pceNum = 0; pceNum < pos->pceNum[wP]; ++pceNum) {//for the number of all the present white pawns on the board
			sq = pos->pList[wP][pceNum];//set the square to the location of the white pawn
			ASSERT(SqOnBoard(sq));//check if the square is on the board

			if(pos->pieces[sq + 10] == EMPTY) {//if the square in front of the pawn is empty
				AddWhitePawnMove(pos, sq, sq+10, list);//move white pawn 
				if(RanksBrd[sq] == RANK_2 && pos->pieces[sq + 20] == EMPTY) {//if the pawn is on the second rank and the square two squares in front of the pawn is empty
					AddQuietMove(pos, MOVE(sq,(sq+20),EMPTY,EMPTY,MFLAGPS),list);//move the pawn two squares forward and set the pawn start flag
				}
			}

			if(!SQOFFBOARD(sq + 9) && PieceCol[pos->pieces[sq + 9]] == BLACK) {//if the square to the front left of the pawn is not offboard and the piece on that square is black
				AddWhitePawnCapMove(pos, sq, sq+9, pos->pieces[sq + 9], list);//move the white pawn to capture the black piece
			}
			if(!SQOFFBOARD(sq + 11) && PieceCol[pos->pieces[sq + 11]] == BLACK) {//if the square to the front right of the pawn is not offboard and the piece on that square is black
				AddWhitePawnCapMove(pos, sq, sq+11, pos->pieces[sq + 11], list);//move the white pawn to capture the black piece
			}

			if(pos->enPas != NO_SQ) {//if the en passant square is not no square
				if(sq + 9 == pos->enPas) {//if the square to the front left of the pawn is the en passant square
					AddEnPassantMove(pos, MOVE(sq,sq + 9,EMPTY,EMPTY,MFLAGEP), list);//move the pawn to set the en passant flag
				}
				if(sq + 11 == pos->enPas) {//if the square to the front right of the pawn is the en passant square
					AddEnPassantMove(pos, MOVE(sq,sq + 11,EMPTY,EMPTY,MFLAGEP), list);//move the pawn to set the en passant flag
				}
			}
		}

		if(pos->castlePerm & WKCA) {//if the white king can castle on the kingside
			if(pos->pieces[F1] == EMPTY && pos->pieces[G1] == EMPTY) {//if the squares f1 and g1 are empty
				if(!SqAttacked(E1,BLACK,pos) && !SqAttacked(F1,BLACK,pos) ) {//if the squares e1 and f1 are not attacked by black
					AddQuietMove(pos, MOVE(E1, G1, EMPTY, EMPTY, MFLAGCA), list);//move the white king to g1 and set the castle flag
				}
			}
		}

		if(pos->castlePerm & WQCA) {//if the white king can castle on the queenside
			if(pos->pieces[D1] == EMPTY && pos->pieces[C1] == EMPTY && pos->pieces[B1] == EMPTY) {//if the squares d1, c1 and b1 are empty
				if(!SqAttacked(E1,BLACK,pos) && !SqAttacked(D1,BLACK,pos) ) {//if the squares e1 and d1 are not attacked by black
					AddQuietMove(pos, MOVE(E1, C1, EMPTY, EMPTY, MFLAGCA), list);//move the white king to c1 and set the castle flag
				}
			}
		}

	} else {

		for(pceNum = 0; pceNum < pos->pceNum[bP]; ++pceNum) {//for the number of all the present black pawns on the board
			sq = pos->pList[bP][pceNum];//get the location of the black pawn
			ASSERT(SqOnBoard(sq));//check if the square is on the board

			if(pos->pieces[sq - 10] == EMPTY) {//if the square in front of the pawn is empty
				AddBlackPawnMove(pos, sq, sq-10, list);//move the black pawn forward
				if(RanksBrd[sq] == RANK_7 && pos->pieces[sq - 20] == EMPTY) {//if the pawn is on the seventh rank and the square two squares in front of the pawn is empty
					AddQuietMove(pos, MOVE(sq,(sq-20),EMPTY,EMPTY,MFLAGPS),list);//move the pawn two squares forward and set the pawn start flag
				}
			}

			if(!SQOFFBOARD(sq - 9) && PieceCol[pos->pieces[sq - 9]] == WHITE) {//if the square to the front left of the pawn is not offboard and the piece on that square is white
				AddBlackPawnCapMove(pos, sq, sq-9, pos->pieces[sq - 9], list);//move the black pawn to capture the white piece
			}

			if(!SQOFFBOARD(sq - 11) && PieceCol[pos->pieces[sq - 11]] == WHITE) {//if the square to the front right of the pawn is not offboard and the piece on that square is white
				AddBlackPawnCapMove(pos, sq, sq-11, pos->pieces[sq - 11], list);
			}
			if(pos->enPas != NO_SQ) {//if the en passant square is not no square
				if(sq - 9 == pos->enPas) {//if the square to the front left of the pawn is the en passant square
					AddEnPassantMove(pos, MOVE(sq,sq - 9,EMPTY,EMPTY,MFLAGEP), list);//move the pawn to set the en passant flag
				}
				if(sq - 11 == pos->enPas) {//if the square to the front right of the pawn is the en passant square
					AddEnPassantMove(pos, MOVE(sq,sq - 11,EMPTY,EMPTY,MFLAGEP), list);//move the pawn to set the en passant flag
				}
			}
		}

		// castling
		if(pos->castlePerm &  BKCA) {//if the black king can castle on the kingside
			if(pos->pieces[F8] == EMPTY && pos->pieces[G8] == EMPTY) {//if the squares f8 and g8 are empty
				if(!SqAttacked(E8,WHITE,pos) && !SqAttacked(F8,WHITE,pos) ) {//if the squares e8 and f8 are not attacked by white
					AddQuietMove(pos, MOVE(E8, G8, EMPTY, EMPTY, MFLAGCA), list);//move the black king to g8 and set the castle flag
				}
			}
		}

		if(pos->castlePerm &  BQCA) {//if the black king can castle on the queenside
			if(pos->pieces[D8] == EMPTY && pos->pieces[C8] == EMPTY && pos->pieces[B8] == EMPTY) {//if the squares d8, c8 and b8 are empty
				if(!SqAttacked(E8,WHITE,pos) && !SqAttacked(D8,WHITE,pos) ) {//if the squares e8 and d8 are not attacked by white
					AddQuietMove(pos, MOVE(E8, C8, EMPTY, EMPTY, MFLAGCA), list);//move the black king to c8 and set the castle flag
				}
			}
		}
	}

	/* Loop for slide pieces */
	//loop for the slide pieces like the rook, queen, bishop
	pceIndex = LoopSlideIndex[side];//set the piece index to the slide index of the side
	pce = LoopSlidePce[pceIndex++];//set the piece to the slide piece
	while( pce != 0) {//while the piece is not empty
		ASSERT(PieceValid(pce));//check if the piece is valid

		for(pceNum = 0; pceNum < pos->pceNum[pce]; ++pceNum) {//for the number of all the present pieces of that type on the board
			sq = pos->pList[pce][pceNum];//get the location of the piece
			ASSERT(SqOnBoard(sq));//check if the square is on the board

			for(index = 0; index < NumDir[pce]; ++index) {//for the number of directions the piece can move
				dir = PceDir[pce][index];//get the direction of movement of the piece from the PceDir array
				t_sq = sq + dir;//set the target square to the square plus the direction

				while(!SQOFFBOARD(t_sq)) {//while the target square is not offboard
					// BLACK ^ 1 == WHITE       WHITE ^ 1 == BLACK
					if(pos->pieces[t_sq] != EMPTY) {//if the target square is not empty
						if( PieceCol[pos->pieces[t_sq]] == (side ^ 1)) {//if the piece on the target square is of the opposite color
							AddCaptureMove(pos, MOVE(sq, t_sq, pos->pieces[t_sq], EMPTY, 0), list);//move the piece to capture the target piece
						}
						break;//break the loop
					}
					AddQuietMove(pos, MOVE(sq, t_sq, EMPTY, EMPTY, 0), list);//move the piece to the target square as it is empty
					t_sq += dir;//increment the target square by the direction
				}
			}
		}

		pce = LoopSlidePce[pceIndex++];//set the piece to the next slide piece
	}

	/* Loop for non slide */
	//loop for the non slide pieces like the knight, king
	pceIndex = LoopNonSlideIndex[side];//set the piece index to the non slide index of the side
	pce = LoopNonSlidePce[pceIndex++];//set the piece to the non slide piece

	while( pce != 0) {//while the piece is not empty
		ASSERT(PieceValid(pce));//check if the piece is valid

		for(pceNum = 0; pceNum < pos->pceNum[pce]; ++pceNum) {//for the number of all the present pieces of that type on the board
			sq = pos->pList[pce][pceNum];//get the location of the piece
			ASSERT(SqOnBoard(sq));//check if the square is on the board

			for(index = 0; index < NumDir[pce]; ++index) {//for the number of directions the piece can move
				dir = PceDir[pce][index];//get the direction of movement of the piece from the PceDir array
				t_sq = sq + dir;//set the target square to the square plus the direction

				if(SQOFFBOARD(t_sq)) {//if the target square is offboard continue
					continue;
				}

				// BLACK ^ 1 == WHITE       WHITE ^ 1 == BLACK
				if(pos->pieces[t_sq] != EMPTY) {//if the target square is not empty
					if( PieceCol[pos->pieces[t_sq]] == (side ^ 1)) {//if the piece on the target square is of the opposite color
						AddCaptureMove(pos, MOVE(sq, t_sq, pos->pieces[t_sq], EMPTY, 0), list);//move the piece to capture the target piece
					}
					continue;//continue
				}
				AddQuietMove(pos, MOVE(sq, t_sq, EMPTY, EMPTY, 0), list);//move the piece to the target square as it is empty
			}
		}

		pce = LoopNonSlidePce[pceIndex++];//set the piece to the next non slide piece
	}

    ASSERT(MoveListOk(list,pos));//check if the move list is ok
}


void GenerateAllCaps(const S_BOARD *pos, S_MOVELIST *list) {

	ASSERT(CheckBoard(pos));

	list->count = 0;

	int pce = EMPTY;
	int side = pos->side;
	int sq = 0; int t_sq = 0;
	int pceNum = 0;
	int dir = 0;
	int index = 0;
	int pceIndex = 0;

	if(side == WHITE) {

		for(pceNum = 0; pceNum < pos->pceNum[wP]; ++pceNum) {
			sq = pos->pList[wP][pceNum];
			ASSERT(SqOnBoard(sq));

			if(!SQOFFBOARD(sq + 9) && PieceCol[pos->pieces[sq + 9]] == BLACK) {
				AddWhitePawnCapMove(pos, sq, sq+9, pos->pieces[sq + 9], list);
			}
			if(!SQOFFBOARD(sq + 11) && PieceCol[pos->pieces[sq + 11]] == BLACK) {
				AddWhitePawnCapMove(pos, sq, sq+11, pos->pieces[sq + 11], list);
			}

			if(pos->enPas != NO_SQ) {
				if(sq + 9 == pos->enPas) {
					AddEnPassantMove(pos, MOVE(sq,sq + 9,EMPTY,EMPTY,MFLAGEP), list);
				}
				if(sq + 11 == pos->enPas) {
					AddEnPassantMove(pos, MOVE(sq,sq + 11,EMPTY,EMPTY,MFLAGEP), list);
				}
			}
		}

	} else {

		for(pceNum = 0; pceNum < pos->pceNum[bP]; ++pceNum) {
			sq = pos->pList[bP][pceNum];
			ASSERT(SqOnBoard(sq));

			if(!SQOFFBOARD(sq - 9) && PieceCol[pos->pieces[sq - 9]] == WHITE) {
				AddBlackPawnCapMove(pos, sq, sq-9, pos->pieces[sq - 9], list);
			}

			if(!SQOFFBOARD(sq - 11) && PieceCol[pos->pieces[sq - 11]] == WHITE) {
				AddBlackPawnCapMove(pos, sq, sq-11, pos->pieces[sq - 11], list);
			}
			if(pos->enPas != NO_SQ) {
				if(sq - 9 == pos->enPas) {
					AddEnPassantMove(pos, MOVE(sq,sq - 9,EMPTY,EMPTY,MFLAGEP), list);
				}
				if(sq - 11 == pos->enPas) {
					AddEnPassantMove(pos, MOVE(sq,sq - 11,EMPTY,EMPTY,MFLAGEP), list);
				}
			}
		}
	}

	/* Loop for slide pieces */
	pceIndex = LoopSlideIndex[side];
	pce = LoopSlidePce[pceIndex++];
	while( pce != 0) {
		ASSERT(PieceValid(pce));

		for(pceNum = 0; pceNum < pos->pceNum[pce]; ++pceNum) {
			sq = pos->pList[pce][pceNum];
			ASSERT(SqOnBoard(sq));

			for(index = 0; index < NumDir[pce]; ++index) {
				dir = PceDir[pce][index];
				t_sq = sq + dir;

				while(!SQOFFBOARD(t_sq)) {
					// BLACK ^ 1 == WHITE       WHITE ^ 1 == BLACK
					if(pos->pieces[t_sq] != EMPTY) {
						if( PieceCol[pos->pieces[t_sq]] == (side ^ 1)) {
							AddCaptureMove(pos, MOVE(sq, t_sq, pos->pieces[t_sq], EMPTY, 0), list);
						}
						break;
					}
					t_sq += dir;
				}
			}
		}

		pce = LoopSlidePce[pceIndex++];
	}

	/* Loop for non slide */
	pceIndex = LoopNonSlideIndex[side];
	pce = LoopNonSlidePce[pceIndex++];

	while( pce != 0) {
		ASSERT(PieceValid(pce));

		for(pceNum = 0; pceNum < pos->pceNum[pce]; ++pceNum) {
			sq = pos->pList[pce][pceNum];
			ASSERT(SqOnBoard(sq));

			for(index = 0; index < NumDir[pce]; ++index) {
				dir = PceDir[pce][index];
				t_sq = sq + dir;

				if(SQOFFBOARD(t_sq)) {
					continue;
				}

				// BLACK ^ 1 == WHITE       WHITE ^ 1 == BLACK
				if(pos->pieces[t_sq] != EMPTY) {
					if( PieceCol[pos->pieces[t_sq]] == (side ^ 1)) {
						AddCaptureMove(pos, MOVE(sq, t_sq, pos->pieces[t_sq], EMPTY, 0), list);
					}
					continue;
				}
			}
		}

		pce = LoopNonSlidePce[pceIndex++];
	}
    ASSERT(MoveListOk(list,pos));
}




