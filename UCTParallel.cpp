#include <future>
#include "UCTParallel.h"

#ifndef _DEBUG
const int THREAD_NUM = 8; // 論理コア数
#else
const int THREAD_NUM = 1; // 論理コア数
#endif // !_DEBUG

const int THR = 15; // ノード展開の閾値

extern UCTNode* create_root_node();
extern UCTNode* create_child_node(const int size);

void UCTParallel::search_uct_root(Board& board, const Color color, UCTNode* node, UCTNode* copychild)
{
	// UCBからプレイアウトする手を選択
	// rootノードはアトミックに更新するためUCB計算ではロックしない
	UCTNode* selected_node = select_node_with_ucb(node);

	// rootでは全て合法手なのでエラーチェックはしない
	board.move_legal(selected_node->xy, color);

	// コピーされたノードに変換
	UCTNode* selected_node_copy = copychild + (selected_node - node->child);

	int win;

	// 閾値以下の場合プレイアウト(全スレッドの合計値)
	if (selected_node->playout_num < THR)
	{
		win = 1 - playout(board, opponent(color));
	}
	else {
		if (selected_node_copy->child_num == 0)
		{
			// ノードを展開
			if (selected_node_copy->expand_node(board))
			{
				win = 1 - search_uct(board, opponent(color), selected_node_copy);
			}
			else {
				// ノードプール不足
				win = 1 - playout(board, opponent(color));
			}
		}
		else {
			win = 1 - search_uct(board, opponent(color), selected_node_copy);
		}
	}

	// 勝率を更新(アトミックに加算)
	_InterlockedExchangeAdd(&selected_node->win_num, win);
	_InterlockedIncrement(&selected_node->playout_num);
	_InterlockedIncrement(&node->playout_num_sum);
}

void UCTParallel::expand_root_node(const Board& board, const Color color, UCTNode* root)
{
	// 合法手の数をカウント
	XY legal_xy[BOARD_SIZE_MAX * BOARD_SIZE_MAX + 1];
	for (XY xy = board.empty_list.begin(); xy != board.empty_list.end(); xy = board.empty_list.next(xy))
	{
		if (board.is_legal(xy, color) == SUCCESS)
		{
			legal_xy[root->child_num++] = xy;
		}
	}
	// PASSを追加
	legal_xy[root->child_num++] = PASS;

	// ノードを確保
	root->child = create_child_node(root->child_num);

	// ノードの値を設定
	for (int i = 0; i < root->child_num; i++)
	{
		root->child[i].xy = legal_xy[i];
		root->child[i].playout_num = 0;
		root->child[i].playout_num_sum = 0;
		root->child[i].win_num = 0;
		root->child[i].child_num = 0;
	}
}

XY UCTParallel::select_move(Board& board, Color color)
{
	UCTNode* root = create_root_node();
	this->root = root;

	// ノードを展開(合法手のみ)
	expand_root_node(board, color, root);

	// root並列化
	std::thread th[THREAD_NUM];
	for (int th_i = 0; th_i < THREAD_NUM; th_i++)
	{
		th[th_i] = std::thread([root, &board, color] {
			// rootの子ノードのコピー
			UCTNode* copychild = create_child_node(root->child_num);
			if (copychild == nullptr)
			{
				fprintf(stderr, "node pool too small\n");
				return;
			}

			for (int i = 0; i < root->child_num; i++)
			{
				copychild[i].xy = root->child[i].xy;
				copychild[i].playout_num = 0;
				copychild[i].playout_num_sum = 0;
				copychild[i].child_num = 0;
			}

			for (int i = 0; i < PLAYOUT_MAX / THREAD_NUM; i++)
			{
				// 局面コピー
				Board board_tmp = board;

				// UCT
				search_uct_root(board_tmp, color, root, copychild);
			}
		});
	}

	// スレッド終了待機
	for (int th_i = 0; th_i < THREAD_NUM; th_i++)
	{
		th[th_i].join();
	}

	// 最もプレイアウト数が多い手を選ぶ
	UCTNode* best_move;
	int num_max = -999;
	double rate_min = 1; // 勝率
	double rate_max = 0; // 勝率
	for (int i = 0; i < root->child_num; i++)
	{
		UCTNode* child = root->child + i;
		if (child->playout_num > 0)
		{
			int num = child->playout_num;
			if (num > num_max)
			{
				best_move = child;
				num_max = num;
			}

			double rate;
			if (rate_min == 1)
			{
				rate = double(child->win_num) / child->playout_num;
				if (rate < rate_min)
				{
					rate_min = rate;
				}
			}
			if (rate_max == 0)
			{
				rate = double(child->win_num) / child->playout_num;
				if (rate > rate_max)
				{
					rate_max = rate;
				}
			}
		}
	}

	if (rate_min == 1)
	{
		return PASS;
	}
	if (rate_max == 0)
	{
		return RESIGN;
	}

	return best_move->xy;
}