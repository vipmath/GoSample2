#include "Board.h"

int BOARD_SIZE = 9;
int BOARD_WIDTH = BOARD_SIZE + 1;
int BOARD_MAX = BOARD_WIDTH * (BOARD_SIZE + 2);
double KOMI = 6.5;
XY DIR4[4];

MoveResult Board::move(const XY xy, const Color color, const bool fill_eye_err)
{
	// �p�X�̏ꍇ
	if (xy == PASS) {
		ko = -1;
		return SUCCESS;
	}

	int tmp_ko;

	// ����4�����̘A
	int around_group_self[4];
	int around_group_self_num = 0;
	int around_group_capture[4];
	int around_group_capture_num = 0;
	int around_group_oponnent[4];
	int around_group_oponnent_num = 0;

	int capture_num = 0;

	// �ċz�_����v����A���擾
	for (int i = 0, hit_num = 0; hit_num < group_num; i++)
	{
		if (bit_test(&group_unusedflg[i / BIT], i % BIT) == 0)
		{
			hit_num++;
			if (group[i].hit_liberties(xy))
			{
				if (group[i].color == color)
				{
					around_group_self[around_group_self_num++] = i;
				}
				else {
					// ��邱�Ƃ��ł��邩
					if (group[i].liberty_num == 1)
					{
						around_group_capture[around_group_capture_num++] = i;
						capture_num += group[i].stone_num;
						if (group[i].stone_num == 1)
						{
							tmp_ko = group[i].stone[0];
						}
					}
					else {
						around_group_oponnent[around_group_oponnent_num++] = i;
					}
				}
			}
		}
	}

	int offboard_num = 0;
	int empty_num = 0;
	int alive_num = 0;
	// ����4�����̋�
	XY around_liberty[4] = { 0 };
	for (int i = 0; i < 4; i++)
	{
		XY xyd = xy + DIR4[i];

		if (board[xyd] == G_OFFBOARD)
		{
			offboard_num++;
			continue;
		}

		if (board[xyd] == G_NONE)
		{
			empty_num++;
			around_liberty[i] = 1;
			continue;
		}

		// �אڂ��鎩���̐F�̘A�̌ċz�_���Ȃ��Ȃ�Ȃ���
		if (group[board[xyd]].color == color && group[board[xyd]].liberty_num >= 2)
		{
			alive_num++;
		}
	}

	// ���E��
	if (capture_num == 0 && empty_num == 0 && alive_num == 0)
	{
		return ILLIGAL;
	}
	// �R�E
	if (xy == ko)
	{
		return KO;
	}
	// ��
	if (offboard_num + alive_num == 4 && fill_eye_err)
	{
		return EYE;
	}

	// �΂�u��
	if (around_group_self_num == 0)
	{
		// �A��ǉ�
		int idx = add_group(xy, color, around_liberty);

		// �A�ԍ��𖄂߂�
		board[xy] = idx;
	}
	else
	{
		// �A�ɐ΂�ǉ�
		Group& group0 = group[around_group_self[0]];
		group0.add_stone_and_liberties(xy, around_liberty);

		// �A�ԍ��𖄂߂�
		board[xy] = around_group_self[0];

		for (int i = 1; i < around_group_self_num; i++)
		{
			Group& groupi = group[around_group_self[i]];

			// �A���Ȃ���
			group0.chain_group(xy, groupi);

			// �A�ԍ���u��������
			for (int j = 0; j < groupi.stone_num; j++)
			{
				board[groupi.stone[j]] = around_group_self[0];
			}

			// �A���폜
			remove_group(around_group_self[i]);
		}
	}

	// �΂����
	for (int i = 0; i < around_group_capture_num; i++)
	{
		// �A�̂������ʒu���󔒂ɂ��Čċz�_��ǉ�����
		Group &remove = group[around_group_capture[i]];
		for (int j = 0; j < remove.stone_num; j++)
		{
			// �A�̂������ʒu���󔒂ɂ���
			XY xyr = remove.stone[j];
			board[xyr] = G_NONE;

			// �אڂ���΂Ɍċz�_��ǉ�
			for (XY d : DIR4)
			{
				XY xyd = xyr + d;
				if (board[xyd] != G_OFFBOARD && board[xyd] != G_NONE)
				{
					if (group[board[xyd]].color == color)
					{
						group[board[xyd]].add_liberty(xyr);
					}
				}
			}
		}

		// �A���폜
		remove_group(around_group_capture[i]);
	}

	// �ċz�_�̍폜
	for (int i = 0; i < around_group_oponnent_num; i++)
	{
		group[around_group_oponnent[i]].remove_liberty(xy);
	}

	// �R�E
	if (capture_num == 1 && group[board[xy]].stone_num == 1 && group[board[xy]].liberty_num == 1)
	{
		ko = tmp_ko;
	}
	else {
		ko = -1;
	}

	return SUCCESS;
}