// search.c

#include "stdio.h"
#include "defs.h"
// hisPly typically represents the total number of half-moves made in the game so far,
// from the starting position to the current position. 
//It's often used to access game history information, such as the positions 
//or moves that have occurred earlier in the game.
//ply, on the other hand, usually represents the number of half-moves made in the search 
//from the root position to the current position.
int rootDepth;

static void CheckUp(S_SEARCHINFO *info) {
	// .. check if time up, or interrupt from GUI
	if(info->timeset == TRUE && GetTimeMs() > info->stoptime) {
		info->stopped = TRUE;
	}

	ReadInput(info);
}
//hm. By examining promising moves first, the algorithm can prune larger portions of the search tree early on. This is because alpha-beta pruning's effectiveness is heavily dependent on the order in which moves are evaluated. 
//If the best moves are evaluated early, it leads to more pruning opportunities, reducing the number of nodes the algorithm needs to examine.
//The PickNextMove function in the search.c file is designed to reorder the moves in a move list (S_MOVELIST) starting from a given move number (moveNum). 
//It aims to place the move with the highest score first among the remaining moves, starting from moveNum to the end of the list.

static void PickNextMove(int moveNum, S_MOVELIST *list) {
//
	S_MOVE temp;
	int index = 0;
	int bestScore = 0;
	int bestNum = moveNum;

	for (index = moveNum; index < list->count; ++index) {//for all the moves in the list
		if (list->moves[index].score > bestScore) {//if the score of the move is greater than the best score
			bestScore = list->moves[index].score;//set the best score to the score of the move
			bestNum = index;//set the best number to the index
		}
	}

	ASSERT(moveNum>=0 && moveNum<list->count);//assert that the move number is valid
	ASSERT(bestNum>=0 && bestNum<list->count);//assert that the best number is valid
	ASSERT(bestNum>=moveNum);//assert that the best number is greater than or equal to the move number

	temp = list->moves[moveNum];//swap the moves
	list->moves[moveNum] = list->moves[bestNum];
	list->moves[bestNum] = temp;
}

static int IsRepetition(const S_BOARD *pos) {//this function checks if the position is a repetition

	int index = 0;

	for(index = pos->hisPly - pos->fiftyMove; index < pos->hisPly-1; ++index) {
		//if the position key is equal to the position key of the previous position
		ASSERT(index >= 0 && index < MAXGAMEMOVES);
		if(pos->posKey == pos->history[index].posKey) {//if the position key is equal to the position key of the previous position
			return TRUE;//return true
		}
	}
	return FALSE;//return false
}
//The ClearForSearch function in the search.c file is used to reset or initialize certain values before starting a new search in the chess engine. Here's what it does:
static void ClearForSearch(S_BOARD *pos, S_SEARCHINFO *info) {

	int index = 0;
	int index2 = 0;
	//resetting the search history and search killers arrays
	//setting the history heuristic scores to 0 
	//Resetting it ensures that the scores from the previous search do not affect the new search
	for(index = 0; index < 13; ++index) {
		for(index2 = 0; index2 < BRD_SQ_NUM; ++index2) {
			pos->searchHistory[index][index2] = 0;
		}
	}

	for(index = 0; index < 2; ++index) {
		for(index2 = 0; index2 < MAXDEPTH; ++index2) {
			pos->searchKillers[index][index2] = 0;
		}
	}

	pos->HashTable->overWrite=0;
	pos->HashTable->hit=0;
	pos->HashTable->cut=0;
	pos->ply = 0;//set it to 0 after the search

	info->stopped = 0;
	info->nodes = 0;
	info->fh = 0;
	info->fhf = 0;
}

static int Quiescence(int alpha, int beta, S_BOARD *pos, S_SEARCHINFO *info) {
	// The Quiescence function is part of a technique used in chess engines known as Quiescence Search. This is a type of search used to prevent the "horizon effect".
	//The horizon effect is a problem that occurs in game tree searches when a potentially disastrous move is not detected because it lies just beyond the maximum search depth (the "horizon").
	//Quiescence Search is used to try to avoid this problem by extending the search at the leaves of the game tree, but only for "quiet" 
	//positions. A position is considered "quiet" if there are no major threats or captures that can be made. The idea is to continue searching until a stable position is reached where a more accurate evaluation can be made.
	
	ASSERT(CheckBoard(pos));//check the board
	ASSERT(beta>alpha);//assert that beta is greater than alpha
	if(( info->nodes & 2047 ) == 0) {
		CheckUp(info);
	}

	info->nodes++;//increment the number of nodes

	if(IsRepetition(pos) || pos->fiftyMove >= 100) {
		return 0;//if the position is a repetition or the fifty move rule applies, return 0
	}

	if(pos->ply > MAXDEPTH - 1) {//if the ply is greater than the maximum depth
		return EvalPosition(pos);//return the evaluation of the position
	}

	int Score = EvalPosition(pos);//get the evaluation of the position

	ASSERT(Score>-INFINITE && Score<INFINITE);//assert that the score is valid

	if(Score >= beta) {//if the score is greater than or equal to beta then we already have a good move
		return beta;
	}

	if(Score > alpha) {//if the score is greater than alpha
		alpha = Score;
	}

	S_MOVELIST list[1];//create a move list
    GenerateAllCaps(pos,list);//generate all the captures in the position

    int MoveNum = 0;//set the Movenum to 0
	int Legal = 0;
	Score = -INFINITE;

	for(MoveNum = 0; MoveNum < list->count; ++MoveNum) {//traverse through all the moves in movelist

		PickNextMove(MoveNum, list);//pick the next move

        if ( !MakeMove(pos,list->moves[MoveNum].move))  {//if the move is not legal
            continue;//continue
        }

		Legal++;//increment the legal moves
		
		Score = -Quiescence( -beta, -alpha, pos, info);//recursive call to the quiescence function
        TakeMove(pos);//take the move back

		if(info->stopped == TRUE) {//if the search is stopped as the time is up
			return 0;
		}

		if(Score > alpha) {//if the score is greater than alpha
			if(Score >= beta) {//if the score is greater than or equal to beta
				if(Legal==1) {//if the move is the first legal move
					info->fhf++;//increment the fail high first counter
				}
				info->fh++;//increment the fail high counter
				return beta;//return beta
			}
			alpha = Score;//set alpha to the score
		}
    }

	ASSERT(alpha >= OldAlpha);//assert that alpha is greater than or equal to old alpha

	return alpha;//return alpha
}

static int AlphaBeta(int alpha, int beta, int depth, S_BOARD *pos, S_SEARCHINFO *info, int DoNull) {
//Alpha represents the best score (highest value) that the maximizing player is assured of. It's the lower bound of the possible outcome.
//Beta represents the best score (lowest value) that the minimizing player is assured of. It's the upper bound of the possible outcome.
	ASSERT(CheckBoard(pos));//check the board
	ASSERT(beta>alpha);//assert that beta is greater than alpha
	ASSERT(depth>=0);//assert that the depth is greater than or equal to 0

	if(depth <= 0) {
		return Quiescence(alpha, beta, pos, info);
		// return EvalPosition(pos);
	}

	if(( info->nodes & 2047 ) == 0) {
		//: The condition if((info->nodes & 2047) == 0) uses bitwise AND operation to periodically check if it's time to perform a "checkup" after every 2048 nodes have been evaluated 
		//(since 2047 in binary is 11111111111, and info->nodes & 2047 will be 0 when info->nodes is a multiple of 2048).
		CheckUp(info);
	}

	info->nodes++;//increment the number of nodes
//IsRepetition(pos): This function checks if the current position (pos) on the board has been repeated. In chess, the threefold repetition rule states that a game is drawn if the 
//same position occurs three times with the same player to move and all possible moves.
//pos->fiftyMove >= 100: This checks if the fifty-move rule applies. The fifty-move rule in chess states that a player can claim a draw if no capture has been made and 
//no pawn has been moved in the last fifty moves by each player. 
	if((IsRepetition(pos) || pos->fiftyMove >= 100) && pos->ply) {
		return 0;
	}

	if(pos->ply > MAXDEPTH - 1) {//if u have reached the maximum depth
		return EvalPosition(pos);//return the evaluation of the position
	}

	int InCheck = SqAttacked(pos->KingSq[pos->side],pos->side^1,pos);
	//SqAttacked: This function checks if a square on the board is attacked by a given side. 
	//If that is the case then more importance is given to the move that gets the king out of check.

	if(InCheck == TRUE) {
		depth++;
	}

	int Score = -INFINITE;//set the score to negative infinity minimum value
	int PvMove = NOMOVE;//set the principal variation move to no move

	if( ProbeHashEntry(pos, &PvMove, &Score, alpha, beta, depth) == TRUE ) {
		pos->HashTable->cut++;
		return Score;
	}

	if( DoNull && !InCheck && pos->ply && (pos->bigPce[pos->side] > 0) && depth >= 4) {
		MakeNullMove(pos);
		Score = -AlphaBeta( -beta, -beta + 1, depth-4, pos, info, FALSE);
		TakeNullMove(pos);
		if(info->stopped == TRUE) {
			return 0;
		}

		if (Score >= beta && abs(Score) < ISMATE) {
			info->nullCut++;
			return beta;
		}
	}

	S_MOVELIST list[1];
    GenerateAllMoves(pos,list);

    int MoveNum = 0;
	int Legal = 0;
	int OldAlpha = alpha;
	int BestMove = NOMOVE;

	int BestScore = -INFINITE;

	Score = -INFINITE;
//Prioritization: By assigning a high score to the PvMove, the move ordering mechanism ensures that this move is examined first during the search. This is 
//based on the heuristic that the best move from a previous iteration is likely to be a strong candidate in the current iteration as well.
// Exploring the PvMove first can lead to quicker alpha-beta cutoffs because this move is presumed to be strong. If the PvMove indeed leads to a good position, evaluating it early helps to establish tighter alpha
// (for the maximizing player) or beta (for the minimizing player) bounds for the rest of the search, thereby pruning the search tree more effectively.


	if( PvMove != NOMOVE) {
		for(MoveNum = 0; MoveNum < list->count; ++MoveNum) {
			if( list->moves[MoveNum].move == PvMove) {
				list->moves[MoveNum].score = 2000000;
				//printf("Pv move found \n");
				break;
			}
		}
	}

	for(MoveNum = 0; MoveNum < list->count; ++MoveNum) {
		//for all the moves in the list
		PickNextMove(MoveNum, list);
		//pick the next move
        if ( !MakeMove(pos,list->moves[MoveNum].move))  {
            continue;
        }//if the move is not legal, continue

		Legal++;//increment the legal moves
		Score = -AlphaBeta( -beta, -alpha, depth-1, pos, info, TRUE);//recursive call to the alpha beta function
		TakeMove(pos);//take the move back

		if(info->stopped == TRUE) {//if the search is stopped as the time is up
			return 0;//return 0
		}
		if(Score > BestScore) {//if the score is greater than the best score
			BestScore = Score;//set the best score to the score
			BestMove = list->moves[MoveNum].move;//get the best move
			if(Score > alpha) {
				if(Score >= beta) {
					if(Legal==1) {
	// Stands for "fail-high first." This counter is incremented only if the first move at a node causes a beta cutoff. This is a more specific statistic than fh, 
	//as it measures how often the very first move considered at a node is good enough to cause a cutoff. 
						info->fhf++;
					}
					info->fh++;
	// Stands for "fail-high." This counter is incremented every time a move causes a beta cutoff. A beta cutoff occurs when the engine finds a move that is so good for the player making the move that it assumes the opponent will avoid the position,
	// leading the engine to prune the rest of the moves at that node. The fh counter tracks how often these cutoffs happen, which is an indicator of how effective the move ordering is.				
					if(!(list->moves[MoveNum].move & MFLAGCAP)) {
						pos->searchKillers[1][pos->ply] = pos->searchKillers[0][pos->ply];
						pos->searchKillers[0][pos->ply] = list->moves[MoveNum].move;
					}
    //Higher Weight to Deeper Cutoffs: Moves that cause cutoffs at deeper levels of the search tree are generally more valuable for pruning the search space efficiently. By incrementing the history value by the depth at which the cutoff occurred, moves that prove effective at deeper levels are given more weight. 
	//This means they will be considered more favorable in future move ordering, potentially leading to earlier cutoffs and a more efficient search
					StoreHashEntry(pos, BestMove, beta, HFBETA, depth);

					return beta;
				}
				alpha = Score;
				//this is to do the alp
				if(!(list->moves[MoveNum].move & MFLAGCAP)) {
					pos->searchHistory[pos->pieces[FROMSQ(BestMove)]][TOSQ(BestMove)] += depth;
				}
			}
		}
    }

	if(Legal == 0) {
		if(InCheck) {
			return -INFINITE + pos->ply;
		} else {
			return 0;
		}
	}

	ASSERT(alpha>=OldAlpha);

	if(alpha != OldAlpha) {
		StoreHashEntry(pos, BestMove, BestScore, HFEXACT, depth);
	} else {
		StoreHashEntry(pos, BestMove, alpha, HFALPHA, depth);
	}

	return alpha;
}

void SearchPosition(S_BOARD *pos, S_SEARCHINFO *info) {

	int bestMove = NOMOVE;
	//initilises the best move found so far to no move
	int bestScore = -INFINITE;
	//Sets the initial best score to a very low value, effectively negative infinity, to ensure that any evaluated move will replace this score.
	int currentDepth = 0;
	//currentDepth = 0;: Initializes the current search depth to 0. Depth is often used 
	//in chess engines to control how many moves ahead the engine will consider.
	int pvMoves = 0;
	//to count the number of moves in the principal variation
	int pvNum = 0;

	ClearForSearch(pos,info);
	//The ClearForSearch function is used to reset or initialize certain values before starting a new search in the chess engine.
	
	if(EngineOptions->UseBook == TRUE) {
		bestMove = GetBookMove(pos);
	}

	//printf("Search depth:%d\n",info->depth);

	// iterative deepening
	//Purpose: The primary goal of iterative deepening in a chess engine is to search through the game's possible moves to a certain depth, 
	//then gradually increase this depth and search again.
	if(bestMove == NOMOVE) {//start the iterative deepening loop
		for( currentDepth = 1; currentDepth <= info->depth; ++currentDepth ) {
								// alpha	 beta
			rootDepth = currentDepth;
			bestScore = AlphaBeta(-INFINITE, INFINITE, currentDepth, pos, info, TRUE);
	// for each depth, the AlphaBeta function is called to search the game tree to that depth and return the best move and score found.
			if(info->stopped == TRUE) {
				break;
			}

			pvMoves = GetPvLine(currentDepth, pos);
			//After each depth iteration, it retrieves the principal variation line (pvMoves = GetPvLine(currentDepth, pos);) 
			//which is the sequence of moves considered the best from the current position.
			bestMove = pos->PvArray[0];
			//get the next best move
			if(info->GAME_MODE == UCIMODE) {
				printf("info score cp %d depth %d nodes %ld time %d ",
					bestScore,currentDepth,info->nodes,GetTimeMs()-info->starttime);
			} else if(info->GAME_MODE == XBOARDMODE && info->POST_THINKING == TRUE) {
				printf("%d %d %d %ld ",
					currentDepth,bestScore,(GetTimeMs()-info->starttime)/10,info->nodes);
			} else if(info->POST_THINKING == TRUE) {
				printf("score:%d depth:%d nodes:%ld time:%d(ms) ",
					bestScore,currentDepth,info->nodes,GetTimeMs()-info->starttime);
			}
			if(info->GAME_MODE == UCIMODE || info->POST_THINKING == TRUE) {
				pvMoves = GetPvLine(currentDepth, pos);
				if(!info->GAME_MODE == XBOARDMODE) {
					printf("pv");
				}
				for(pvNum = 0; pvNum < pvMoves; ++pvNum) {
					printf(" %s",PrMove(pos->PvArray[pvNum]));
				}
				printf("\n");
			}

			//printf("Hits:%d Overwrite:%d NewWrite:%d Cut:%d\nOrdering %.2f NullCut:%d\n",pos->HashTable->hit,pos->HashTable->overWrite,pos->HashTable->newWrite,pos->HashTable->cut,
			//(info->fhf/info->fh)*100,info->nullCut);
		}
	}

	if(info->GAME_MODE == UCIMODE) {
		printf("bestmove %s\n",PrMove(bestMove));
	} else if(info->GAME_MODE == XBOARDMODE) {
		printf("move %s\n",PrMove(bestMove));
		MakeMove(pos, bestMove);
	} else {
		printf("\n\n***!! Vice makes move %s !!***\n\n",PrMove(bestMove));
		MakeMove(pos, bestMove);
		PrintBoard(pos);
	}

}




















