// board.c

#include "stdio.h"
#include "defs.h"

int PceListOk(const S_BOARD *pos) {
	int pce = wP;
	int sq;
	int num;
	for(pce = wP; pce <= bK; ++pce) {
		if(pos->pceNum[pce]<0 || pos->pceNum[pce]>=10) return FALSE;
	}

	if(pos->pceNum[wK]!=1 || pos->pceNum[bK]!=1) return FALSE;

	for(pce = wP; pce <= bK; ++pce) {
		for(num = 0; num < pos->pceNum[pce]; ++num) {
			sq = pos->pList[pce][num];
			if(!SqOnBoard(sq)) return FALSE;
		}
	}
    return TRUE;
}
//The CheckBoard function is typically used in a chess engine to verify the integrity of the board position. 
// It takes a pointer to a S_BOARD structure as an argument,
// which represents the current state of the chessboard.

// While the exact implementation can vary, this function generally performs checks like:

// Verifying that the board's piece count, material count, and other data are consistent with the actual pieces on the board.
// Checking that the side to move is valid (i.e., either white or black).
// Checking that the state of the en passant square is valid.
// Verifying the castling rights are valid.
// Checking that the position's hash key is correct.
// If all these checks pass, the function typically returns TRUE (or 1), indicating that the board state is valid. If any check fails, it returns FALSE (or 0), indicating an inconsistency in the board state.

int CheckBoard(const S_BOARD *pos) {
	//this is the function that checks if the board is valid
	int t_pceNum[13] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
	//this is the number of pieces of each type
	int t_bigPce[2] = { 0, 0};
	//this is the number of big pieces for each side that is not pawns
	int t_majPce[2] = { 0, 0};
	//this is the number of major pieces for each side that is king or queen or rook
	int t_minPce[2] = { 0, 0};
	//this is the number of minor pieces for each side that is bishop or knight
	int t_material[2] = { 0, 0};
    //this is the material score for each side
	int sq64,t_piece,t_pce_num,sq120,colour,pcount;
	//these are the square, the piece, the piece number, the square 120, the colour and the piece count
	U64 t_pawns[3] = {0ULL, 0ULL, 0ULL};

	t_pawns[WHITE] = pos->pawns[WHITE];
	//this typicaaly represent the state of the white pawns on the board
	//if the bit is set then there is a white pawn on that square
	t_pawns[BLACK] = pos->pawns[BLACK];//this is the state of the black pawns on the board
	t_pawns[BOTH] = pos->pawns[BOTH];
	//to tell if there is a pawn on the square both for white and black
	// check piece lists
	for(t_piece = wP; t_piece <= bK; ++t_piece) {//for all the pieces
		for(t_pce_num = 0; t_pce_num < pos->pceNum[t_piece]; ++t_pce_num) {//for all the pieces of that type in the piece list
			sq120 = pos->pList[t_piece][t_pce_num];//get the position square of the piece
			ASSERT(pos->pieces[sq120]==t_piece);//assert that the piece on the square is the same as the piece in the piece list
		}
	}

	// check piece count and other counters
	for(sq64 = 0; sq64 < 64; ++sq64) {
		//for all the squares on the board 64 squares
		sq120 = SQ120(sq64);//get the 120 based indexe of the square
		t_piece = pos->pieces[sq120];//get the piece on the square
		t_pceNum[t_piece]++;//increment the number of pieces of that type
		colour = PieceCol[t_piece];//get the colour of the piece
		if( PieceBig[t_piece] == TRUE) t_bigPce[colour]++;//if the piece is a big piece increment the number of big pieces
		if( PieceMin[t_piece] == TRUE) t_minPce[colour]++;//if the piece is a minor piece increment the number of minor pieces
		if( PieceMaj[t_piece] == TRUE) t_majPce[colour]++;//if the piece is a major piece increment the number of major pieces

		t_material[colour] += PieceVal[t_piece];//increment the material score for that colour
	}

	for(t_piece = wP; t_piece <= bK; ++t_piece) {//for all the pieces
		ASSERT(t_pceNum[t_piece]==pos->pceNum[t_piece]);//assert that the number of pieces of that type is the same as the number of pieces in the piece list
	}

	// check bitboards count
	pcount = CNT(t_pawns[WHITE]);//this is to count the number bits that are set in the white pawns bitboard
	ASSERT(pcount == pos->pceNum[wP]);//assert that the number of pawns is the same as the number of white pawns in the piece list
	pcount = CNT(t_pawns[BLACK]);//this is to count the number bits that are set in the black pawns bitboard
	ASSERT(pcount == pos->pceNum[bP]);//assert that the number of pawns is the same as the number of black pawns in the piece list
	pcount = CNT(t_pawns[BOTH]);//this is to count the number bits that are set in the both pawns bitboard
	ASSERT(pcount == (pos->pceNum[bP] + pos->pceNum[wP]));
	//assert that the number of pawns is the same as the number of white and black pawns in the piece list

	// check bitboards squares
	while(t_pawns[WHITE]) {
		sq64 = POP(&t_pawns[WHITE]);
		ASSERT(pos->pieces[SQ120(sq64)] == wP);//assert that the piece on the square returned is a white pawn
	}

	while(t_pawns[BLACK]) {//while there are black pawns
		sq64 = POP(&t_pawns[BLACK]);//pop the black pawn
		ASSERT(pos->pieces[SQ120(sq64)] == bP);//assert that the piece on the square returned is a black pawn
	}

	while(t_pawns[BOTH]) {//while there are pawns for both sides
		sq64 = POP(&t_pawns[BOTH]);
		ASSERT( (pos->pieces[SQ120(sq64)] == bP) || (pos->pieces[SQ120(sq64)] == wP) );
	}

	ASSERT(t_material[WHITE]==pos->material[WHITE] && t_material[BLACK]==pos->material[BLACK]);
	//assert that the material score for both sides is the same as the material score in the position
	ASSERT(t_minPce[WHITE]==pos->minPce[WHITE] && t_minPce[BLACK]==pos->minPce[BLACK]);
	//assert that the number of minor pieces for both sides is the same as the number of minor pieces in the position
	ASSERT(t_majPce[WHITE]==pos->majPce[WHITE] && t_majPce[BLACK]==pos->majPce[BLACK]);
	//assert that the number of major pieces for both sides is the same as the number of major pieces in the position
	ASSERT(t_bigPce[WHITE]==pos->bigPce[WHITE] && t_bigPce[BLACK]==pos->bigPce[BLACK]);
	//assert that the number of big pieces for both sides is the same as the number of big pieces in the position

	ASSERT(pos->side==WHITE || pos->side==BLACK);//assert that the side to move is either white or black
	ASSERT(GeneratePosKey(pos)==pos->posKey);//assert that the position key is the same as the generated position key

	ASSERT(pos->enPas==NO_SQ || ( RanksBrd[pos->enPas]==RANK_6 && pos->side == WHITE)
		 || ( RanksBrd[pos->enPas]==RANK_3 && pos->side == BLACK));

	ASSERT(pos->pieces[pos->KingSq[WHITE]] == wK);//assert that the piece on the white king square is a white king
	ASSERT(pos->pieces[pos->KingSq[BLACK]] == bK);//assert that the piece on the black king square is a black king

	ASSERT(pos->castlePerm >= 0 && pos->castlePerm <= 15);//assert that the castle permission is valid

	ASSERT(PceListOk(pos));//assert that the piece list is ok

	return TRUE;//return true if all the checks pass
}

void UpdateListsMaterial(S_BOARD *pos) {

	int piece,sq,index,colour;

	for(index = 0; index < BRD_SQ_NUM; ++index) {
		sq = index;
		piece = pos->pieces[index];
		ASSERT(PceValidEmptyOffbrd(piece));
		if(piece!=OFFBOARD && piece!= EMPTY) {
			colour = PieceCol[piece];
			ASSERT(SideValid(colour));

		    if( PieceBig[piece] == TRUE) pos->bigPce[colour]++;
		    if( PieceMin[piece] == TRUE) pos->minPce[colour]++;
		    if( PieceMaj[piece] == TRUE) pos->majPce[colour]++;

			pos->material[colour] += PieceVal[piece];

			ASSERT(pos->pceNum[piece] < 10 && pos->pceNum[piece] >= 0);

			pos->pList[piece][pos->pceNum[piece]] = sq;
			pos->pceNum[piece]++;


			if(piece==wK) pos->KingSq[WHITE] = sq;
			if(piece==bK) pos->KingSq[BLACK] = sq;

			if(piece==wP) {
				SETBIT(pos->pawns[WHITE],SQ64(sq));
				SETBIT(pos->pawns[BOTH],SQ64(sq));
			} else if(piece==bP) {
				SETBIT(pos->pawns[BLACK],SQ64(sq));
				SETBIT(pos->pawns[BOTH],SQ64(sq));
			}
		}
	}
}

int ParseFen(char *fen, S_BOARD *pos) {

	ASSERT(fen!=NULL);
	ASSERT(pos!=NULL);

	int  rank = RANK_8;
    int  file = FILE_A;
    int  piece = 0;
    int  count = 0;
    int  i = 0;
	int  sq64 = 0;
	int  sq120 = 0;

	ResetBoard(pos);

	while ((rank >= RANK_1) && *fen) {
	    count = 1;
		switch (*fen) {
            case 'p': piece = bP; break;
            case 'r': piece = bR; break;
            case 'n': piece = bN; break;
            case 'b': piece = bB; break;
            case 'k': piece = bK; break;
            case 'q': piece = bQ; break;
            case 'P': piece = wP; break;
            case 'R': piece = wR; break;
            case 'N': piece = wN; break;
            case 'B': piece = wB; break;
            case 'K': piece = wK; break;
            case 'Q': piece = wQ; break;

            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
                piece = EMPTY;
                count = *fen - '0';
                break;

            case '/':
            case ' ':
                rank--;
                file = FILE_A;
                fen++;
                continue;

            default:
                printf("FEN error \n");
                return -1;
        }

		for (i = 0; i < count; i++) {
            sq64 = rank * 8 + file;
			sq120 = SQ120(sq64);
            if (piece != EMPTY) {
                pos->pieces[sq120] = piece;
            }
			file++;
        }
		fen++;
	}

	ASSERT(*fen == 'w' || *fen == 'b');

	pos->side = (*fen == 'w') ? WHITE : BLACK;
	fen += 2;

	for (i = 0; i < 4; i++) {
        if (*fen == ' ') {
            break;
        }
		switch(*fen) {
			case 'K': pos->castlePerm |= WKCA; break;
			case 'Q': pos->castlePerm |= WQCA; break;
			case 'k': pos->castlePerm |= BKCA; break;
			case 'q': pos->castlePerm |= BQCA; break;
			default:	     break;
        }
		fen++;
	}
	fen++;

	ASSERT(pos->castlePerm>=0 && pos->castlePerm <= 15);

	if (*fen != '-') {
		file = fen[0] - 'a';
		rank = fen[1] - '1';

		ASSERT(file>=FILE_A && file <= FILE_H);
		ASSERT(rank>=RANK_1 && rank <= RANK_8);

		pos->enPas = FR2SQ(file,rank);
    }

	pos->posKey = GeneratePosKey(pos);

	UpdateListsMaterial(pos);

	return 0;
}

void ResetBoard(S_BOARD *pos) {

	int index = 0;

	for(index = 0; index < BRD_SQ_NUM; ++index) {
		pos->pieces[index] = OFFBOARD;
	}

	for(index = 0; index < 64; ++index) {
		pos->pieces[SQ120(index)] = EMPTY;
	}

	for(index = 0; index < 2; ++index) {
		pos->bigPce[index] = 0;
		pos->majPce[index] = 0;
		pos->minPce[index] = 0;
		pos->material[index] = 0;
	}

	for(index = 0; index < 3; ++index) {
		pos->pawns[index] = 0ULL;
	}

	for(index = 0; index < 13; ++index) {
		pos->pceNum[index] = 0;
	}

	pos->KingSq[WHITE] = pos->KingSq[BLACK] = NO_SQ;

	pos->side = BOTH;
	pos->enPas = NO_SQ;
	pos->fiftyMove = 0;

	pos->ply = 0;
	pos->hisPly = 0;

	pos->castlePerm = 0;

	pos->posKey = 0ULL;

}
void PrintBoard(const S_BOARD *pos) {

	int sq,file,rank,piece;

	printf("\nGame Board:\n\n");

	for(rank = RANK_8; rank >= RANK_1; rank--) {
		printf("%d  ",rank+1);
		for(file = FILE_A; file <= FILE_H; file++) {
			sq = FR2SQ(file,rank);
			piece = pos->pieces[sq];
			printf("%3c",PceChar[piece]);
		}
		printf("\n");
	}

	printf("\n   ");
	for(file = FILE_A; file <= FILE_H; file++) {
		printf("%3c",'a'+file);
	}
	printf("\n");
	printf("side:%c\n",SideChar[pos->side]);
	printf("enPas:%d\n",pos->enPas);
	printf("castle:%c%c%c%c\n",
			pos->castlePerm & WKCA ? 'K' : '-',
			pos->castlePerm & WQCA ? 'Q' : '-',
			pos->castlePerm & BKCA ? 'k' : '-',
			pos->castlePerm & BQCA ? 'q' : '-'
			);
	printf("PosKey:%llX\n",pos->posKey);
}

void MirrorBoard(S_BOARD *pos) {

    int tempPiecesArray[64];
    int tempSide = pos->side^1;
	int SwapPiece[13] = { EMPTY, bP, bN, bB, bR, bQ, bK, wP, wN, wB, wR, wQ, wK };
    int tempCastlePerm = 0;
    int tempEnPas = NO_SQ;

	int sq;
	int tp;

    if (pos->castlePerm & WKCA) tempCastlePerm |= BKCA;
    if (pos->castlePerm & WQCA) tempCastlePerm |= BQCA;

    if (pos->castlePerm & BKCA) tempCastlePerm |= WKCA;
    if (pos->castlePerm & BQCA) tempCastlePerm |= WQCA;

	if (pos->enPas != NO_SQ)  {
        tempEnPas = SQ120(Mirror64[SQ64(pos->enPas)]);
    }

    for (sq = 0; sq < 64; sq++) {
        tempPiecesArray[sq] = pos->pieces[SQ120(Mirror64[sq])];
    }

    ResetBoard(pos);

	for (sq = 0; sq < 64; sq++) {
        tp = SwapPiece[tempPiecesArray[sq]];
        pos->pieces[SQ120(sq)] = tp;
    }

	pos->side = tempSide;
    pos->castlePerm = tempCastlePerm;
    pos->enPas = tempEnPas;

    pos->posKey = GeneratePosKey(pos);

	UpdateListsMaterial(pos);

    ASSERT(CheckBoard(pos));
}
