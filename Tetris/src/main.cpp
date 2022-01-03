#include "Application.h"
#include "Renderer.h"
#include "RendererCommands.h"
#include "OrthoCameraController.h"
#include "Geometry.h"
#include "RandomNumberGenerator.h"
#include "Audio.h"

//Not from Ember
#include "Util.h"

#define WINDOW_WIDTH 1280	//Only use these for inital stuff, in update function, etc. use window->Properties()->width or height
#define WINDOW_HEIGHT 720

class Tetris : public Ember::Application {
public:
	void OnCreate() {
		Ember::RendererCommand::Init();
		Ember::RendererCommand::SetViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
		camera = Ember::OrthoCamera(0, WINDOW_WIDTH, WINDOW_HEIGHT, 0);
		camera.SetPosition({ 0, 0, 0 });

		renderer->SetFlag(Ember::RenderFlags::TopLeft, true);
		window->SetResizeable(true);

		board = new unsigned char[board_width * board_height];
		clear_board();

		current_piece = Ember::RandomGenerator::GenRandom(0, 6);

		music.Initialize("Tetris.wav");
		music.Play();
	}

	void clear_board() {
		for (int i = 0; i < board_width * board_height; i++)
			board[i] = '.';
	}

	virtual ~Tetris() {
		delete[] board;
	}

	glm::vec4 DecodeColor(char color) {
		switch (color) {
		case 'R': return { 1, 0, 0, 1 };
		case 'G': return { 0, 1, 0, 1 };
		case 'B': return { 0, 0, 1, 1 };
		case 'O': return { 0.925, 0.568, 0.054, 1 };
		case 'Y': return { 0.925, 0.870, 0.054, 1 };
		case 'C': return { 0.039, 0.419, 0.439, 1 };
		case 'P': return { 0.180, 0.039, 0.439, 1 };
		}
		return { 0, 0, 0, 1 };
	}

	void Render() {
		Ember::RendererCommand::Clear();
		Ember::RendererCommand::SetClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		renderer->BeginScene(&camera);

		Ember::Quad::Renderer(renderer);

		for (int y = 0; y < 4; y++) 
			for (int x = 0; x < 4; x++) 
				if (tetrominoes[current_piece][Rotate(x, y, current_rotation)] != '.') 
					Ember::Quad::DrawQuad({ board_pos.x + (x * square_size_width) + (current_piece_position.x * square_size_width),
						board_pos.y + (y * square_size_height) + (current_piece_position.y * square_size_height), 0.0f }, { square_size_width, square_size_width }, DecodeColor(current_color));

		for (int y = 0; y < board_height; y++) 
			for (int x = 0; x < board_width; x++) 
				Ember::Quad::DrawQuad({ board_pos.x + (x * square_size_width), board_pos.y + (y * square_size_height), 0.0f }, { square_size_width, square_size_width }, 
					DecodeColor(board[y * board_width + x]));

		renderer->EndScene();
	}

	void OnUserUpdate(float delta) {
		farthest = Sides(current_rotation);
		if (piece_drop_anim % piece_drop_delay == 0) {
			current_piece_position.y++;
			piece_drop_anim = 0;
		}

		if ((farthest.w + current_piece_position.y > board_height - 2 || !CanPieceFit()) && !Ember::KeyboardEvents::GetKeyboardState(Ember::EmberKeyCode::DownArrow)) 
			start_wait = true;
		if (start_wait)
			waittime_counter++;

		if (waittime_counter % waittime == 0) {
			waittime_counter = 1;
			start_wait = false;
		}

		if ((farthest.w + current_piece_position.y > board_height - 2 || !CanPieceFit()) && !start_wait) {
			for (int y = current_piece_position.y; y < current_piece_position.y + 4; y++) {
				for (int x = current_piece_position.x; x < current_piece_position.x + 4; x++) {
					if (x < board_width && x >= 0 && y < board_height && y >= 0) {
						char p = tetrominoes[current_piece][Rotate(x - current_piece_position.x, y - current_piece_position.y, current_rotation)];
						if (p != '.')
							board[(y * board_width) + x] = current_color;
					}
				}
			}

			start_wait = false;
			current_piece = Ember::RandomGenerator::GenRandom(0, 6);
			current_piece_position = { board_width / 2 - 2, 0 };
			current_rotation = 0;
			current_color = *SelectRandomly(COLORS.begin(), COLORS.end());

			//Check if line is filled
			for (int y = 0; y < board_height; y++) {
				int counter = 0;
				for (int x = 0; x < board_width; x++) {
					if (board[y * board_width + x] != '.')
						counter++;
				}
				if (counter == board_width) {
					for (int x = 0; x < board_width; x++) 
						board[y * board_width + x] = '.';
					
					Shift(y);
				}
			}
		}

		if (Gameover()) 
			clear_board();

		piece_drop_anim++;

		//Rendering Begins
		Render();
	}

	bool Gameover() {
		for (int x = 0; x < board_width; x++) 
			if (board[x] != '.')
				return true;
		return false;
	}

	void Shift(int starty) {
		for (int y = starty; y >= 0; y--) 
			for (int x = 0; x < board_width; x++) 
				if (y > 0)
					board[y * board_width + x] = board[(y - 1) * board_width + x];
	}

	glm::ivec4 Sides(int r) {
		int farthest_left = 3;
		int farthest_right = 0;
		int farthest_top = 3;
		int farthest_bottom = 0;
		for (int y = 0; y < 4; y++) {
			for (int x = 0; x < 4; x++) {
				if (tetrominoes[current_piece][Rotate(x, y, r)] != '.') {
					farthest_left = std::min(farthest_left, x);
					farthest_right = std::max(farthest_right, x);

					farthest_top = std::min(farthest_top, y);
					farthest_bottom = std::max(farthest_bottom, y);
				}
			}
		}
		return { farthest_left, farthest_right, farthest_top, farthest_bottom };
	}

	int Rotate(int px, int py, int r) {
		int pi = 0;
		switch (r % 4) {
		case 0: // 0 degrees		
			pi = py * 4 + px;		
			break;														
		case 1: // 90 degrees		
			pi = 12 + py - (px * 4);
			break;														
		case 2: // 180 degrees		
			pi = 15 - (py * 4) - px;
			break;					
		case 3: // 270 degrees		
			pi = 3 - py + (px * 4);	
			break;					
		}							

		return pi;
	}

	bool CheckSide(int xoffset, int yoffset, int r) {
		for (int y = current_piece_position.y + yoffset; y < current_piece_position.y + yoffset + 4; y++) 
			for (int x = current_piece_position.x + xoffset; x < current_piece_position.x + xoffset + 4; x++) 
				if (x < board_width && x >= 0 && y < board_height && y >= 0) 
					if (board[y * board_width + x] != '.' && tetrominoes[current_piece][Rotate(x - current_piece_position.x - xoffset, y - current_piece_position.y - yoffset, r)] != '.')
						return false;
		return true;
	}

	bool CanPieceFit() {
		bool bot = CheckSide(0, 1, current_rotation);
		return (bot);
	}

	void OnKeyboardEvent(Ember::KeyboardEvents& keyboard) {
		glm::ivec4 next_rot = Sides(current_rotation + 1);
		if (keyboard.scancode == Ember::EmberKeyCode::LeftArrow && farthest.x + current_piece_position.x > 0 && CheckSide(-1, 0, current_rotation))
			current_piece_position.x--;
		else if (keyboard.scancode == Ember::EmberKeyCode::RightArrow && farthest.y + current_piece_position.x < board_width - 1 && CheckSide(1, 0, current_rotation))
			current_piece_position.x++;
		else if (keyboard.scancode == Ember::EmberKeyCode::DownArrow)
			current_piece_position.y++;
		else if (keyboard.scancode == Ember::EmberKeyCode::UpArrow && next_rot.x + current_piece_position.x >= 0 && next_rot.y + current_piece_position.x < board_width 
			&& CheckSide(0, 0, current_rotation + 1)) 
			current_rotation++;
	}

	void UserDefEvent(Ember::Event& event) {
		Ember::EventDispatcher dispatch(&event);
		dispatch.Dispatch<Ember::KeyboardEvents>(EMBER_BIND_FUNC(OnKeyboardEvent));
	}

	void OnGuiUpdate() { }
private:
	std::string tetrominoes[7] = {	//Stores all 7 pieces as a 4x4 matrix
		{ "..X...X...X...X." },
		{ "..X..XX...X....." },
		{ ".....XX..XX....." },
		{ "..X..XX..X......" },
		{ ".X...XX...X....." },
		{ ".X...X...XX....." },
		{ "..X...X..XX....." }
	};	

	const int waittime = 50;
	int waittime_counter = 1;
	bool start_wait = false;

	unsigned char* board = nullptr;
	int board_width = 12;
	int board_height = 28;

	int square_size_width = 20;
	int square_size_height = 20;
	int current_piece = 0;
	int current_rotation = 0;
	char current_color = 'R';
	glm::ivec2 current_piece_position = { board_width / 2 - 2, 0 };
	int piece_drop_delay = 50; //milliseconds
	int piece_drop_anim = 1;
	glm::ivec4 farthest = { 0, 0, 0, 0 };
	glm::vec2 board_pos = { (float) ((WINDOW_WIDTH / 2) - ((board_width * square_size_width) / 2)), WINDOW_HEIGHT - (square_size_height * board_height) };

	const std::vector<char> COLORS = { 'R', 'G', 'B', 'Y', 'O', 'P', 'C' };

	Ember::AudioMusic music;
	Ember::OrthoCamera camera;
};

int main(int argc, char** argv) {
	Tetris tetris;
	tetris.Initialize("Tetris", WINDOW_WIDTH, WINDOW_HEIGHT);

	tetris.Run();

	return 0;
}
