// validate.c

#include "defs.h"
#include "stdio.h"
#include "string.h"

int MoveListOk(const S_MOVELIST *list,  const S_BOARD *pos) {//is the move list ok?
	if(list->count < 0 || list->count >= MAXPOSITIONMOVES) {//if the count is less than 0 or greater than or equal to the maximum position moves
		return FALSE;
	}

	int MoveNum;
	int from = 0;
	int to = 0;
	for(MoveNum = 0; MoveNum < list->count; ++MoveNum) {//for all the moves in the list
		to = TOSQ(list->moves[MoveNum].move);//get the to square
		from = FROMSQ(list->moves[MoveNum].move);//get the from square
		if(!SqOnBoard(to) || !SqOnBoard(from)) {
			return FALSE;//if the to or from square is not on the board return false
		}
		if(!PieceValid(pos->pieces[from])) {
			PrintBoard(pos);
			return FALSE;//if the piece on the from square is not valid return false
		}
	}

	return TRUE;//return true
}

int SqIs120(const int sq) {//is the square a 120 square?
	return (sq>=0 && sq<120);
}

int PceValidEmptyOffbrd(const int pce) {
	return (PieceValidEmpty(pce) || pce == OFFBOARD);
}
int SqOnBoard(const int sq) {//is the square on the board?
	return FilesBrd[sq]==OFFBOARD ? 0 : 1;
}

int SideValid(const int side) {//is the side valid?
	return (side==WHITE || side == BLACK) ? 1 : 0;
}

int FileRankValid(const int fr) {//is the file or rank valid?
	return (fr >= 0 && fr <= 7) ? 1 : 0;
}

int PieceValidEmpty(const int pce) {//is the piece valid or empty?
	return (pce >= EMPTY && pce <= bK) ? 1 : 0;
}

int PieceValid(const int pce) {//is the piece valid?
	return (pce >= wP && pce <= bK) ? 1 : 0;
}

void DebugAnalysisTest(S_BOARD *pos, S_SEARCHINFO *info) {

	FILE *file;
    file = fopen("lct2.epd","r");
    char lineIn [1024];

	info->depth = MAXDEPTH;
	info->timeset = TRUE;
	int time = 1140000;


    if(file == NULL) {
        printf("File Not Found\n");
        return;
    }  else {
        while(fgets (lineIn , 1024 , file) != NULL) {
			info->starttime = GetTimeMs();
			info->stoptime = info->starttime + time;
			ClearHashTable(pos->HashTable);
            ParseFen(lineIn, pos);
            printf("\n%s\n",lineIn);
			printf("time:%d start:%d stop:%d depth:%d timeset:%d\n",
				time,info->starttime,info->stoptime,info->depth,info->timeset);
			SearchPosition(pos, info);
            memset(&lineIn[0], 0, sizeof(lineIn));
        }
    }
}



void MirrorEvalTest(S_BOARD *pos) {
    FILE *file;
    file = fopen("mirror.epd","r");
    char lineIn [1024];
    int ev1 = 0; int ev2 = 0;
    int positions = 0;
    if(file == NULL) {
        printf("File Not Found\n");
        return;
    }  else {
        while(fgets (lineIn , 1024 , file) != NULL) {
            ParseFen(lineIn, pos);
            positions++;
            ev1 = EvalPosition(pos);
            MirrorBoard(pos);
            ev2 = EvalPosition(pos);

            if(ev1 != ev2) {
                printf("\n\n\n");
                ParseFen(lineIn, pos);
                PrintBoard(pos);
                MirrorBoard(pos);
                PrintBoard(pos);
                printf("\n\nMirror Fail:\n%s\n",lineIn);
                getchar();
                return;
            }

            if( (positions % 1000) == 0)   {
                printf("position %d\n",positions);
            }

            memset(&lineIn[0], 0, sizeof(lineIn));
        }
    }
}

