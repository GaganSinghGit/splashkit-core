#include "window_manager.h"
#include "terminal.h"
#include "logging.h"
#include "random.h"
#include "machine_learning.h"

#include <iostream>
#include <vector>
#include <unordered_map>
using namespace std;
using namespace splashkit_lib;

class TicTacToe : public Game
{
public:
	enum class Cell
	{
		Empty,
		X,
		O
	};

	struct Board
	{
		Cell cells[3][3];
	};

	enum class GameState
	{
		Playing,
		X_Won,
		O_Won,
		Draw
	};

	enum class Player
	{
		X,
		O
	};

	struct Move
	{
		int row;
		int col;
	};

	Board board;
	Player current_player;
	GameState state;
	OutputFormat format = OutputFormat(get_board_size());

	int get_current_player() override { return (int)current_player; }
	int get_max_board_index() override { return 2; } // O or X
	int get_board_size() override { return 9; }		 // 3x3 = 9
	OutputFormat get_output_format() override { return format; }
	vector<int> get_board() override
	{
		vector<int> board_data(9);
		for (int i = 0; i < 3; i++)
		{
			for (int j = 0; j < 3; j++)
			{
				board_data[i * 3 + j] = (int)board.cells[i][j];
			}
		}
		return board_data;
	}
	int convert_output(QValue output, bool random) override
	{
		return format.get_max_position(output, 0, get_possible_moves(), random);
	}

	TicTacToe()
	{
		board = {{{Cell::Empty, Cell::Empty, Cell::Empty},
				  {Cell::Empty, Cell::Empty, Cell::Empty},
				  {Cell::Empty, Cell::Empty, Cell::Empty}}};

		format.add_type(OutputFormat::Type::Position, get_board_size());

		// When passing to AI
		// convert board to int array
		// e.g. {X, O, X, 		{1, 2, 1,		{[0, 1], [1, 0], [0, 1],
		//		 _, O, _,  -> 	 0, 2, 0,  ->	 [0, 0], [1, 0], [0, 0],  ->	[01 10 01 | 00 10 00 | 00 00 00]
		//		 _, _, _}		 0, 0, 0}		 [0, 0], [0, 0], [0, 0]}

		current_player = Player::X;
		state = GameState::Playing;
	}

	// Checks whether the board is won or drawn
	void update_state()
	{
		// Check rows
		for (int y = 0; y < 3; y++)
		{
			if (board.cells[y][0] != Cell::Empty &&
				board.cells[y][0] == board.cells[y][1] &&
				board.cells[y][1] == board.cells[y][2])
			{
				state = board.cells[y][0] == Cell::X ? GameState::X_Won : GameState::O_Won;
				return;
			}
		}

		// Check columns
		for (int x = 0; x < 3; x++)
		{
			if (board.cells[0][x] != Cell::Empty &&
				board.cells[0][x] == board.cells[1][x] &&
				board.cells[1][x] == board.cells[2][x])
			{
				state = board.cells[0][x] == Cell::X ? GameState::X_Won : GameState::O_Won;
				return;
			}
		}

		// Check diagonals
		if (board.cells[0][0] != Cell::Empty &&
			board.cells[0][0] == board.cells[1][1] &&
			board.cells[1][1] == board.cells[2][2])
		{
			state = board.cells[0][0] == Cell::X ? GameState::X_Won : GameState::O_Won;
			return;
		}

		if (board.cells[0][2] != Cell::Empty &&
			board.cells[0][2] == board.cells[1][1] &&
			board.cells[1][1] == board.cells[2][0])
		{
			state = board.cells[0][2] == Cell::X ? GameState::X_Won : GameState::O_Won;
			return;
		}

		// Check draw
		bool found_empty = false;
		for (int y = 0; y < 3; y++)
		{
			for (int x = 0; x < 3; x++)
			{
				if (board.cells[y][x] == Cell::Empty)
				{
					found_empty = true;
					break;
				}
			}
		}
		if (!found_empty)
		{
			state = GameState::Draw;
		}
	}

	void draw_board()
	{
		write_line();
		for (int y = 0; y < 3; y++)
		{
			for (int x = 0; x < 3; x++)
			{
				if (board.cells[y][x] == Cell::Empty)
				{
					write("_");
				}
				else if (board.cells[y][x] == Cell::X)
				{
					write("X");
				}
				else if (board.cells[y][x] == Cell::O)
				{
					write("O");
				}
			}
			write_line();
		}
	}

	void draw_game()
	{
		draw_board();
		if (state == GameState::X_Won)
		{
			write_line("X Won!");
		}
		else if (state == GameState::O_Won)
		{
			write_line("O Won!");
		}
		else if (state == GameState::Draw)
		{
			write_line("Draw!");
		}
	}

	vector<int> get_possible_moves() override
	{
		vector<int> moves = {};
		for (int y = 0; y < 3; y++)
		{
			for (int x = 0; x < 3; x++)
			{
				if (board.cells[y][x] == Cell::Empty)
				{
					moves.push_back(y * 3 + x);
				}
			}
		}
		return moves;
	}

	void make_move(int move) override
	{
		board.cells[move / 3][move % 3] = current_player == Player::X ? Cell::X : Cell::O;
		current_player = current_player == Player::X ? Player::O : Player::X;
		update_state();
		draw_game();
	}
};

int random_agent_play(int posb_moves)
{
	if (posb_moves < 1)
		throw logic_error("No moves available; Game over?");
	if (posb_moves == 1)
		return 0;
	return rnd(0, posb_moves - 1);
}

int q_agent_play(Game &game)
{
	OutputFormat format = game.get_output_format();
	vector<bool> input = game.convert_board();

	// vector<float> q_output = q_learning(input)
	vector<float> q_output = {0.2, 0.9, 0, 0, 1, 0, 0, 0.9, 0.3};

	return game.convert_output(QValue(q_output), false);
}

bool test_q_table()
{
	bool passes = true;

	TicTacToe game;
	OutputFormat format = game.get_output_format();
	QTable q_table = QTable(&format);
	QValue test = q_table.get_q_value(game.convert_board());
	if (test[0] != 0.5f)
	{
		passes = false;
	}
	write("Initial QValues = 0.5? ");
	write(((test[0] == 0.5f) ? "true" : "false"));
	write_line(" (" + to_string(test[0]) + ")");
	write_line(test.to_string());

	return passes;
}

bool test_q_value()
{
	bool passes = true;
	const float F_ERR = 0.01f; // Float error for comparisons.

	// Consider a right hand turn represented with possible actions LEFT=0 CENTER=1 or RIGHT=2
	vector<float> val = {0, 0, 0};
	QValue test = QValue(val);

	// We take a left turn
	test.to_update(0);
	// We punish the left turn
	test.update(-1);
	if (abs(-1 - test[0]) > F_ERR || abs(0 - test[1]) > F_ERR || abs(0 - test[2]) > F_ERR)
	{
		passes = false;
		write("Expected: {-1, 0, 0}, Actual: "); write_line(test.to_string());
		log(WARNING, "LEFT turn failed");
	}

	// We take a LEFT CENTER turn
	test.to_update(0);
	test.to_update(1);
	// We punish the center less
	test.update(-0.7);
	if (abs(-1.7 - test[0]) > F_ERR || abs(-0.7 - test[1]) > F_ERR || abs(0 - test[2]) > F_ERR)
	{
		passes = false;
		write("Expected: {-1.7, -0.7, 0}, Actual: "); write_line(test.to_string());
		log(WARNING, "LEFT CENTER turn failed");
	}

	// We take a RIGHT CENTER turn
	test.to_update(1);
	test.to_update(2);
	// We reward a correct turn
	test.update(1);
	if (abs(-1.7 - test[0]) > F_ERR || abs(0.3 - test[1]) > F_ERR || abs(1 - test[2]) > F_ERR)
	{
		passes = false;
		write("Expected: {-1.7, 0.3, 1}, Actual: "); write_line(test.to_string());
		log(WARNING, "RIGHT CENTER turn failed");
	}

	// We take no action
	test.update(999);
	if (abs(-1.7 - test[0]) > F_ERR || abs(0.3 - test[1]) > F_ERR || abs(1 - test[2]) > F_ERR)
	{
		passes = false;
		write("Expected: {-1.7, 0.3, 1}, Actual: "); write_line(test.to_string());
		log(WARNING, "NULL update failed");
	}

	write("test_q_value: "); write_line(test.to_string());

	return passes;
}

void run_machine_learning_test()
{
	log_level _log_level = INFO;
	log_mode _log_mode = LOG_CONSOLE;
	init_custom_logger(_log_mode);

	test_q_table();
	test_q_value();

	TicTacToe game;
	game.draw_game();
	while (game.state == TicTacToe::GameState::Playing)
	{
		vector<int> moves = game.get_possible_moves();
		if (game.current_player == TicTacToe::Player::X)
		{
			OutputFormat format = game.get_output_format();
			int ai_move = q_agent_play(game);
			game.make_move(ai_move);
		}
		else
		{
			game.make_move(moves[random_agent_play(moves.size())]);
		}
	}
}
