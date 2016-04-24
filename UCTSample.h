#pragma once
#include "Player.h"

const int FPU = 10; // First Play Urgency
const double C = 1.0; // UCB定数
const int THR = 15; // ノード展開の閾値

extern int PLAYOUT_MAX;

class UCTNode
{
public:
	XY xy;
	volatile long playout_num;
	volatile long playout_num_sum;
	volatile long win_num;
	int child_num; // 子ノードの数
	UCTNode* child; // 子ノード

	bool expand_node(const Board& board);
};

class UCTSample : public Player
{
public:
	UCTNode* root;
	virtual XY select_move(Board& board, Color color);

	int get_created_node_cnt();
};
