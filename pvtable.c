// pvtable.c

#include "stdio.h"
#include "defs.h"
// The term "principal variation" (PV) is commonly used in the context of chess programming. In a chess engine, the principal variation is the sequence of moves that the engine considers to be the best after it has performed its search of the game tree.

// The principal variation is often stored in a data structure known as a PV table or PV line. This allows the engine to remember the best sequence of moves it has found, so that it can play them out on the board, or use them to guide its search in future.

int GetPvLine(const int depth, S_BOARD *pos) {
// this function gets the principal variation line
	ASSERT(depth < MAXDEPTH && depth >= 1);

	int move = ProbePvMove(pos);
	//probe the principal variation move
	int count = 0;///initialize the count to 0
	
	while(move != NOMOVE && count < depth) {
	//while the move is not no move and the count is less than the depth
		ASSERT(count < MAXDEPTH);//assert that the count is less than the maximum depth
	
		if( MoveExists(pos, move) ) {//if the move exists
			MakeMove(pos, move);//make the move
			pos->PvArray[count++] = move;//store the move in the principal variation array
		} else {
			break;
		}		
		move = ProbePvMove(pos);//probe the principal variation move
	}
	
	while(pos->ply > 0) {//this is to take back the moves that were made
		TakeMove(pos);
	}
	
	return count;
	
}

void ClearHashTable(S_HASHTABLE *table) {//this function clears the hash table

  S_HASHENTRY *tableEntry;//get the variable for the table entry
  
  for (tableEntry = table->pTable; tableEntry < table->pTable + table->numEntries; tableEntry++) {
	//traverse the table entries              // start of the table + the number of entries
    tableEntry->posKey = 0ULL;//set the position key to 0
    tableEntry->move = NOMOVE;//set the move to no move
    tableEntry->depth = 0;//set the depth to 0
    tableEntry->score = 0;//set the score to 0
    tableEntry->flags = 0;//set the flags to 0
  }
  table->newWrite=0;
}

void InitHashTable(S_HASHTABLE *table, const int MB) {  
	//this function initializes the hash table principal variation table
	int HashSize = 0x100000 * MB;//the size of the hash table that is one megabyte
    table->numEntries = HashSize / sizeof(S_HASHENTRY);
	// the number of entries in the hash table is the size of the hash table divided by the size of a hash entry 
    table->numEntries -= 2;
	// it is decremented by 2 to avoid any out of bounds errors
	
	
	if(table->pTable!=NULL) {
		free(table->pTable);
	}
		
    table->pTable = (S_HASHENTRY *) malloc(table->numEntries * sizeof(S_HASHENTRY));
	//dynamic memory allocation for the hash table
	if(table->pTable == NULL) {//if the allocation fails
		printf("Hash Allocation Failed, trying %dMB...\n",MB/2);//print the message
		InitHashTable(table,MB/2);//initialize the hash table with half the size
	} else {
		ClearHashTable(table);//clear the hash table
		printf("HashTable init complete with %d entries\n",table->numEntries);//print the message 
		//that the hash table has been initialized with the number of entries
	}
	
}

int ProbeHashEntry(S_BOARD *pos, int *move, int *score, int alpha, int beta, int depth) {

	int index = pos->posKey % pos->HashTable->numEntries;
	
	ASSERT(index >= 0 && index <= pos->HashTable->numEntries - 1);
    ASSERT(depth>=1&&depth<MAXDEPTH);
    ASSERT(alpha<beta);
    ASSERT(alpha>=-INFINITE&&alpha<=INFINITE);
    ASSERT(beta>=-INFINITE&&beta<=INFINITE);
    ASSERT(pos->ply>=0&&pos->ply<MAXDEPTH);
	
	if( pos->HashTable->pTable[index].posKey == pos->posKey ) {
		*move = pos->HashTable->pTable[index].move;
		if(pos->HashTable->pTable[index].depth >= depth){
			pos->HashTable->hit++;
			
			ASSERT(pos->HashTable->pTable[index].depth>=1&&pos->HashTable->pTable[index].depth<MAXDEPTH);
            ASSERT(pos->HashTable->pTable[index].flags>=HFALPHA&&pos->HashTable->pTable[index].flags<=HFEXACT);
			
			*score = pos->HashTable->pTable[index].score;
			if(*score > ISMATE) *score -= pos->ply;
            else if(*score < -ISMATE) *score += pos->ply;
			
			switch(pos->HashTable->pTable[index].flags) {
				
                ASSERT(*score>=-INFINITE&&*score<=INFINITE);

                case HFALPHA: if(*score<=alpha) {
                    *score=alpha;
                    return TRUE;
                    }
                    break;
                case HFBETA: if(*score>=beta) {
                    *score=beta;
                    return TRUE;
                    }
                    break;
                case HFEXACT:
                    return TRUE;
                    break;
                default: ASSERT(FALSE); break;
            }
		}
	}
	
	return FALSE;
}

void StoreHashEntry(S_BOARD *pos, const int move, int score, const int flags, const int depth) {

	int index = pos->posKey % pos->HashTable->numEntries;
	
	ASSERT(index >= 0 && index <= pos->HashTable->numEntries - 1);
	ASSERT(depth>=1&&depth<MAXDEPTH);
    ASSERT(flags>=HFALPHA&&flags<=HFEXACT);
    ASSERT(score>=-INFINITE&&score<=INFINITE);
    ASSERT(pos->ply>=0&&pos->ply<MAXDEPTH);
	
	if( pos->HashTable->pTable[index].posKey == 0) {
		pos->HashTable->newWrite++;
	} else {
		pos->HashTable->overWrite++;
	}
	
	if(score > ISMATE) score += pos->ply;
    else if(score < -ISMATE) score -= pos->ply;
	
	pos->HashTable->pTable[index].move = move;
    pos->HashTable->pTable[index].posKey = pos->posKey;
	pos->HashTable->pTable[index].flags = flags;
	pos->HashTable->pTable[index].score = score;
	pos->HashTable->pTable[index].depth = depth;
}

int ProbePvMove(const S_BOARD *pos) {
	// to probe the principal variation move
	int index = pos->posKey % pos->HashTable->numEntries;//indexed based on the position key and the number of entries in the hash table
	ASSERT(index >= 0 && index <= pos->HashTable->numEntries - 1);//assert that the index is valid
	
	if( pos->HashTable->pTable[index].posKey == pos->posKey ) {//if the position key matches
		return pos->HashTable->pTable[index].move;//return the move
	}
	
	return NOMOVE;//return no move if the move is not found
}
















