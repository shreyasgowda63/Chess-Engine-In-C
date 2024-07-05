// makemove.c

#include "defs.h"
#include "stdio.h"

#define HASH_PCE(pce,sq) (pos->posKey ^= (PieceKeys[(pce)][(sq)]))
#define HASH_CA (pos->posKey ^= (CastleKeys[(pos->castlePerm)]))//this is a macro that hash the castle permission with the position key
#define HASH_SIDE (pos->posKey ^= (SideKey))//this is a macro that hash the side with the position key
#define HASH_EP (pos->posKey ^= (PieceKeys[EMPTY][(pos->enPas)]))//this is a macro that hash the en passant square with the position key

const int CastlePerm[120] = {//this is an array that stores the castle permission for each square
    15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
    15, 13, 15, 15, 15, 12, 15, 15, 14, 15,
    15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
    15,  7, 15, 15, 15,  3, 15, 15, 11, 15,
    15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15, 15, 15
};

static void ClearPiece(const int sq, S_BOARD *pos) {//this function clears a piece from the board

	ASSERT(SqOnBoard(sq));//this is a macro that checks if the square is on the board
	ASSERT(CheckBoard(pos));//this is a function that checks if the board is valid
	
    int pce = pos->pieces[sq];//this is the piece on the square
	
    ASSERT(PieceValid(pce));//this is a macro that checks if the piece is valid
	
	int col = PieceCol[pce];//this is the color of the piece
    int index = 0;//this is the index of the piece
	int t_pceNum = -1;//this is the piece number
	
	ASSERT(SideValid(col));//this is a macro that checks if the side is valid
	
    HASH_PCE(pce,sq);
	
	pos->pieces[sq] = EMPTY;//the square is now made empty
    pos->material[col] -= PieceVal[pce];//the material of the color is decreased by the value of the piece
	
	if(PieceBig[pce]) {//if the piece is big
			pos->bigPce[col]--;//the number of big pieces of the color is decreased
		if(PieceMaj[pce]) {//if the piece is major
			pos->majPce[col]--;//the number of major pieces of the color is decreased
		} else {
			pos->minPce[col]--;//the number of minor pieces of the color is decreased
		}
	} else {//if the piece is pawn
		CLRBIT(pos->pawns[col],SQ64(sq));//the pawn is cleared from the color's pawns
		CLRBIT(pos->pawns[BOTH],SQ64(sq));//the pawn is cleared from both colors' pawns
	}
	
	for(index = 0; index < pos->pceNum[pce]; ++index) {//this loop goes through the every piece of a certain type
		if(pos->pList[pce][index] == sq) {//the pList tells us the location of the piece of a certain type
			t_pceNum = index;//the piece number is the index
			break;
		}
	}
	
	ASSERT(t_pceNum != -1);//this is a macro to tell that u found the piece 
	ASSERT(t_pceNum>=0&&t_pceNum<10);//this is a macro to check if the piece number is valid
	
	pos->pceNum[pce]--;//the number of pieces of that type is decreased
	
	pos->pList[pce][t_pceNum] = pos->pList[pce][pos->pceNum[pce]];
  //the piece number is now the last piece number of that type
}


static void AddPiece(const int sq, S_BOARD *pos, const int pce) {//this function adds a piece to the board

    ASSERT(PieceValid(pce));//this is a macro that checks if the piece is valid
    ASSERT(SqOnBoard(sq));//this is a macro that checks if the square is on the board
	
	int col = PieceCol[pce];//this is the color of the piece
    
	ASSERT(SideValid(col));//this is a macro that checks if the side is valid

    HASH_PCE(pce,sq);//this is a macro that hashes the posKey with the combination piece and square
	
	pos->pieces[sq] = pce;//get the piece on the square

    if(PieceBig[pce]) {//if the piece is big
			pos->bigPce[col]++;//the number of big pieces of the color is increased
		if(PieceMaj[pce]) {//if the piece is major
			pos->majPce[col]++;//the number of major pieces of the color is increased
		} else {
			pos->minPce[col]++;//the number of minor pieces of the color is increased
		}
	} else {
		SETBIT(pos->pawns[col],SQ64(sq));//the pawn is set to the color's pawns
		SETBIT(pos->pawns[BOTH],SQ64(sq));//the pawn is set to both colors' pawns
	}
	
	pos->material[col] += PieceVal[pce];//the material of the color is increased by the value of the piece
	pos->pList[pce][pos->pceNum[pce]++] = sq;//set the last piece number of that type to the square as u add a piece
                    //last piece number of that type is increased by 1 for the next piece of that type
}

static void MovePiece(const int from, const int to, S_BOARD *pos) {//this function moves a piece from one square to another

    ASSERT(SqOnBoard(from));//this is a macro that checks if the square is on the board
    ASSERT(SqOnBoard(to));//this is a macro that checks if the square is on the board
	
	int index = 0;//this is the index of the piece
	int pce = pos->pieces[from];//this is the piece on the square from
	int col = PieceCol[pce];//this is the color of the piece
	ASSERT(SideValid(col));//this is a macro that checks if the side is valid
    ASSERT(PieceValid(pce));//this is a macro that checks if the piece is valid
	
#ifdef DEBUG
	int t_PieceNum = FALSE;
#endif

	HASH_PCE(pce,from);//this is a macro that hashes the posKey with the combination piece and square
	pos->pieces[from] = EMPTY;//the square from is now made empty
	
	HASH_PCE(pce,to);//this is a macro that hashes the posKey with the combination piece and square
	pos->pieces[to] = pce;//the piece is now on the square to
	
	if(!PieceBig[pce]) {//if the piece is not big then it is a pawn
		CLRBIT(pos->pawns[col],SQ64(from));//the pawn is cleared from the color's pawns
		CLRBIT(pos->pawns[BOTH],SQ64(from));//the pawn is cleared from both colors' pawns
		SETBIT(pos->pawns[col],SQ64(to));//the pawn is set to the color's pawns
		SETBIT(pos->pawns[BOTH],SQ64(to));//the pawn is set to both colors' pawns
	}    
	
	for(index = 0; index < pos->pceNum[pce]; ++index) {//this loop goes through the every piece of a certain type
		if(pos->pList[pce][index] == from) {//the pList tells us the location of the piece of a certain type and if it is the from square
			pos->pList[pce][index] = to;//change the location of the piece to the to square
#ifdef DEBUG
			t_PieceNum = TRUE;
#endif
			break;
		}
	}
	ASSERT(t_PieceNum);//this is a macro to tell that u found the piece
}

int MakeMove(S_BOARD *pos, int move) {//this function makes a move

	ASSERT(CheckBoard(pos));//this is a function that checks if the board is valid

    int from = FROMSQ(move);//this is to get the from square of the move
    int to = TOSQ(move);//this is to get the to square of the move
    int side = pos->side;//this is the side to move
	
	ASSERT(SqOnBoard(from));//this is a macro that checks if the square is on the board
    ASSERT(SqOnBoard(to));//this is a macro that checks if the square is on the board
    ASSERT(SideValid(side));//this is a macro that checks if the side is valid
    ASSERT(PieceValid(pos->pieces[from]));//this is a macro that checks if the piece is valid
	ASSERT(pos->hisPly >= 0 && pos->hisPly < MAXGAMEMOVES);//this is a macro that checks if the hisPly[history ply is the number of half moves in the game] is valid
	//hisPly stands for "history ply". It represents the total number of half-moves that have been made in the 
    //game so far. A half-move, also known as a ply in chess terminology, is a single player's move. So, for example, if white has moved 5 times and black has moved 4 times, hisPly would be 9.
    ASSERT(pos->ply >= 0 && pos->ply < MAXDEPTH);//this is a macro that checks if the ply[ply is the number of half moves in the search] is valid
	//ply represents the number of half-moves in the current search. In the context of a chess engine, a "search" is the process of exploring the tree of possible moves to decide on the best move to make.
	pos->history[pos->hisPly].posKey = pos->posKey;
	//this is the position key of the history ply
    //pos->hisPly act as a index for the history array and to get the position key of the history ply

	if(move & MFLAGEP) {//if the move is en passant
        if(side == WHITE) {//if the side is white
            ClearPiece(to-10,pos);//clear the piece on the to_square - 10 position
        } else {
            ClearPiece(to+10,pos);//clear the piece on the to_square + 10 position
        }
    } else if (move & MFLAGCA) {//if the move is a castle
        switch(to) {
            case C1://if the to square is C1 that means the move is a queenside castle of white
                MovePiece(A1, D1, pos);//move the white rook from A1 to D1
			break;
            case C8://if the to square is C8 that means the move is a queenside castle of black
                MovePiece(A8, D8, pos);//move the black rook from A8 to D8
			break;
            case G1://if the to square is G1 that means the move is a kingside castle of white
                MovePiece(H1, F1, pos);//move the white rook from H1 to F1
			break;
            case G8://if the to square is G8 that means the move is a kingside castle of black
                MovePiece(H8, F8, pos);//move the black rook from H8 to F8
			break;
            default: ASSERT(FALSE); break;//if the to square is not any of the above then it is an assertion error
        }
    }	
	
	if(pos->enPas != NO_SQ) HASH_EP;//if the en passant square is not no square then hash the en passant square
    HASH_CA;//hash the castle permission
	
	pos->history[pos->hisPly].move = move;//the move is stored in the history ply
    pos->history[pos->hisPly].fiftyMove = pos->fiftyMove;//the status fifty move is stored in the history ply
    pos->history[pos->hisPly].enPas = pos->enPas;//the en passant square is stored in the history ply
    pos->history[pos->hisPly].castlePerm = pos->castlePerm;//the castle permission is stored in the history ply

    pos->castlePerm &= CastlePerm[from];//the castle permission is updated
    pos->castlePerm &= CastlePerm[to];//the castle permission is updated
    pos->enPas = NO_SQ;//the en passant square is no square

	HASH_CA;//hash the castle permission
    
	
	int captured = CAPTURED(move);//the captured piece is stored
    pos->fiftyMove++;//the fifty move is increased
	
	if(captured != EMPTY) {//if the captured piece is not empty[0]
        ASSERT(PieceValid(captured));//this is a macro that checks if the piece is valid
        ClearPiece(to, pos);//clear the piece on the to square
        pos->fiftyMove = 0;//the fifty move is set to 0
    }
	
	pos->hisPly++;//the history ply is increased
	pos->ply++;//the ply is increased
	
	ASSERT(pos->hisPly >= 0 && pos->hisPly < MAXGAMEMOVES);//this is a macro that checks if the hisPly[history ply is the number of half moves in the game] is valid
	ASSERT(pos->ply >= 0 && pos->ply < MAXDEPTH);//this is a macro that checks if the ply[ply is the number of half moves in the search] is valid
	
	if(PiecePawn[pos->pieces[from]]) {//if the piece is a pawn
        pos->fiftyMove = 0;//the fifty move is set to 0
        if(move & MFLAGPS) {//if the move is a pawn start
            if(side==WHITE) {//if the side is white
                pos->enPas=from+10;//the en passant square is the from square + 10
                ASSERT(RanksBrd[pos->enPas]==RANK_3);//the rank of the en passant square is rank 3
            } else {
                pos->enPas=from-10;//the en passant square is the from square - 10
                ASSERT(RanksBrd[pos->enPas]==RANK_6);//the rank of the en passant square is rank 6
            }
            HASH_EP;//hash the en passant square with the position key
        }
    }
	
	MovePiece(from, to, pos);//move the piece from the from square to the to square
	
	int prPce = PROMOTED(move);//the promoted piece is stored
    if(prPce != EMPTY)   {//if the promoted piece is not empty[0]
        ASSERT(PieceValid(prPce) && !PiecePawn[prPce]);//this is a macro that checks if the piece is valid and not a pawn
        ClearPiece(to, pos);//clear the piece on the to square
        AddPiece(to, pos, prPce);//add the promoted piece to the to square
    }
	
	if(PieceKing[pos->pieces[to]]) {//if the piece is a king
        pos->KingSq[pos->side] = to;//set the position of the king of the side to the to square
    }
	
	pos->side ^= 1;//the side is changed
    HASH_SIDE;//hash the side with the position key

    ASSERT(CheckBoard(pos));//this is a function that checks if the board is valid
	
		
	if(SqAttacked(pos->KingSq[side],pos->side,pos))  {//if the king square of the side is attacked
        TakeMove(pos);//undo the move
        return FALSE;//return false
    }
	
	return TRUE;//return true
	
}

void TakeMove(S_BOARD *pos) {//this function undoes the move
	
	ASSERT(CheckBoard(pos));//this is a function that checks if the board is valid
	
	pos->hisPly--;//the history ply is decreased
    pos->ply--;//the ply is decreased
	
	ASSERT(pos->hisPly >= 0 && pos->hisPly < MAXGAMEMOVES);//this is a macro that checks if the hisPly[history ply is the number of half moves in the game] is valid
	ASSERT(pos->ply >= 0 && pos->ply < MAXDEPTH);//this is a macro that checks if the ply[ply is the number of half moves in the search] is valid
	
    int move = pos->history[pos->hisPly].move;//get the previous move
    int from = FROMSQ(move);//get the from square of the move
    int to = TOSQ(move);//get the to square of the move
	
	ASSERT(SqOnBoard(from));//this is a macro that checks if the square is on the board
    ASSERT(SqOnBoard(to));//this is a macro that checks if the square is on the board
	
	if(pos->enPas != NO_SQ) HASH_EP;//if the en passant square is not no square then hash the en passant square
    HASH_CA;//hash the castle permission

    pos->castlePerm = pos->history[pos->hisPly].castlePerm;//the castle permission is set to the previous castle permission
    pos->fiftyMove = pos->history[pos->hisPly].fiftyMove;//the fifty move is set to the previous fifty move
    pos->enPas = pos->history[pos->hisPly].enPas;//the en passant square is set to the previous en passant square

    if(pos->enPas != NO_SQ) HASH_EP;//if the en passant square is not no square then hash the en passant square
    HASH_CA;//hash the castle permission

    pos->side ^= 1;//the side is changed
    HASH_SIDE;//hash the side with the position key
	
	if(MFLAGEP & move) {//if the move was en passant capture then add the pawn back 
        if(pos->side == WHITE) {//if the side is white
            AddPiece(to-10, pos, bP);//add the black pawn to the to square - 10
        } else {//if the side is black
            AddPiece(to+10, pos, wP);//add the white pawn to the to square + 10
        }
    } else if(MFLAGCA & move) {//if the move was a castle then undo the castle
        switch(to) {
            case C1: MovePiece(D1, A1, pos); break;//if the to square is C1 then move the rook from D1 to A1
            case C8: MovePiece(D8, A8, pos); break;//if the to square is C8 then move the rook from D8 to A8
            case G1: MovePiece(F1, H1, pos); break;//if the to square is G1 then move the rook from F1 to H1
            case G8: MovePiece(F8, H8, pos); break;//if the to square is G8 then move the rook from F8 to H8
            default: ASSERT(FALSE); break;
        }
    }
	
	MovePiece(to, from, pos);//move the piece from the to square to the from square undo the move
	
	if(PieceKing[pos->pieces[from]]) {//if the piece is a king
        pos->KingSq[pos->side] = from;//set the position of the king of the side to the from square
    }
	
	int captured = CAPTURED(move);//the captured piece is stored
    if(captured != EMPTY) {//if the captured piece is not empty[0]
        ASSERT(PieceValid(captured));//this is a macro that checks if the piece is valid
        AddPiece(to, pos, captured);//add the captured piece back to the to square
    }
	
	if(PROMOTED(move) != EMPTY)   {//if the promoted piece is not empty[0]
        ASSERT(PieceValid(PROMOTED(move)) && !PiecePawn[PROMOTED(move)]);//this is a macro that checks if the piece is valid and not a pawn
        ClearPiece(from, pos);//clear the piece on the from square
        AddPiece(from, pos, (PieceCol[PROMOTED(move)] == WHITE ? wP : bP));//add the pawn back to the from square
    }
	
    ASSERT(CheckBoard(pos));//this is a function that checks if the board is valid

}


void MakeNullMove(S_BOARD *pos) {

    ASSERT(CheckBoard(pos));
    ASSERT(!SqAttacked(pos->KingSq[pos->side],pos->side^1,pos));

    pos->ply++;
    pos->history[pos->hisPly].posKey = pos->posKey;

    if(pos->enPas != NO_SQ) HASH_EP;

    pos->history[pos->hisPly].move = NOMOVE;
    pos->history[pos->hisPly].fiftyMove = pos->fiftyMove;
    pos->history[pos->hisPly].enPas = pos->enPas;
    pos->history[pos->hisPly].castlePerm = pos->castlePerm;
    pos->enPas = NO_SQ;

    pos->side ^= 1;
    pos->hisPly++;
    HASH_SIDE;
   
    ASSERT(CheckBoard(pos));
	ASSERT(pos->hisPly >= 0 && pos->hisPly < MAXGAMEMOVES);
	ASSERT(pos->ply >= 0 && pos->ply < MAXDEPTH);

    return;
} // MakeNullMove

void TakeNullMove(S_BOARD *pos) {
    ASSERT(CheckBoard(pos));

    pos->hisPly--;
    pos->ply--;

    if(pos->enPas != NO_SQ) HASH_EP;

    pos->castlePerm = pos->history[pos->hisPly].castlePerm;
    pos->fiftyMove = pos->history[pos->hisPly].fiftyMove;
    pos->enPas = pos->history[pos->hisPly].enPas;

    if(pos->enPas != NO_SQ) HASH_EP;
    pos->side ^= 1;
    HASH_SIDE;
  
    ASSERT(CheckBoard(pos));
	ASSERT(pos->hisPly >= 0 && pos->hisPly < MAXGAMEMOVES);
	ASSERT(pos->ply >= 0 && pos->ply < MAXDEPTH);
}













