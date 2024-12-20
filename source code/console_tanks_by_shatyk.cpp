#include "stdafx.h"
#include <windows.h>
#include <iostream>
#include <conio.h>
#include <ctime>
#include <fstream>
using namespace std;

const int size_x = 51; //size of the game field
const int size_y = 42; 
bool arr_saves[10] = { true, false, false, false, false, false, false, false, false, false }; //заблокированные и открытые уровни 
int arr_scores[4] = { 0,1,1,0 }; //money, speed, HP, respawn
int game_lvl = 0; //level
int players_col; //number of players
int hard; //game difficulty
int game_over=0; // -1 lost, 1 won, 0 in progress

struct Respawns {    //respawns, 1st[]: respawn number, 2nd[]: 0-x, 1-y
	int point_resp_players[2][2]; //players
	int point_resp_enemy[4][2];  //opponents
	int amount_resp_f; //number of respawns for fast tanks
	int amount_resp_m; // medium
	int amount_resp_s; // slow

	void set_amount(int f, int m, int s) { //set the number of respawns for opponent tanks
		if (players_col == 2) {
			amount_resp_f = f * 1.5; amount_resp_m = m * 1.5; amount_resp_s = s * 1.5;
		}
		else {
			amount_resp_f = f; amount_resp_m = m; amount_resp_s = s;
		}
	}
};

unsigned int up_c = 1; //redraw counter
enum Course { STOP = 0, UP, RIGHT, DOWN, LEFT };  //direction
enum Players_number { FIRST = 0, SECOND };  //player number
enum Mode_enemies { RANDOM = 0, DESTR_PL, DESTR_BASE }; //opponents' mode
enum Type_enemies { FAST = 0, MEDIUM, SLOW };

void set_cur(int x, int y)//установка directionора на позицию  x y
{
	COORD coord;
	coord.X = x;
	coord.Y = y;
	SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), coord);
};

//////////////////////CELL///////////////////////////////////
class Cell {  //cell class with its properties
	char symbol;   //cell symbol and type
	bool pass;    //passing through a cell
	bool bul_pop; //bullet hitting a cell
	bool bul_crash; //true if the bullet destroys the cell
public:
	Cell(char _symbol);
	char ret_sym() { return symbol; }
	bool ret_pass() { return pass; }
	bool ret_bul_pop() { return bul_pop; }
	bool ret_bul_crash() { return bul_crash; }
};

Cell::Cell(char _symbol)
{
	switch (_symbol)
	{
	case ' ':
		pass = true;
		bul_pop = false;
		bul_crash = false;
		break;
	case '#':
		pass = false;
		bul_pop = true;
		bul_crash = true;
		break;
	case '%':
		pass = false;
		bul_pop = true;
		bul_crash = false;
		break;
	case '~':
		pass = false;
		bul_pop = false;
		bul_crash = false;
		break;
	case '*':
		pass = true;
		bul_pop = true;
		bul_crash = true;
		break;
	default:
		pass = false;
		bul_pop = true;
		bul_crash = true;
		break;
	}

	symbol = _symbol;
}

///////////////////////BASE_TANK////////////////////////////////
class Base_tank { //base class for tanks
protected:
	int hp_resp;   //number of respawns
	int hp;
	int dead; // <0 - tank respawned, >0 - dead, ==0 - ready to respawn
	char sprite_up[3][3];   //скин танка вврх
	char sprite_right[3][3];  //to the right
	int x;  //coordinates of the top-left corner of the tank
	int y;
	char model; //tank type determination for constructors
	Course course;
	//////bullet
	int bullet_x; //x, y coordinates of the projectile
	int bullet_y;
	int bullet_kd; //delay between shots
	Course bullet_course; //direction пули, stop - пуля готова к выстрелу
public:
	char kostil;//workaround! symbol under the projectile

	Course ret_course() { return course; }  //вернуть direction танка
	void set_course(Course _course) { course = _course; }  //установка directionа
	void move_tank(Course _course); //move tank coordinates
	void set_xy(int _x, int _y) { x = _x; y = _y; } //set tank coordinates
	int ret_x() { return x; }  //return x
	int ret_y() { return y; }  //y
	char ret_sprite(int _x, int _y, Course _course); //return one symbol from the tank sprite
	void kill_tank(); //tank death
	int ret_dead() { return dead; }  //return respawn counter after death
	void dec_dead() { --dead; } //decrement counter
	int ret_hp_resp() { return hp_resp; }
	void set_hp_resp(int a) { hp_resp = a; }
								//////bullet
	void move_bullet();  //bullet movement
	Course ret_bullet_course() { return bullet_course; } //вернуть direction пули
	void stop_bullet_course() { bullet_course = STOP; }
	void start_bullet(); //generate bullet (shoot)
	int ret_bullet_x() { return bullet_x; }  //return bullet x coordinate
	int ret_bullet_y() { return bullet_y; }  //y
	int ret_bullet_kd() { return bullet_kd; } //return bullet cooldown
	void dec_bullet_kd() { if(bullet_kd>0) --bullet_kd;  } //decrement bullet cooldown 
	void set_bullet_kd(int kd) { bullet_kd = kd; } //set bullet cooldown
};

void Base_tank::move_tank(Course _course)
void Base_tank::move_tank(Course _course)
{
	switch (_course)
	{
	case UP:
		y -= 1;
		break;
	case RIGHT:
		x += 1;
		break;
	case DOWN:
		y += 1;
		break;
	case LEFT:
		x -= 1;
		break;
	}
}
char Base_tank::ret_sprite(int _x, int _y, Course _course)
{
	switch (_course)
	{
	case UP:
	case DOWN:
		return sprite_up[_y][_x];
		break;
	case RIGHT:
	case LEFT:
		return sprite_right[_y][_x];
		break;
	}
}
void  Base_tank::kill_tank()
{
	hp--;
	if (hp == 0) {
		hp_resp--;
		dead = 30;
		x = 0;
		y = 0;
	}
}

//////bullet
void Base_tank::move_bullet()
{
	switch (bullet_course)
	{
	case UP:
		bullet_y -= 1;
		break;
	case RIGHT:
		bullet_x += 1;
		break;
	case DOWN:
		bullet_y += 1;
		break;
	case LEFT:
		bullet_x -= 1;
		break;
	}
}
void Base_tank::start_bullet()
{
	bullet_course = course;
	switch (course)
	{
	case UP:
		bullet_x = x + 1;
		bullet_y = y;
		break;
	case RIGHT:
		bullet_x = x + 2;
		bullet_y = y + 1;
		break;
	case DOWN:
		bullet_x = x + 1;
		bullet_y = y + 2;
		break;
	case LEFT:
		bullet_x = x;
		bullet_y = y + 1;
		break;
	case STOP:
		break;
	}
}
//VVVVVVVV///////////PLAYERS_TANK//////////////VVVVVVVVVVVV///
class Players_tank : public Base_tank {
	Players_number player_numb; //player number
public:
	void set_config(char _model); //определяет спрайт, player number, хп
	void set_hp(int h) { hp = h; }
	int ret_hp() { return hp; }
	void dec_hp() { --hp; }
	void kill_tank();

	Players_number ret_player_numb() { return player_numb; } //вернуть player number
};

void Players_tank::set_config(char _model)
{
	course = UP;
	char temp_u[3][3] =
	{
		'/', '|', '\\',
		'|', 'O', '|',
		'\\', '@', '/'
	};
	char temp_r[3][3] =
	{
		'/', '-', '\\',
		'@', 'O', '-',
		'\\', '-', '/'
	};
	for (int i = 0; i<3; i++) {
		cout << "\n";
		for (int j = 0; j<3; j++) {
			sprite_up[i][j] = temp_u[i][j];
			sprite_right[i][j] = temp_r[i][j];
		}
	}
	sprite_up[2][1] = _model;
	sprite_right[1][0] = _model;
	hp_resp = 2;
	switch (_model)
	{
	case '1':
		player_numb = FIRST;
		break;
	case '2':
		player_numb = SECOND;
		break;
	default:
		exit(3);
	}
	hp = 1;
	bullet_course = STOP;
	dead = 0;
	kostil = ' ';
	set_bullet_kd(1);
}
void Players_tank::kill_tank()
{
	hp--;
	if (hp == 0) {
		hp_resp--;
		dead = 30;
		arr_scores[0] = arr_scores[0] - (10 * game_lvl) +10;
		set_cur(53, 1); cout << "COINS - " << "      ";
		set_cur(53, 1); cout << "COINS - " << arr_scores[0];
		set_cur(0, 0);
		x = 0;
		y = 0;
	}
}

//VVVVVVVV///////////ENEMIES_TANK//////////////VVVVVVVVVVVV///
class Enemies_tank : public Base_tank {
	Mode_enemies enem_mode; //mode
	Type_enemies enem_type; //tank type
	int counter; //счетчик для переключения modeа
public:
	void set_config(char _model); //determine tank model, f - fast, s - slow, m - medium
	void set_dead(int d) { dead = d; }
	Type_enemies ret_type_en() { return enem_type; }
	Mode_enemies ret_mode_en() { return enem_mode; }
	void set_mode_en(Mode_enemies me) { enem_mode = me; }
};

void Enemies_tank::set_config(char _model)
{
	course = DOWN;

	char temp_u[3][3];
	char temp_r[3][3];

	switch (_model)
	{
	case 'f': {
		char _temp_u[3][3] =
		{
			'o', '|', 'o',
			'0', 'O', '0',
			'0', '@', '0'
		};
		char _temp_r[3][3] =
		{
			'0', '0', 'o',
			'@', 'O', '-',
			'0', '0', 'o'
		};
		for (int i = 0; i<3; i++)
			for (int j = 0; j<3; j++) {
				temp_u[i][j] = _temp_u[i][j];
				temp_r[i][j] = _temp_r[i][j];
			}
		enem_type = FAST;
		hp = 1;
		break;
	}
	case 'm': {
		char _temp_u[3][3] =
		{
			'-', '|', '-',
			'-', 'O', '-',
			'-', '@', '-'
		};
		char _temp_r[3][3] =
		{
			'|', '|', '|',
			'@', 'O', '-',
			'|', '|', '|'
		};
		for (int i = 0; i<3; i++)
			for (int j = 0; j<3; j++) {
				temp_u[i][j] = _temp_u[i][j];
				temp_r[i][j] = _temp_r[i][j];
			}
		enem_type = MEDIUM;
		hp = 2;
		break;
	}
	case 's': {
		char _temp_u[3][3] =
		{
			'x', '|', 'x',
			'X', 'O', 'X',
			'X', '@', 'X'
		};
		char _temp_r[3][3] =
		{
			'X', 'X', 'x',
			'@', 'O', '-',
			'X', 'X', 'x'
		};
		for (int i = 0; i<3; i++)
			for (int j = 0; j<3; j++) {
				temp_u[i][j] = _temp_u[i][j];
				temp_r[i][j] = _temp_r[i][j];
			}
		enem_type = SLOW;
		hp = 3;
		break;
	}
	default:
		cout << "shatik_zahvatit_mir";
		exit(3);
	}

	for (int i = 0; i<3; i++) {

		for (int j = 0; j<3; j++) {
			sprite_up[i][j] = temp_u[i][j];
			sprite_right[i][j] = temp_r[i][j];
		}
	}
	hp_resp = 1;
	//bullet_course = STOP;
	dead = 0;
	kostil = ' ';
	enem_mode = RANDOM;
	x = y = 0;
	set_bullet_kd(1);
}

/////////////////////GAME_MAP////////////////////////////////////
class Game_map {  // game field
	Respawns resp;
	char old_map_arr[size_y][size_x];
	char new_map_arr[size_y][size_x];

	bool check_resp(int x, int y); //check respawn for tanks 

public:
	void set_blocks_for_map_lvl(int _i, int _j, char _symb); //fill a 3x3 block
	void gen_map_lvl(int lvl); //map filling
	//-----------
	void show_new_map_arr()
	{
		register int i, j;
		cout << "\n";
		for (i = 0; i<size_y; i++) {
			cout << ' ';
			for (j = 0; j<size_x; j++) {
				cout << new_map_arr[i][j];
			}cout << '\n';
		}
	}
	//-----------
	void bullet_splash(Cell b, int x, int y, Course bul_crs); //block hit processing
	void resp_players(Players_tank &pl_t); //record players' coordinates at the respawn point
	void resp_enemies(Enemies_tank &en_t); //record opponents' coordinates at the respawn points
	bool check_move_tank(Base_tank *b_t); //check tank passage permission
	int check_move_bullet(Base_tank *b_t, int &x, int &y); //check bullet movement and handle hits on blocks. If it hits a bullet, returns <0; if it hits a tank, >0. x, y - coordinates checked ahead of the bullet
	void clear_tank(Base_tank *b_t); //erase tank from the map
	void clear_bullet(Base_tank *b_t); //erase bullet from the map
	void draw_tank(Base_tank *b_t); //draw tank
	void draw_bullet(Base_tank *b_t); //draw bullet
	void change_new_map(Players_tank *pl_t, Enemies_tank *en_t); //changes to the map array
	void show_change_map(); //display changes on the screen
	void swap_new_old_arr(); //swap pseudo-buffers
	int ret_amount_resp(char c); //возврат number of respawns для определенного типа танка противника
	char ret_new_map_arr_ch(int x, int y) { return new_map_arr[y][x]; }
};

bool Game_map::check_resp(int x, int y)
{
	bool temp_ret = true;
	for (int i = y; i < y + 3; i++) 
		for (int j = x; j < x + 3; j++) 
			if (new_map_arr[i][j] != ' ') temp_ret = false;
	return temp_ret;
}

void Game_map::bullet_splash(Cell bc, int x, int y, Course bul_crs)
{
	switch (bul_crs)
	{
	case UP:
	case DOWN: {
		Cell bl((char)new_map_arr[y][x - 1]);
		Cell br((char)new_map_arr[y][x + 1]);
		if (bl.ret_bul_crash()) new_map_arr[y][x - 1] = ' ';
		if (bc.ret_bul_crash()) new_map_arr[y][x] = ' ';
		if (br.ret_bul_crash()) new_map_arr[y][x + 1] = ' ';
		break; }
	case LEFT:
	case RIGHT: {
		Cell bl((char)new_map_arr[y - 1][x]);
		Cell br((char)new_map_arr[y + 1][x]);
		if (bl.ret_bul_crash()) new_map_arr[y - 1][x] = ' ';
		if (bc.ret_bul_crash()) new_map_arr[y][x] = ' ';
		if (br.ret_bul_crash()) new_map_arr[y + 1][x] = ' ';
		break; }
	}
}
void Game_map::set_blocks_for_map_lvl(int _i, int _j, char _symb)
{
	register int i, j;
	for (i = _i; i<_i + 3; i++) {
		for (j = _j; j<_j + 3; j++) {
			new_map_arr[i][j] = _symb;
		}
	}
}
bool Game_map::check_move_tank(Base_tank *b_t)
{
	char c1, c2, c3;
	switch (b_t->ret_course())
	{
	case UP:
		c1 = new_map_arr[b_t->ret_y() - 1][b_t->ret_x()];
		c2 = new_map_arr[b_t->ret_y() - 1][b_t->ret_x() + 1];
		c3 = new_map_arr[b_t->ret_y() - 1][b_t->ret_x() + 2];
		break;
	case RIGHT:
		c1 = new_map_arr[b_t->ret_y()][b_t->ret_x() + 3];
		c2 = new_map_arr[b_t->ret_y() + 1][b_t->ret_x() + 3];
		c3 = new_map_arr[b_t->ret_y() + 2][b_t->ret_x() + 3];
		break;
	case DOWN:
		c1 = new_map_arr[b_t->ret_y() + 3][b_t->ret_x()];
		c2 = new_map_arr[b_t->ret_y() + 3][b_t->ret_x() + 1];
		c3 = new_map_arr[b_t->ret_y() + 3][b_t->ret_x() + 2];
		break;
	case LEFT:
		c1 = new_map_arr[b_t->ret_y()][b_t->ret_x() - 1];
		c2 = new_map_arr[b_t->ret_y() + 1][b_t->ret_x() - 1];
		c3 = new_map_arr[b_t->ret_y() + 2][b_t->ret_x() - 1];
		break;
	}

	Cell b1((char)c1);
	Cell b2((char)c2);
	Cell b3((char)c3);
	if (b1.ret_pass() && b2.ret_pass() && b3.ret_pass()) return true;
	else return false;
}
int Game_map::check_move_bullet(Base_tank *b_t, int &x, int &y)
{
	char c1=' ';
	Course bul_crs = b_t->ret_bullet_course();
	switch (bul_crs)
	{
	case UP:
		x = b_t->ret_bullet_x();
		y = b_t->ret_bullet_y() - 1;
		c1 = new_map_arr[y][x];
		break;
	case RIGHT:
		x = b_t->ret_bullet_x() + 1;
		y = b_t->ret_bullet_y();
		c1 = new_map_arr[y][x];
		break;
	case DOWN:
		x = b_t->ret_bullet_x();
		y = b_t->ret_bullet_y() + 1;
		c1 = new_map_arr[y][x];
		break;
	case LEFT:
		x = b_t->ret_bullet_x() - 1;
		y = b_t->ret_bullet_y();
		c1 = new_map_arr[y][x];
		break;
	case STOP:
		break;
	}

	Cell b1((char)c1);
	if (x >= b_t->ret_x() && x <= b_t->ret_x() + 2 &&
		y >= b_t->ret_y() && y <= b_t->ret_y() + 2) return 0;
	if (b1.ret_bul_pop()) {
		bullet_splash(b1, x, y, bul_crs);
		b_t->stop_bullet_course();
		new_map_arr[b_t->ret_bullet_y()][b_t->ret_bullet_x()] = ' ';
		if (c1 == '*') return -1;
		else if (c1 != '%' && c1 != '#' && c1 != ' ' && c1 != '~') return 1;
	}
	return 0;
}
void Game_map::gen_map_lvl(int lvl)
{
	char map_lvl[size_y / 3][size_x / 3];
	int i, j;
	switch (lvl)
	{
	case 1: {
		char map_lvl[size_y / 3][size_x / 3] =
		{
			'%','%','%','%','%','%','%','%','%','%','%','%','%','%','%','%','%',
			'%',' ','#',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ','#',' ','%',
			'%',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ','%',
			'%',' ',' ','#','#',' ',' ','%',' ','%',' ',' ','#','#',' ',' ','%',
			'%',' ','%','~','~','%','%','%','#','%','%','%','~','~','%',' ','%',
			'%',' ','%','~','~','#','~','#','#','#','~','#','~','~','%',' ','%',
			'%',' ','#',' ',' ',' ','#','#','#','#','#',' ',' ',' ','#',' ','%',
			'%',' ','#',' ',' ','%','%','%','#','%','%','%',' ',' ','#',' ','%',
			'%',' ','#',' ',' ',' ',' ','%','#','%',' ',' ',' ',' ','#',' ','%',
			'%',' ','%','%','%',' ',' ','#','#','#',' ',' ','%','%','%',' ','%',
			'%','#','%',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ','%','#','%',
			'%','#','#','#',' ',' ',' ','#','#','#',' ',' ',' ','#','#','#','%',
			'%','#','#','#',' ',' ',' ','#','$','#',' ',' ',' ','#','#','#','%',
			'%','%','%','%','%','%','%','%','%','%','%','%','%','%','%','%','%'
		};
		resp.point_resp_players[0][0] = 18; //x
		resp.point_resp_players[0][1] = 33; //y
		resp.point_resp_players[1][0] = 30;
		resp.point_resp_players[1][1] = 33;

		resp.point_resp_enemy[0][0] = 3;
		resp.point_resp_enemy[0][1] = 3;
		resp.point_resp_enemy[1][0] = 9;
		resp.point_resp_enemy[1][1] = 3;
		resp.point_resp_enemy[2][0] = 39;
		resp.point_resp_enemy[2][1] = 3;
		resp.point_resp_enemy[3][0] = 45;
		resp.point_resp_enemy[3][1] = 3;

		resp.set_amount(6, 0, 0);

		for (i = 0; i < size_y / 3; i++) {
			for (j = 0; j < size_x / 3; j++) {
				set_blocks_for_map_lvl(i * 3, j * 3, map_lvl[i][j]);
			}
		}
		hard = 10;
		break;
	}
	
	case 2: {
		char map_lvl[size_y / 3][size_x / 3] =
		{
			'%','%','%','%','%','%','%','%','%','%','%','%','%','%','%','%','%',
			'%',' ','#','#',' ','#','#','#','#','#','#','#',' ','#','#',' ','%',
			'%',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ','%',
			'%',' ',' ',' ',' ','~','~','~','~','~','~','~',' ',' ',' ',' ','%',
			'%',' ',' ',' ',' ',' ','~','~','~','~','~',' ',' ',' ',' ',' ','%',
			'%',' ','#',' ',' ',' ',' ','~','~','~',' ',' ',' ',' ','#',' ','%',
			'%',' ','#',' ',' ',' ',' ',' ','~',' ',' ',' ',' ',' ','#',' ','%',
			'%',' ','#','#','%','#',' ','#','#','#',' ','#','%','#','#',' ','%',
			'%',' ','#','#','%','%',' ','%','#','%',' ','%','%','#','#',' ','%',
			'%',' ','#','#','%','%',' ',' ',' ',' ',' ','%','%','#','#',' ','%',
			'%',' ',' ',' ','#','%',' ',' ',' ',' ',' ','%','#',' ',' ',' ','%',
			'%',' ',' ',' ',' ',' ',' ','#','#','#',' ',' ',' ',' ',' ',' ','%',
			'%',' ',' ',' ','#','%',' ','#','$','#',' ','%','#',' ',' ',' ','%',
			'%','%','%','%','%','%','%','%','%','%','%','%','%','%','%','%','%'
		};
		resp.point_resp_players[0][0] = 18; //x
		resp.point_resp_players[0][1] = 33; //y
		resp.point_resp_players[1][0] = 30;
		resp.point_resp_players[1][1] = 33;

		resp.point_resp_enemy[0][0] = 3;
		resp.point_resp_enemy[0][1] = 3;
		resp.point_resp_enemy[1][0] = 12;
		resp.point_resp_enemy[1][1] = 3;
		resp.point_resp_enemy[2][0] = 36;
		resp.point_resp_enemy[2][1] = 3;
		resp.point_resp_enemy[3][0] = 45;
		resp.point_resp_enemy[3][1] = 3;

		resp.set_amount(5, 1, 0);

		for (i = 0; i < size_y / 3; i++) {
			for (j = 0; j < size_x / 3; j++) {
				set_blocks_for_map_lvl(i * 3, j * 3, map_lvl[i][j]);
			}
		}
		hard = 10;
		break;
	}

	case 3: {
		char map_lvl[size_y / 3][size_x / 3] =
		{
			'%','%','%','%','%','%','%','%','%','%','%','%','%','%','%','%','%',
			'%',' ','%','%',' ',' ',' ',' ',' ',' ',' ',' ',' ','%','%',' ','%',
			'%',' ','#','#',' ',' ',' ','~',' ','~',' ',' ',' ','#','#',' ','%',
			'%',' ','~','~',' ',' ',' ','#',' ','#',' ',' ',' ','~','~',' ','%',
			'%',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ','%',
			'%',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ','%',
			'%',' ',' ',' ',' ','~',' ','~','~','~',' ','~',' ',' ',' ',' ','%',
			'%',' ',' ',' ',' ','#','#','#',' ','#','#','#',' ',' ',' ',' ','%',
			'%',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ','%',
			'%',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ','%',
			'%',' ',' ','~','#',' ',' ',' ',' ',' ',' ',' ','#','~',' ',' ','%',
			'%',' ',' ',' ','#',' ',' ','#','#','#',' ',' ','#',' ',' ',' ','%',
			'%',' ',' ','~','#',' ',' ','#','$','#',' ',' ','#','~',' ',' ','%',
			'%','%','%','%','%','%','%','%','%','%','%','%','%','%','%','%','%'
		};
		resp.point_resp_players[0][0] = 18; //x
		resp.point_resp_players[0][1] = 33; //y
		resp.point_resp_players[1][0] = 30;
		resp.point_resp_players[1][1] = 33;

		resp.point_resp_enemy[0][0] = 3;
		resp.point_resp_enemy[0][1] = 3;
		resp.point_resp_enemy[1][0] = 12;
		resp.point_resp_enemy[1][1] = 3;
		resp.point_resp_enemy[2][0] = 36;
		resp.point_resp_enemy[2][1] = 3;
		resp.point_resp_enemy[3][0] = 45;
		resp.point_resp_enemy[3][1] = 3;

		resp.set_amount(5, 3, 0);

		for (i = 0; i < size_y / 3; i++) {
			for (j = 0; j < size_x / 3; j++) {
				set_blocks_for_map_lvl(i * 3, j * 3, map_lvl[i][j]);
			}
		}
		hard = 9;
		break;
	}

	case 4: {
		char map_lvl[size_y / 3][size_x / 3] =
		{
			'%','%','%','%','%','%','%','%','%','%','%','%','%','%','%','%','%',
			'%',' ','%',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ','%',' ','%',
			'%',' ','#',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ','#',' ','%',
			'%',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ','%',
			'%',' ','#',' ','~',' ',' ',' ','%',' ',' ',' ','~',' ','#',' ','%',
			'%',' ',' ',' ','~',' ',' ',' ','%',' ',' ',' ','~',' ',' ',' ','%',
			'%',' ','#',' ','~',' ',' ',' ','%',' ',' ',' ','~',' ','#',' ','%',
			'%',' ',' ',' ','~','~','~','%',' ','%','~','~','~',' ',' ',' ','%',
			'%',' ','#',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ','#',' ','%',
			'%',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ','%',
			'%',' ','#',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ','#',' ','%',
			'%',' ',' ',' ',' ','#',' ','#','#','#',' ','#',' ',' ',' ',' ','%',
			'%','#','#',' ',' ','#',' ','#','$','#',' ','#',' ',' ','#','#','%',
			'%','%','%','%','%','%','%','%','%','%','%','%','%','%','%','%','%'
		};
		resp.point_resp_players[0][0] = 18; //x
		resp.point_resp_players[0][1] = 33; //y
		resp.point_resp_players[1][0] = 30;
		resp.point_resp_players[1][1] = 33;

		resp.point_resp_enemy[0][0] = 3;
		resp.point_resp_enemy[0][1] = 3;
		resp.point_resp_enemy[1][0] = 9;
		resp.point_resp_enemy[1][1] = 3;
		resp.point_resp_enemy[2][0] = 39;
		resp.point_resp_enemy[2][1] = 3;
		resp.point_resp_enemy[3][0] = 45;
		resp.point_resp_enemy[3][1] = 3;

		resp.set_amount(3, 2, 3);

		for (i = 0; i < size_y / 3; i++) {
			for (j = 0; j < size_x / 3; j++) {
				set_blocks_for_map_lvl(i * 3, j * 3, map_lvl[i][j]);
			}
		}
		hard = 9;
		break;
	}

	case 5: {
		char map_lvl[size_y / 3][size_x / 3] =
		{
			'%','%','%','%','%','%','%','%','%','%','%','%','%','%','%','%','%',
			'%',' ','~','~',' ','~',' ',' ',' ',' ',' ','~',' ','~','~',' ','%',
			'%',' ','~','~',' ','~',' ',' ',' ',' ',' ','~',' ','~','~',' ','%',
			'%',' ',' ','%',' ','%','#','#','#','#','#','%',' ','%',' ',' ','%',
			'%',' ',' ','#',' ',' ',' ',' ',' ',' ',' ',' ',' ','#',' ',' ','%',
			'%',' ',' ','#',' ',' ',' ',' ',' ',' ',' ',' ',' ','#',' ',' ','%',
			'%',' ',' ','#','#','#','#','#',' ','#','#','#','#','#',' ',' ','%',
			'%',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ','%',
			'%','%','#',' ','#','#','#','#',' ','#','#','#','#',' ','#','%','%',
			'%','~','~',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ','~','~','%',
			'%','~','~','~',' ',' ',' ',' ',' ',' ',' ',' ',' ','~','~','~','%',
			'%','~','~','~','~',' ',' ','#','#','#',' ',' ','~','~','~','~','%',
			'%','~','~','~','~','~',' ','#','$','#',' ','~','~','~','~','~','%',
			'%','%','%','%','%','%','%','%','%','%','%','%','%','%','%','%','%'
		};
		resp.point_resp_players[0][0] = 18; //x
		resp.point_resp_players[0][1] = 33; //y
		resp.point_resp_players[1][0] = 30;
		resp.point_resp_players[1][1] = 33;

		resp.point_resp_enemy[0][0] = 3;
		resp.point_resp_enemy[0][1] = 3;
		resp.point_resp_enemy[1][0] = 12;
		resp.point_resp_enemy[1][1] = 3;
		resp.point_resp_enemy[2][0] = 36;
		resp.point_resp_enemy[2][1] = 3;
		resp.point_resp_enemy[3][0] = 45;
		resp.point_resp_enemy[3][1] = 3;

		resp.set_amount(4, 4, 6);

		for (i = 0; i < size_y / 3; i++) {
			for (j = 0; j < size_x / 3; j++) {
				set_blocks_for_map_lvl(i * 3, j * 3, map_lvl[i][j]);
			}
		}
		hard = 8;
		break;
	}

	case 6: {
		char map_lvl[size_y / 3][size_x / 3] =
		{
			'%','%','%','%','%','%','%','%','%','%','%','%','%','%','%','%','%',
			'%',' ',' ','#',' ',' ',' ',' ',' ',' ',' ',' ',' ','#',' ',' ','%',
			'%','#',' ','#',' ',' ',' ',' ',' ',' ',' ',' ',' ','#',' ','#','%',
			'%',' ',' ',' ','#',' ',' ','#',' ','#',' ',' ','#',' ',' ',' ','%',
			'%',' ',' ',' ','#',' ',' ','#',' ','#',' ',' ','#',' ',' ',' ','%',
			'%','#','#','#','#',' ',' ','#','#','#',' ',' ','#','#','#','#','%',
			'%',' ',' ',' ',' ','~','~',' ',' ',' ','~','~',' ',' ',' ',' ','%',
			'%',' ',' ',' ',' ','~','~',' ',' ',' ','~','~',' ',' ',' ',' ','%',
			'%','#','#','#','#',' ',' ','#','%','#',' ',' ','#','#','#','#','%',
			'%',' ',' ',' ','~',' ',' ','#',' ','#',' ',' ','~',' ',' ',' ','%',
			'%',' ',' ',' ','%',' ',' ',' ',' ',' ',' ',' ','%',' ',' ',' ','%',
			'%','#',' ','%',' ',' ',' ','#','#','#',' ',' ',' ','%',' ','#','%',
			'%',' ',' ','%',' ',' ',' ','#','$','#',' ',' ',' ','%',' ',' ','%',
			'%','%','%','%','%','%','%','%','%','%','%','%','%','%','%','%','%'
		};
		resp.point_resp_players[0][0] = 18; //x
		resp.point_resp_players[0][1] = 33; //y
		resp.point_resp_players[1][0] = 30;
		resp.point_resp_players[1][1] = 33;

		resp.point_resp_enemy[0][0] = 3;
		resp.point_resp_enemy[0][1] = 3;
		resp.point_resp_enemy[1][0] = 3;
		resp.point_resp_enemy[1][1] = 36;
		resp.point_resp_enemy[2][0] = 45;
		resp.point_resp_enemy[2][1] = 36;
		resp.point_resp_enemy[3][0] = 45;
		resp.point_resp_enemy[3][1] = 3;

		resp.set_amount(5, 4, 4);

		for (i = 0; i < size_y / 3; i++) {
			for (j = 0; j < size_x / 3; j++) {
				set_blocks_for_map_lvl(i * 3, j * 3, map_lvl[i][j]);
			}
		}
		hard = 7;
		break;
	}

	case 7: {
		char map_lvl[size_y / 3][size_x / 3] =
		{
			'%','%','%','%','%','%','%','%','%','%','%','%','%','%','%','%','%',
			'%',' ','%','%',' ',' ',' ',' ',' ',' ',' ',' ',' ','%','%',' ','%',
			'%',' ','#','#',' ','%',' ',' ',' ',' ',' ','%',' ','#','#',' ','%',
			'%',' ',' ',' ',' ',' ','#',' ',' ',' ','#',' ',' ',' ',' ',' ','%',
			'%',' ',' ',' ','#',' ','#',' ',' ',' ','#',' ','#',' ',' ',' ','%',
			'%','%',' ','#',' ',' ',' ','#',' ','#',' ',' ',' ','#',' ','%','%',
			'%',' ','#',' ','#',' ',' ','#',' ','#',' ',' ','#',' ','#',' ','%',
			'%',' ','#',' ',' ','%',' ','~','~','~',' ','%',' ',' ','#',' ','%',
			'%',' ',' ','#',' ',' ',' ',' ','#',' ',' ',' ',' ','#',' ',' ','%',
			'%',' ',' ','#',' ',' ',' ',' ','#',' ',' ',' ',' ','#',' ',' ','%',
			'%',' ',' ','~','#',' ',' ',' ',' ',' ',' ',' ','#','~',' ',' ','%',
			'%',' ',' ','~','#',' ',' ','#','#','#',' ',' ','#','~',' ',' ','%',
			'%',' ',' ','~','#','#',' ','#','$','#',' ','#','#','~',' ',' ','%',
			'%','%','%','%','%','%','%','%','%','%','%','%','%','%','%','%','%'
		};
		resp.point_resp_players[0][0] = 18; //x
		resp.point_resp_players[0][1] = 33; //y
		resp.point_resp_players[1][0] = 30;
		resp.point_resp_players[1][1] = 33;

		resp.point_resp_enemy[0][0] = 3;
		resp.point_resp_enemy[0][1] = 3;
		resp.point_resp_enemy[1][0] = 12;
		resp.point_resp_enemy[1][1] = 3;
		resp.point_resp_enemy[2][0] = 36;
		resp.point_resp_enemy[2][1] = 3;
		resp.point_resp_enemy[3][0] = 45;
		resp.point_resp_enemy[3][1] = 3;

		resp.set_amount(5, 5, 5);

		for (i = 0; i < size_y / 3; i++) {
			for (j = 0; j < size_x / 3; j++) {
				set_blocks_for_map_lvl(i * 3, j * 3, map_lvl[i][j]);
			}
		}
		hard = 7;
		break;
	}

	case 8: {
		char map_lvl[size_y / 3][size_x / 3] =
		{
			'%','%','%','%','%','%','%','%','%','%','%','%','%','%','%','%','%',
			'%',' ',' ',' ',' ',' ',' ',' ','%',' ',' ',' ',' ',' ',' ',' ','%',
			'%',' ','%',' ',' ','%',' ',' ',' ',' ',' ','%',' ',' ','%',' ','%',
			'%',' ','#',' ',' ','#',' ','%',' ','%',' ','#',' ',' ','#',' ','%',
			'%',' ','#',' ',' ','#',' ','#',' ','#',' ','#',' ',' ','#',' ','%',
			'%',' ','#',' ',' ','#',' ','%','~','%',' ','#',' ',' ','#',' ','%',
			'%',' ','#',' ',' ','#',' ',' ',' ',' ',' ','#',' ',' ','#',' ','%',
			'%',' ','#',' ',' ','#',' ',' ',' ',' ',' ','#',' ',' ','#',' ','%',
			'%',' ','#',' ',' ','#','#','%','~','%','#','#',' ',' ','#',' ','%',
			'%',' ','#',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ','#',' ','%',
			'%',' ','#','#','#','#',' ',' ',' ',' ',' ','#','#','#','#',' ','%',
			'%',' ',' ',' ',' ','#',' ','#','#','#',' ','#',' ',' ',' ',' ','%',
			'%','#',' ','#','#','#',' ','#','$','#',' ','#','#','#',' ','#','%',
			'%','%','%','%','%','%','%','%','%','%','%','%','%','%','%','%','%'
		};
		resp.point_resp_players[0][0] = 18; //x
		resp.point_resp_players[0][1] = 33; //y
		resp.point_resp_players[1][0] = 30;
		resp.point_resp_players[1][1] = 33;

		resp.point_resp_enemy[0][0] = 6;
		resp.point_resp_enemy[0][1] = 3;
		resp.point_resp_enemy[1][0] = 15;
		resp.point_resp_enemy[1][1] = 3;
		resp.point_resp_enemy[2][0] = 33;
		resp.point_resp_enemy[2][1] = 3;
		resp.point_resp_enemy[3][0] = 42;
		resp.point_resp_enemy[3][1] = 3;

		resp.set_amount(7, 6, 2);

		for (i = 0; i < size_y / 3; i++) {
			for (j = 0; j < size_x / 3; j++) {
				set_blocks_for_map_lvl(i * 3, j * 3, map_lvl[i][j]);
			}
		}
		hard = 7;
		break;
	}

	case 9: {
		char map_lvl[size_y / 3][size_x / 3] =
		{
			'%','%','%','%','%','%','%','%','%','%','%','%','%','%','%','%','%',
			'%',' ',' ',' ',' ',' ','~','~','~','~','~',' ',' ',' ',' ',' ','%',
			'%',' ','#',' ','#',' ',' ',' ','#',' ',' ',' ','#',' ','#',' ','%',
			'%',' ','#',' ','#','#','%',' ','#',' ','%','#','#',' ','#',' ','%',
			'%',' ',' ',' ',' ',' ',' ',' ','#',' ',' ',' ',' ',' ',' ',' ','%',
			'%',' ','#',' ','#','#','#',' ','~',' ','#','#','#',' ','#',' ','%',
			'%',' ',' ',' ','#','#',' ',' ',' ',' ',' ','#','#',' ',' ',' ','%',
			'%',' ','#',' ','%','#',' ','~',' ','~',' ','#','%',' ','#',' ','%',
			'%',' ','~',' ','~','#',' ',' ',' ',' ',' ','#','~',' ','~',' ','%',
			'%',' ','~',' ','~','%',' ',' ',' ',' ',' ','%','~',' ','~',' ','%',
			'%',' ','~',' ',' ','#',' ',' ',' ',' ',' ','#',' ',' ','~',' ','%',
			'%',' ','~','~','~','%',' ','#','#','#',' ','%','~','~','~',' ','%',
			'%',' ',' ',' ',' ','#',' ','#','$','#',' ','#',' ',' ',' ',' ','%',
			'%','%','%','%','%','%','%','%','%','%','%','%','%','%','%','%','%'
		};
		resp.point_resp_players[0][0] = 18; //x
		resp.point_resp_players[0][1] = 33; //y
		resp.point_resp_players[1][0] = 30;
		resp.point_resp_players[1][1] = 33;

		resp.point_resp_enemy[0][0] = 3;
		resp.point_resp_enemy[0][1] = 3;
		resp.point_resp_enemy[1][0] = 12;
		resp.point_resp_enemy[1][1] = 3;
		resp.point_resp_enemy[2][0] = 36;
		resp.point_resp_enemy[2][1] = 3;
		resp.point_resp_enemy[3][0] = 45;
		resp.point_resp_enemy[3][1] = 3;

		resp.set_amount(0, 7, 10);

		for (i = 0; i < size_y / 3; i++) {
			for (j = 0; j < size_x / 3; j++) {
				set_blocks_for_map_lvl(i * 3, j * 3, map_lvl[i][j]);
			}
		}
		hard = 6;
		break;
	}

	case 10: {
		char map_lvl[size_y / 3][size_x / 3] =
		{
			'%','%','%','%','%','%','%','%','%','%','%','%','%','%','%','%','%',
			'%',' ',' ',' ',' ',' ',' ','#','%','#',' ',' ',' ',' ',' ',' ','%',
			'%','%','%',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ','%','%','%',
			'%',' ',' ','#','#','%',' ',' ',' ',' ',' ','%','#','#',' ',' ','%',
			'%','%',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ','%','%',
			'%',' ',' ',' ',' ','#',' ',' ',' ',' ',' ','#',' ',' ',' ',' ','%',
			'%',' ',' ',' ',' ','#','#',' ',' ',' ','#','#',' ',' ',' ',' ','%',
			'%',' ',' ',' ',' ',' ','#','#',' ','#','#',' ',' ',' ',' ',' ','%',
			'%',' ',' ','~','~',' ',' ','#','#','#',' ',' ','~','~',' ',' ','%',
			'%',' ',' ',' ','~',' ',' ',' ','#',' ',' ',' ','~',' ',' ',' ','%',
			'%','#',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ','#','%',
			'%','%','#',' ',' ',' ',' ','#','#','#',' ',' ',' ',' ','#','%','%',
			'%','%','%','#',' ',' ',' ','#','$','#',' ',' ',' ','#','%','%','%',
			'%','%','%','%','%','%','%','%','%','%','%','%','%','%','%','%','%'
		};
		resp.point_resp_players[0][0] = 18; //x
		resp.point_resp_players[0][1] = 33; //y
		resp.point_resp_players[1][0] = 30;
		resp.point_resp_players[1][1] = 33;

		resp.point_resp_enemy[0][0] = 3;
		resp.point_resp_enemy[0][1] = 3;
		resp.point_resp_enemy[1][0] = 3;
		resp.point_resp_enemy[1][1] = 9;
		resp.point_resp_enemy[2][0] = 45;
		resp.point_resp_enemy[2][1] = 9;
		resp.point_resp_enemy[3][0] = 45;
		resp.point_resp_enemy[3][1] = 3;

		resp.set_amount(5, 9, 9);

		for (i = 0; i < size_y / 3; i++) {
			for (j = 0; j < size_x / 3; j++) {
				set_blocks_for_map_lvl(i * 3, j * 3, map_lvl[i][j]);
			}
		}
		hard = 7;
		break;
	}
//
	}

}
void Game_map::resp_players(Players_tank &pl_t)
{
	pl_t.set_hp(arr_scores[2]);
	pl_t.set_xy(resp.point_resp_players[pl_t.ret_player_numb()][0], resp.point_resp_players[pl_t.ret_player_numb()][1]);
	pl_t.dec_dead();
}
void Game_map::resp_enemies(Enemies_tank &en_t)
{
	int temp_resp, ex;
	if (!check_resp(resp.point_resp_enemy[0][0], resp.point_resp_enemy[0][1]) &&
		!check_resp(resp.point_resp_enemy[1][0], resp.point_resp_enemy[1][1]) &&
		!check_resp(resp.point_resp_enemy[2][0], resp.point_resp_enemy[2][1]) &&
		!check_resp(resp.point_resp_enemy[3][0], resp.point_resp_enemy[3][1])) return;
	do {
		ex = temp_resp = rand() % 3;
		switch (temp_resp)
		{
		case 0:
			if (resp.amount_resp_f == 0) break;
			else {
				en_t.set_config('f');
				resp.amount_resp_f--;
				set_cur(59, 11); cout << "   ";
				set_cur(59, 11); cout << resp.amount_resp_f;
				set_cur(0, 0);
				ex = 100;
			}
			break;
		case 1:
			if (resp.amount_resp_m == 0) break;
			else {
				en_t.set_config('m');
				resp.amount_resp_m--;
				set_cur(59, 15); cout << "   ";
				set_cur(59, 15); cout << resp.amount_resp_m;
				set_cur(0, 0);
				ex = 100;
			}
			break;
		case 2:
			if (resp.amount_resp_s == 0) break;
			else {
				en_t.set_config('s');
				resp.amount_resp_s--;
				set_cur(59, 19); cout << "   ";
				set_cur(59, 19); cout << resp.amount_resp_s;
				set_cur(0, 0);
				ex = 100;
			}
			break;
		}
	} while(ex != 100);

	temp_resp = rand() % 4;
	if (check_resp(resp.point_resp_enemy[temp_resp][0], resp.point_resp_enemy[temp_resp][1])) {
		en_t.set_xy(resp.point_resp_enemy[temp_resp][0], resp.point_resp_enemy[temp_resp][1]);
		en_t.dec_dead();
	}
	else {
		for (int i = 0; i < 4; i++) {
			if (check_resp(resp.point_resp_enemy[i][0], resp.point_resp_enemy[i][1])) {
				en_t.set_xy(resp.point_resp_enemy[i][0], resp.point_resp_enemy[i][1]);
				en_t.dec_dead();
				break;
			}
		}
	}
}
void Game_map::clear_tank(Base_tank *b_t)
{
	int i, j, _i, _j;
	_i = b_t->ret_y();
	_j = b_t->ret_x();
	for (i = _i; i<_i + 3; i++)
		for (j = _j; j<_j + 3; j++)
			new_map_arr[i][j] = ' ';
}
void Game_map::clear_bullet(Base_tank *b_t)
{

	new_map_arr[b_t->ret_bullet_y()][b_t->ret_bullet_x()] = ' ';

}
void Game_map::draw_tank(Base_tank *b_t)
{
	int i, _i, j, _j;
	Course crs = b_t->ret_course();
	switch (crs)
	{
	case UP:
	case RIGHT:
		for (i = b_t->ret_y(), _i = 0; _i<3; i++, _i++)
			for (j = b_t->ret_x(), _j = 0; _j<3; j++, _j++)
				new_map_arr[i][j] = b_t->ret_sprite(_j, _i, crs);
		break;
	case DOWN:
	case LEFT:
		for (i = b_t->ret_y(), _i = 2; _i >= 0; i++, _i--)
			for (j = b_t->ret_x(), _j = 2; _j >= 0; j++, _j--)
				new_map_arr[i][j] = b_t->ret_sprite(_j, _i, crs);
		break;
	}
}
void Game_map::draw_bullet(Base_tank *b_t)
{

	new_map_arr[b_t->ret_bullet_y()][b_t->ret_bullet_x()] = '*';

}
void Game_map::change_new_map(Players_tank *pl_t, Enemies_tank *en_t)
{
	int t_b, x, y; //принимает значение от проверки снаряда, координаты проверок снаряда


	for (int i = 0; i < players_col; i++) {
		draw_tank(&pl_t[i]);
		pl_t[i].dec_bullet_kd();
	}

	for (int i = 0; i < 4; i++) {
		draw_tank(&en_t[i]);
		en_t[i].dec_bullet_kd();
	}

	for (int i = 0; i < players_col; i++) {
		if (pl_t[i].ret_dead() > 0) pl_t[i].dec_dead();
		else if (pl_t[i].ret_dead() == 0 && pl_t[i].ret_hp_resp() > 0) {
			resp_players(pl_t[i]);

			if (i == 0) set_cur(53, 5);
			else set_cur(70, 5);
			if (pl_t[i].ret_hp_resp() > 0) cout << "RESPAWNS - " << pl_t[i].ret_hp_resp() - 1;
			else cout << "DEAD!!!!      ";

			if (i == 0) set_cur(53, 6);
			else set_cur(70, 6);
			cout << "              ";
			if (i == 0) set_cur(53, 6);
			else set_cur(70, 6);
			if (pl_t[i].ret_hp() > 0) cout << "HP - " << pl_t[i].ret_hp();
			else cout << "DEAD!!!!      ";
		}

		if (pl_t[i].ret_bullet_course() != STOP) {
			t_b = check_move_bullet(&pl_t[i], x, y);

			if (t_b == -1) {
				for (int j = 0; j < players_col; j++) {
					if (pl_t[j].ret_bullet_x() == x && pl_t[j].ret_bullet_y() == y) {
						pl_t[j].stop_bullet_course();
						if (pl_t[j].kostil == '~') new_map_arr[pl_t[j].ret_bullet_y()][pl_t[j].ret_bullet_x()] = '~';
					}
				}
				for (int j = 0; j < 4; j++) {
					if (en_t[j].ret_bullet_x() == x && en_t[j].ret_bullet_y() == y) {
						en_t[j].stop_bullet_course();
						if (en_t[j].kostil == '~') new_map_arr[en_t[j].ret_bullet_y()][en_t[j].ret_bullet_x()] = '~';
					}
				}
			}
			else if (t_b == 1) {
				for (int j = 0; j < players_col; j++) {
					if (x >= pl_t[j].ret_x() && x <= pl_t[j].ret_x() + 2 &&
						y >= pl_t[j].ret_y() && y <= pl_t[j].ret_y() + 2) {

						clear_tank(&pl_t[j]);
						pl_t[j].kill_tank();

						if (j == 0) set_cur(53, 5);
						else set_cur(70, 5);
						if (pl_t[j].ret_hp_resp() > 0) cout << "RESPAWNS - " << pl_t[j].ret_hp_resp() - 1;
						else cout << "DEAD!!!!      ";
						set_cur(0, 0);

						if (j == 0) set_cur(53, 6);
						else set_cur(70, 6);
						cout << "              ";
						if (j == 0) set_cur(53, 6);
						else set_cur(70, 6);
						if (pl_t[j].ret_hp() > 0) cout << "HP - " << pl_t[j].ret_hp();
						else cout << "DEAD!!!!      ";
					}
				}
				for (int j = 0; j < 4; j++) {
					if (x >= en_t[j].ret_x() && x <= en_t[j].ret_x() + 2 &&
						y >= en_t[j].ret_y() && y <= en_t[j].ret_y() + 2) {
						clear_tank(&en_t[j]);
						en_t[j].kill_tank();
						switch (en_t[j].ret_type_en())
						{
						case FAST:
							arr_scores[0] = (arr_scores[0] + 5 + (game_lvl * 1));
							break;
						case MEDIUM:
							arr_scores[0] = (arr_scores[0] + 7 + (game_lvl * 1.5));
							break;
						case SLOW:
							arr_scores[0] = (arr_scores[0] + 9 + (game_lvl * 2));
							break;
						}
						set_cur(53, 1); cout << "COINS - " << "      ";
						set_cur(53, 1); cout << "COINS - " << arr_scores[0];
						set_cur(0, 0);
					}
				}
			}

			clear_bullet(&pl_t[i]);
			if (pl_t[i].kostil == '~') new_map_arr[pl_t[i].ret_bullet_y()][pl_t[i].ret_bullet_x()] = '~';

			if (pl_t[i].ret_bullet_course() == STOP) continue;

			pl_t[i].move_bullet();
			if (new_map_arr[pl_t[i].ret_bullet_y()][pl_t[i].ret_bullet_x()] == '~') pl_t[i].kostil = '~';
			else pl_t[i].kostil = ' ';

			draw_bullet(&pl_t[i]);
		}
	}

	for (int i = 0; i < 4; i++) {
		if (en_t[i].ret_bullet_course() != STOP) {
			t_b = check_move_bullet(&en_t[i], x, y);

			if (t_b == -1) {
				for (int j = 0; j < players_col; j++) {
					if (pl_t[j].ret_bullet_x() == x && pl_t[j].ret_bullet_y() == y) {
						pl_t[j].stop_bullet_course();
						if (pl_t[j].kostil == '~') new_map_arr[pl_t[j].ret_bullet_y()][pl_t[j].ret_bullet_x()] = '~';
					}
				}
				for (int j = 0; j < 4; j++) {
					if (en_t[j].ret_bullet_x() == x && en_t[j].ret_bullet_y() == y) {
						en_t[j].stop_bullet_course();
						if (en_t[j].kostil == '~') new_map_arr[en_t[j].ret_bullet_y()][en_t[j].ret_bullet_x()] = '~';
					}
				}
			}
			else if (t_b == 1) {
				for (int j = 0; j < 4; j++) {
					if (x >= en_t[j].ret_x() && x <= en_t[j].ret_x() + 2 &&
						y >= en_t[j].ret_y() && y <= en_t[j].ret_y() + 2) {
						if (en_t[j].kostil == '~') new_map_arr[en_t[j].ret_bullet_y()][en_t[j].ret_bullet_x()] = '~';
					}
				}
				for (int j = 0; j < players_col; j++) {
					if (x >= pl_t[j].ret_x() && x <= pl_t[j].ret_x() + 2 &&
						y >= pl_t[j].ret_y() && y <= pl_t[j].ret_y() + 2) {
						clear_tank(&pl_t[j]);
						pl_t[j].kill_tank();

						if (j == 0) set_cur(53, 5);
						else set_cur(70, 5);
						if (pl_t[j].ret_hp_resp() > 0) cout << "RESPAWNS - " << pl_t[j].ret_hp_resp() - 1;
						else cout << "DEAD!!!!      ";
						set_cur(0, 0);

						if (j == 0) set_cur(53, 6);
						else set_cur(70, 6);
						cout << "              ";
						if (j == 0) set_cur(53, 6);
						else set_cur(70, 6);
						if (pl_t[j].ret_hp() > 0) cout << "HP - " << pl_t[j].ret_hp();
						else cout << "DEAD!!!!      ";
					}
				}
			}

			clear_bullet(&en_t[i]);
			if (en_t[i].kostil == '~') new_map_arr[en_t[i].ret_bullet_y()][en_t[i].ret_bullet_x()] = '~';

			if (en_t[i].ret_bullet_course() == STOP) continue;

			en_t[i].move_bullet();
			if (new_map_arr[en_t[i].ret_bullet_y()][en_t[i].ret_bullet_x()] == '~') en_t[i].kostil = '~';
			else en_t[i].kostil = ' ';

			draw_bullet(&en_t[i]);
		}
	}

	for (int i = 0; i<players_col; i++)
		draw_tank(&pl_t[i]);

	for (int i = 0; i<4; i++) {
		if (en_t[i].ret_dead()>0) en_t[i].dec_dead();
		else if (en_t[i].ret_dead() == 0 && 
			resp.amount_resp_f + resp.amount_resp_m + resp.amount_resp_s > 0) resp_enemies(en_t[i]);
	}

	for (int i = 0; i<4; i++)
		draw_tank(&en_t[i]);

}
void Game_map::show_change_map()
{
	register int i, j;
	for (i = 3; i<size_y - 3; i++)
		for (j = 3; j<size_x - 3; j++)
			if (old_map_arr[i][j] != new_map_arr[i][j]) {
				set_cur(j + 1, i + 1);
				cout << new_map_arr[i][j];
				set_cur(0, 0);
			}
}
void Game_map::swap_new_old_arr()
{
	register int i, j;
	for (int i = 3; i<size_y - 3; i++)
		for (int j = 3; j<size_x - 3; j++)
			old_map_arr[i][j] = new_map_arr[i][j];
}
int Game_map::ret_amount_resp(char c)
{
	switch (c)
	{
	case 'f': return resp.amount_resp_f;
	case 'm': return resp.amount_resp_m;
	case 's': return resp.amount_resp_s;
	}
}
///////////////////////OTHER/////////////////////////////////
void control_tank(Game_map &g_m, Players_tank &pl_t) //handle key presses
{
	Base_tank *b_t;
	b_t = &pl_t;
	switch (pl_t.ret_player_numb())
	{
	case FIRST:
		if (GetKeyState('W') < 0) {
			if (b_t->ret_course() == UP && g_m.check_move_tank(b_t)) { g_m.clear_tank(b_t); b_t->move_tank(UP); }
			else b_t->set_course(UP);
		}
		if (GetKeyState('D') < 0) {
			if (b_t->ret_course() == RIGHT && g_m.check_move_tank(b_t)) { g_m.clear_tank(b_t); b_t->move_tank(RIGHT); }
			else b_t->set_course(RIGHT);
		}
		if (GetKeyState('S') < 0) {
			if (b_t->ret_course() == DOWN && g_m.check_move_tank(b_t)) { g_m.clear_tank(b_t); b_t->move_tank(DOWN); }
			else b_t->set_course(DOWN);
		}
		if (GetKeyState('A') < 0) {
			if (b_t->ret_course() == LEFT && g_m.check_move_tank(b_t)) { g_m.clear_tank(b_t); b_t->move_tank(LEFT); }
			else b_t->set_course(LEFT);
		}
		break;
	case SECOND:
		if (GetKeyState(VK_UP) < 0) {
			if (b_t->ret_course() == UP && g_m.check_move_tank(b_t)) { g_m.clear_tank(b_t); b_t->move_tank(UP); }
			else b_t->set_course(UP);
		}
		if (GetKeyState(VK_RIGHT) < 0) {
			if (b_t->ret_course() == RIGHT && g_m.check_move_tank(b_t)) { g_m.clear_tank(b_t); b_t->move_tank(RIGHT); }
			else b_t->set_course(RIGHT);
		}
		if (GetKeyState(VK_DOWN) < 0) {
			if (b_t->ret_course() == DOWN && g_m.check_move_tank(b_t)) { g_m.clear_tank(b_t); b_t->move_tank(DOWN); }
			else b_t->set_course(DOWN);
		}
		if (GetKeyState(VK_LEFT) < 0) {
			if (b_t->ret_course() == LEFT && g_m.check_move_tank(b_t)) { g_m.clear_tank(b_t); b_t->move_tank(LEFT); }
			else b_t->set_course(LEFT);
		}
		break;
	}
}

void control_bullet(Game_map &g_m, Players_tank &pl_t)
{
	Base_tank *b_t;
	b_t = &pl_t;
	switch (pl_t.ret_player_numb())
	{
	case FIRST:
		if (GetKeyState('E') < 0) {
			b_t->start_bullet();
			b_t->set_bullet_kd(4);
			if (game_lvl >= 10) arr_scores[0] -= 4;
			else if (game_lvl >= 7) arr_scores[0] -= 3;
			else if (game_lvl >= 5) arr_scores[0] -= 2;
			else if (game_lvl >=2) arr_scores[0] -= 1;
			set_cur(53, 1); cout << "COINS - " << "      ";
			set_cur(53, 1); cout << "COINS - " << arr_scores[0];
			set_cur(0, 0);
		}
		break;
	case SECOND:
		if (GetKeyState('P') < 0) {
			b_t->start_bullet();
			b_t->set_bullet_kd(4);
			if (game_lvl >= 10) arr_scores[0] -= 4;
			else if (game_lvl >= 7) arr_scores[0] -= 3;
			else if (game_lvl >= 5) arr_scores[0] -= 2;
			else if (game_lvl >= 2) arr_scores[0] -= 1;
			set_cur(53, 1); cout << "COINS - " << "      ";
			set_cur(53, 1); cout << "COINS - " << arr_scores[0];
			set_cur(0, 0);
		}
		break;
	}
}

void enemies_tank_logic(Game_map &g_m, Enemies_tank &en_t, Players_tank *pl_t, int mode)
{
	Course temp_c = DOWN;
		if (en_t.ret_dead() < 0) {
			if (rand() % hard == 2 && mode < 3) {
				double temp_dist=0,d=0;
				int x, y;
				switch (mode)
				{
				case 3:
				case 0: 
					x = 25;
					y = 37;
					break;
				case 1:
					if (pl_t[0].ret_dead() >= 0) {
						x = 25;
						y = 37;
					} else {
						x = pl_t[0].ret_x();
						y = pl_t[0].ret_y();
					}
					break;
				case 2:
					if (players_col == 2) {
						if (pl_t[1].ret_dead() >= 0) {
							x = 25;
							y = 37;
						}
						else {
							x = pl_t[1].ret_x();
							y = pl_t[1].ret_y();
						}
					}
					else {
						if (pl_t[0].ret_dead() >= 0) {
							x = 25;
							y = 37;
						}
						else {
							x = pl_t[0].ret_x();
							y = pl_t[0].ret_y();
						}
					}
					break;
				}

				en_t.set_course(UP); temp_dist = d = sqrt(pow((x - (en_t.ret_x() + 1)), 2) + pow((y - (en_t.ret_y() - 1)), 2));
				if (g_m.check_move_tank(&en_t) && temp_dist >= d) { temp_dist = d; temp_c = UP; }

				en_t.set_course(RIGHT); d = sqrt(pow((x - (en_t.ret_x() + 3)), 2) + pow((y - (en_t.ret_y() + 1)), 2));
				if (g_m.check_move_tank(&en_t) && temp_dist >= d) { temp_dist = d; temp_c = RIGHT; }

				en_t.set_course(DOWN); d = sqrt(pow((x - (en_t.ret_x() + 1)), 2) + pow((y - (en_t.ret_y() + 3)), 2));
				if (g_m.check_move_tank(&en_t) && temp_dist >= d) { temp_dist = d; temp_c = DOWN; }

				en_t.set_course(LEFT); d = sqrt(pow((x - (en_t.ret_x() - 1)), 2) + pow((y - (en_t.ret_y() + 1)), 2));
				if (g_m.check_move_tank(&en_t) && temp_dist >= d) { temp_dist = d; temp_c = LEFT; }

				en_t.set_course(temp_c);
			}
			if (g_m.check_move_tank(&en_t)) {
				if (rand() % 15 == 10) {
					switch (rand() % 4)
					{
					case 0: temp_c = UP; break;
					case 1: temp_c = RIGHT; break;
					case 2: temp_c = DOWN; break;
					case 3: temp_c = LEFT; break;
					}
					en_t.set_course(temp_c);
				}
				else {
					g_m.clear_tank(&en_t);
					en_t.move_tank(en_t.ret_course());
				}
			}
			else {
				switch (rand() % 4)
				{
				case 0: temp_c = UP; break;
				case 1: temp_c = RIGHT; break;
				case 2: temp_c = DOWN; break;
				case 3: temp_c = LEFT; break;
				}
				en_t.set_course(temp_c);
			}
			if (en_t.ret_bullet_course() == STOP && en_t.ret_bullet_kd() == 0 && rand()%2==0) {
				en_t.start_bullet();
				en_t.set_bullet_kd(5);
			}
		}	

}

BOOL ShowConsoleCursor(BOOL bShow)
{
	CONSOLE_CURSOR_INFO cci;
	HANDLE hStdOut;
	hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
	if (hStdOut == INVALID_HANDLE_VALUE)
		return FALSE;
	if (!GetConsoleCursorInfo(hStdOut, &cci))
		return FALSE;
	cci.bVisible = bShow;
	if (!SetConsoleCursorInfo(hStdOut, &cci))
		return FALSE;
	return TRUE;
}

void start_menu(int s) //game start menu
{
	int price[3][6] = //upgrade prices
	{
		0,50,1500,20,0,0, //speed
		0,200,1000,2500,50,60, //hp
		1000,2000,3000,30,40,0 //resp
	};

	int select_menu = 0;

	ifstream in("scores", ios::in | ios::binary);
	if (!in) {
		ofstream out("scores", ios::out | ios::binary);
		out.write((char*)&arr_scores, sizeof arr_scores);
		out.close();
	}
	in.read((char*)&arr_scores, sizeof arr_scores);
	in.close();

	cout << "\n";
	for (int i = 0; i < size_y; i++) {
		cout << " ";
		for (int j = 0; j < size_x; j++) {
			if (((j >= 0 && j <= 2) || (j >= 48 && j <= 50)) || ((i >= 0 && i <= 2) || (i >= 39 && i <= 41))) cout << '%';
			else cout << " ";
		}
		cout << "\n";
		Sleep(40);
	}


	set_cur(5, 6); cout << "====|=== =|=====| ||\\   || ||   // =|====|"; Sleep(10);
	set_cur(5, 7); cout << "   ||    ||    || ||\\\\  || ||  //  ||      "; Sleep(10);
	set_cur(5, 8); cout << "   ||    ||    || || \\\\ || ||==|   =|====|"; Sleep(10);
	set_cur(5, 9); cout << "   ||    ||=====| ||  \\\\|| ||  \\\\       ||"; Sleep(10); 
	set_cur(5, 10); cout << "   ||    ||    || ||   \\\\| ||   \\\\ =|====|"; Sleep(10);
	set_cur(5, 11); cout << "                                 BY SHATIK"; Sleep(30);

	set_cur(6, 20); cout << "Start game"; Sleep(20);
	set_cur(6, 22); cout << "Upgrade tank"; Sleep(20);
	set_cur(6, 24); cout << "Instruction"; Sleep(20);
	set_cur(6, 26); cout << "Reset progress"; Sleep(20);
	set_cur(6, 28); cout << "Exit"; Sleep(20);

	set_cur(6, 38); cout << "Menu control: arrows, esc, enter";

	do {
		if (GetKeyState(VK_DOWN) < 0) {
			select_menu++;
		} else
		if (GetKeyState(VK_UP) < 0) {
			select_menu--;
		} else
		if (GetKeyState(VK_RETURN) < 0 || s==1) {
			for (int i = 6; i < 12; i++) {
				set_cur(5, i);
				cout << "                                          "; Sleep(10);
			}
			set_cur(6, 20); cout << "                    "; Sleep(20);
			set_cur(6, 22); cout << "                    "; Sleep(20);
			set_cur(6, 24); cout << "                    "; Sleep(20);
			set_cur(6, 26); cout << "                    "; Sleep(20);
			set_cur(6, 28); cout << "                    "; Sleep(20);
			s = 0;
			switch (select_menu)
			{
			case 0:
			{
				char ch;
				int predel=-1;
				select_menu = 0;
				set_cur(6, 7); cout << "Select lvl:";
				ifstream in("saves", ios::in | ios::binary);
				if (!in) {
					ofstream out("saves", ios::out | ios::binary);
					out.write((char*)&arr_saves, sizeof arr_saves);
					out.close();
				}
				in.read((char*)&arr_saves, sizeof arr_saves);
				in.close();
				for (int i = 0; i < 10; i++) {
					set_cur(8, 9 + (i*2));
					cout << i+1;
					if (!arr_saves[i]) cout << " (locked!)";
					else predel++;
					Sleep(20);
				}
				do {
					if (GetKeyState(VK_DOWN) < 0) {
						set_cur(10, 9 + (select_menu * 2)); cout << "       ";
						select_menu++;
					} else
					if (GetKeyState(VK_UP) < 0) {
						set_cur(10, 9 + (select_menu * 2)); cout << "       ";
						select_menu--;
					} else
					if (GetKeyState(VK_RETURN) < 0) {
						game_lvl = select_menu+1;

						select_menu = 0;

						for (int i = 6; i < 29; i++) {
							set_cur(5, i); cout << "                          ";
							Sleep(10);
						}
						set_cur(6, 7); cout << "Select amount of players:"; Sleep(20);
						set_cur(7, 9); cout << "1"; Sleep(20);
						set_cur(7, 11); cout << "2"; Sleep(20);

						do {
							if (GetKeyState(VK_DOWN) < 0) {
								set_cur(10, 9 + (select_menu * 2)); cout << "      ";
								select_menu++;
							}
							else
							if (GetKeyState(VK_UP) < 0) {
								set_cur(10, 9 + (select_menu * 2)); cout << "      ";
								select_menu--;
							} else
							if (GetKeyState(VK_RETURN) < 0) {
								players_col = select_menu + 1;
								return;
							} else
							if (GetKeyState(VK_ESCAPE) < 0) {
								for (int i = 6; i < 15; i++) {
									set_cur(5, i); cout << "                          ";
									Sleep(10);
								}
								set_cur(6, 7); cout << "Select lvl:";
								predel = -1;
								for (int i = 0; i < 10; i++) {
									set_cur(8, 9 + (i * 2));
									cout << i + 1;
									if (!arr_saves[i]) cout << " (locked!)";
									else predel++;
									Sleep(20);
								}
								break;
							}

							if (select_menu == 2) select_menu = 0;
							if (select_menu == -1) select_menu = 1;
							set_cur(10, 9 + (select_menu * 2));

							cout << "<-----";

							set_cur(0, 0);
							Sleep(100);
							ch = _getch();
							if (ch == 13 || ch == 27) continue;
							_getch();
						} while (true);
					} else
					if (GetKeyState(VK_ESCAPE) < 0) {
						for (int i = 6; i < 29; i++) {
							set_cur(5, i); cout << "                          ";
							Sleep(10);
						}
						set_cur(5, 6); cout << "====|=== =|=====| ||\\   || ||   // =|====|"; Sleep(10);
						set_cur(5, 7); cout << "   ||    ||    || ||\\\\  || ||  //  ||      "; Sleep(10);
						set_cur(5, 8); cout << "   ||    ||    || || \\\\ || ||==|   =|====|"; Sleep(10);
						set_cur(5, 9); cout << "   ||    ||=====| ||  \\\\|| ||  \\\\       ||"; Sleep(10);
						set_cur(5, 10); cout << "   ||    ||    || ||   \\\\| ||   \\\\ =|====|"; Sleep(10);
						set_cur(5, 11); cout << "                                 BY SHATIK"; Sleep(30);
						set_cur(6, 20); cout << "Start game"; Sleep(20);
						set_cur(6, 22); cout << "Upgrade tank"; Sleep(20);
						set_cur(6, 24); cout << "Instruction"; Sleep(20);
						set_cur(6, 26); cout << "Reset progress"; Sleep(20);
						set_cur(6, 28); cout << "Exit"; Sleep(20);
						break;
					}

					if (select_menu == predel+1) select_menu = 0;
					if (select_menu == -1) select_menu = predel;
					if (select_menu > 8) set_cur(11, 9 + (select_menu * 2));
						else set_cur(10, 9 + (select_menu * 2)); 
					
					cout << "<-----";

					set_cur(0, 0);
					Sleep(100);
					ch = _getch();
					if (ch == 13 || ch == 27) continue;
					_getch();
				} while (true);
				break;
			}
			case 1:
			{
				bool max[3] = { true,true,true };
				char ch;
				select_menu = 0;
				set_cur(6, 7); cout << "Coins: " << arr_scores[0]; Sleep(20);
				set_cur(6, 9); cout << "/|\\ /-\\  Speed: " << arr_scores[1]; Sleep(10);
				set_cur(6, 10); cout << "|O| @O-  Hp: " << arr_scores[2]; Sleep(10);
				set_cur(6, 11); cout << "\\@/ \\-/  Respawns: " << arr_scores[3]; Sleep(20);

				if (arr_scores[1] == 3) max[0] = false;
				if (arr_scores[2] == 4) max[1] = false;
				if (arr_scores[3] == 2) max[2] = false;

				set_cur(8, 13); cout << "Upgrade speed - "; if (max[0]) cout << price[0][arr_scores[1]] << " coins"; else cout << "maximum!"; Sleep(20);
				set_cur(8, 15); cout << "Upgrade hp - "; if (max[1]) cout << price[1][arr_scores[2]] << " coins"; else cout << "maximum!"; Sleep(20);
				set_cur(8, 17); cout << "Upgrade respawns - "; if (max[2]) cout << price[2][arr_scores[3]] << " coins"; else cout << "maximum!"; Sleep(20);

				do {
					if (GetKeyState(VK_DOWN) < 0) {
						select_menu++;
					}
					else
					if (GetKeyState(VK_UP) < 0) {
						select_menu--;
					}
					else
					if (GetKeyState(VK_RETURN) < 0) {
						switch (select_menu)
						{
						case 0:
							if (price[0][arr_scores[1]] <= arr_scores[0] && max[0]) {
								arr_scores[0] -= price[0][arr_scores[1]];
								arr_scores[1]++;
							}
							break;
						case 1:
							if (price[1][arr_scores[2]] <= arr_scores[0] && max[1]) {
								arr_scores[0] -= price[1][arr_scores[2]];
								arr_scores[2]++;
							}
							break;
						case 2:
							if (price[2][arr_scores[3]] <= arr_scores[0] && max[2]) {
								arr_scores[0] -= price[2][arr_scores[3]];
								arr_scores[3]++;
							}
							break;
						}
						if (arr_scores[1] == 3) max[0] = false;
						if (arr_scores[2] == 4) max[1] = false;
						if (arr_scores[3] == 2) max[2] = false;
						set_cur(6, 7); cout << "Coins:                       ";  set_cur(6, 7); cout << "Coins: " << arr_scores[0];

						set_cur(22, 9); cout << "          "; set_cur(22, 9); cout << arr_scores[1];
						set_cur(19, 10); cout << "          "; set_cur(19, 10); cout << arr_scores[2];
						set_cur(25, 11); cout << "          "; set_cur(25, 11); cout << arr_scores[3];

						set_cur(8, 13); cout << "Upgrade speed -                      "; set_cur(8, 13); cout << "Upgrade speed - "; 
						if (max[0]) cout << price[0][arr_scores[1]] << " coins"; else cout << "maximum!"; 
						set_cur(8, 15); cout << "Upgrade hp -                       "; set_cur(8, 15); cout << "Upgrade hp - ";
						if (max[1]) cout << price[1][arr_scores[2]] << " coins"; else cout << "maximum!"; 
						set_cur(8, 17); cout << "Upgrade respawns -                      "; set_cur(8, 17); cout << "Upgrade respawns - ";
						if (max[2]) cout << price[2][arr_scores[3]] << " coins"; else cout << "maximum!"; 
					}
					else
					if (GetKeyState(VK_ESCAPE) < 0) {

						ofstream out("scores", ios::out | ios::binary);
						out.write((char*)&arr_scores, sizeof arr_scores);
						out.close();

						for (int i = 6; i < 20; i++) {
							set_cur(5, i); cout << "                                         ";
							Sleep(10);
						}
						set_cur(5, 6); cout << "====|=== =|=====| ||\\   || ||   // =|====|"; Sleep(10);
						set_cur(5, 7); cout << "   ||    ||    || ||\\\\  || ||  //  ||      "; Sleep(10);
						set_cur(5, 8); cout << "   ||    ||    || || \\\\ || ||==|   =|====|"; Sleep(10);
						set_cur(5, 9); cout << "   ||    ||=====| ||  \\\\|| ||  \\\\       ||"; Sleep(10);
						set_cur(5, 10); cout << "   ||    ||    || ||   \\\\| ||   \\\\ =|====|"; Sleep(10);
						set_cur(5, 11); cout << "                                 BY SHATIK"; Sleep(30);
						set_cur(6, 20); cout << "Start game"; Sleep(20);
						set_cur(6, 22); cout << "Upgrade tank"; Sleep(20);
						set_cur(6, 24); cout << "Instruction"; Sleep(20);
						set_cur(6, 26); cout << "Reset progress"; Sleep(20);
						set_cur(6, 28); cout << "Exit"; Sleep(20);
						break;
					}

					if (select_menu == 3) select_menu = 0;
					if (select_menu == -1) select_menu = 2;
					switch (select_menu)
					{
					case 0:
						set_cur(39, 17); cout << "      ";
						set_cur(32, 15); cout << "      ";
						set_cur(35, 13); cout << "<-----";
						break;
					case 1:
						set_cur(39, 17); cout << "      ";
						set_cur(35, 13); cout << "      ";
						set_cur(32, 15); cout << "<-----";
						break;
					case 2:
						set_cur(32, 15); cout << "      ";
						set_cur(35, 13); cout << "      ";
						set_cur(39, 17); cout << "<-----";
						break;
					}

					set_cur(0, 0);
					Sleep(100);
					ch = _getch();
					if (ch == 13 || ch == 27) continue;
					_getch();
				} while (true);
				break;
			}
			case 2:
			{
				set_cur(6, 5); cout << "BLOCKS:";
				set_cur(8, 7); cout << "%%%           ###";
				set_cur(8, 8); cout << "%%% - steel   ### - brick";
				set_cur(8, 9); cout << "%%%           ###";
				set_cur(8, 11); cout << "~~~           $$$";
				set_cur(8, 12); cout << "~~~ - water   $$$ - base";
				set_cur(8, 13); cout << "~~~           $$$";
				set_cur(6, 15); cout << "CONTROL:";
				set_cur(8, 17); cout << "       player 1:    player 2:";
				set_cur(8, 18); cout << "move - W,A,S,D      arrows   ";
				set_cur(8, 19); cout << "shot - E            P   ";
				set_cur(6, 21); cout << "THE ENEMY:";
				set_cur(8, 23); cout << "o|o 00o speed - 3";
				set_cur(8, 24); cout << "0O0 @O- hp - 1";
				set_cur(8, 25); cout << "0@0 00o";
				set_cur(8, 27); cout << "-|- ||| speed - 2";
				set_cur(8, 28); cout << "-O- @O- hp - 2";
				set_cur(8, 29); cout << "-@- ||| ";
				set_cur(8, 31); cout << "x|x XXx speed - 1";
				set_cur(8, 32); cout << "XOX @O- hp - 3";
				set_cur(8, 33); cout << "X@X XXx";
				
				do {
					char ch;
					if (GetKeyState(VK_ESCAPE) < 0) {
					for (int i = 4; i < 34; i++) {
						set_cur(5, i); cout << "                                  ";
						Sleep(10);
					}
					set_cur(5, 6); cout << "====|=== =|=====| ||\\   || ||   // =|====|"; Sleep(10);
					set_cur(5, 7); cout << "   ||    ||    || ||\\\\  || ||  //  ||      "; Sleep(10);
					set_cur(5, 8); cout << "   ||    ||    || || \\\\ || ||==|   =|====|"; Sleep(10);
					set_cur(5, 9); cout << "   ||    ||=====| ||  \\\\|| ||  \\\\       ||"; Sleep(10);
					set_cur(5, 10); cout << "   ||    ||    || ||   \\\\| ||   \\\\ =|====|"; Sleep(10);
					set_cur(5, 11); cout << "                                 BY SHATIK"; Sleep(30);
					set_cur(6, 20); cout << "Start game"; Sleep(20);
					set_cur(6, 22); cout << "Upgrade tank"; Sleep(20);
					set_cur(6, 24); cout << "Instruction"; Sleep(20);
					set_cur(6, 26); cout << "Reset progress"; Sleep(20);
					set_cur(6, 28); cout << "Exit"; Sleep(20);
					break;
				}

					set_cur(0, 0);
					Sleep(100);
					ch = _getch();
					if (ch == 13 || ch == 27) continue;
					_getch();
				} while (true);
				break;
			}
			case 3:
			{
				select_menu = 0;

				set_cur(6, 7); cout << "Reset progress?"; Sleep(20);
				set_cur(7, 9); cout << "Yes"; Sleep(20);
				set_cur(7, 11); cout << "No"; Sleep(20);

				do {
					char ch;
					if (GetKeyState(VK_DOWN) < 0) {
						set_cur(12, 9 + (select_menu * 2)); cout << "      ";
						select_menu++;
					}
					else
						if (GetKeyState(VK_UP) < 0) {
							set_cur(12, 9 + (select_menu * 2)); cout << "      ";
							select_menu--;
						}
						else
							if (GetKeyState(VK_RETURN) < 0) {
								switch (select_menu)
								{
								case 0:
								{
									arr_scores[0] = 0;
									arr_scores[1] = 1;
									arr_scores[2] = 1;
									arr_scores[3] = 0;
									arr_saves[0] = true;
									for (int i = 1; i < 10; i++) arr_saves[i] = false;
									ofstream out1("scores", ios::out | ios::binary);
									out1.write((char*)&arr_scores, sizeof arr_scores);
									out1.close();
									ofstream out2("saves", ios::out | ios::binary);
									out2.write((char*)&arr_saves, sizeof arr_saves);
									out2.close();
								}
								case 1:
									for (int i = 6; i < 15; i++) {
										set_cur(5, i); cout << "                          ";
										Sleep(10);
									}
									set_cur(5, 6); cout << "====|=== =|=====| ||\\   || ||   // =|====|"; Sleep(10);
									set_cur(5, 7); cout << "   ||    ||    || ||\\\\  || ||  //  ||      "; Sleep(10);
									set_cur(5, 8); cout << "   ||    ||    || || \\\\ || ||==|   =|====|"; Sleep(10);
									set_cur(5, 9); cout << "   ||    ||=====| ||  \\\\|| ||  \\\\       ||"; Sleep(10);
									set_cur(5, 10); cout << "   ||    ||    || ||   \\\\| ||   \\\\ =|====|"; Sleep(10);
									set_cur(5, 11); cout << "                                 BY SHATIK"; Sleep(30);
									set_cur(6, 20); cout << "Start game"; Sleep(20);
									set_cur(6, 22); cout << "Upgrade tank"; Sleep(20);
									set_cur(6, 24); cout << "Instruction"; Sleep(20);
									set_cur(6, 26); cout << "Reset progress"; Sleep(20);
									set_cur(6, 28); cout << "Exit"; Sleep(20);
									break;
								}
								break;
							}
							else
								if (GetKeyState(VK_ESCAPE) < 0) {
									for (int i = 6; i < 15; i++) {
										set_cur(5, i); cout << "                          ";
										Sleep(10);
									}
									set_cur(5, 6); cout << "====|=== =|=====| ||\\   || ||   // =|====|"; Sleep(10);
									set_cur(5, 7); cout << "   ||    ||    || ||\\\\  || ||  //  ||      "; Sleep(10);
									set_cur(5, 8); cout << "   ||    ||    || || \\\\ || ||==|   =|====|"; Sleep(10);
									set_cur(5, 9); cout << "   ||    ||=====| ||  \\\\|| ||  \\\\       ||"; Sleep(10);
									set_cur(5, 10); cout << "   ||    ||    || ||   \\\\| ||   \\\\ =|====|"; Sleep(10);
									set_cur(5, 11); cout << "                                 BY SHATIK"; Sleep(30);
									set_cur(6, 20); cout << "Start game"; Sleep(20);
									set_cur(6, 22); cout << "Upgrade tank"; Sleep(20);
									set_cur(6, 24); cout << "Instruction"; Sleep(20);
									set_cur(6, 26); cout << "Reset progress"; Sleep(20);
									set_cur(6, 28); cout << "Exit"; Sleep(20);
									break;
								}

					if (select_menu == 2) select_menu = 0;
					if (select_menu == -1) select_menu = 1;
					set_cur(12, 9 + (select_menu * 2));

					cout << "<-----";

					set_cur(0, 0);
					Sleep(100);
					ch = _getch();
					if (ch == 13 || ch == 27) continue;
					_getch();
				} while (true);
				break;
			}
			case 4:
				exit(0);
				break;
			}
		}
		if (select_menu == 5) select_menu = 0;
		if (select_menu == -1) select_menu = 4;
		switch (select_menu)
		{
		case 0:
			set_cur(11, 28); cout << "      ";
			set_cur(21, 26); cout << "      ";
			set_cur(18, 24); cout << "      ";
			set_cur(19, 22); cout << "      ";
			set_cur(17, 20); cout << "<-----";
			break;
		case 1:
			set_cur(11, 28); cout << "      ";
			set_cur(21, 26); cout << "      ";
			set_cur(18, 24); cout << "      ";
			set_cur(17, 20); cout << "      ";
			set_cur(19, 22); cout << "<-----";
			break;
		case 2:
			set_cur(11, 28); cout << "      ";
			set_cur(21, 26); cout << "      ";
			set_cur(19, 22); cout << "      ";
			set_cur(17, 20); cout << "      ";
			set_cur(18, 24); cout << "<-----";
			break;
		case 3:
			set_cur(11, 28); cout << "      ";
			set_cur(18, 24); cout << "      ";
			set_cur(19, 22); cout << "      ";
			set_cur(17, 20); cout << "      ";
			set_cur(21, 26); cout << "<-----";
			break;
		case 4:
			set_cur(21, 26); cout << "      ";
			set_cur(18, 24); cout << "      ";
			set_cur(19, 22); cout << "      ";
			set_cur(17, 20); cout << "      ";
			set_cur(11, 28); cout << "<-----";
			break;
		}
		set_cur(0, 0);
		Sleep(100);
		if (_getch()==13) continue;
		_getch();
	} while (true);

	set_cur(0, 0);
	Sleep(3000);
}

///////////////////////MAIN////////////////////////////////
int main()
{

	void* handle = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_CURSOR_INFO structCursorInfo;
	GetConsoleCursorInfo(handle, &structCursorInfo);
	structCursorInfo.bVisible = FALSE;
	SetConsoleCursorInfo(handle, &structCursorInfo);

	ShowCursor(FALSE);
	system("mode con cols=110 lines=55");
	SetConsoleCP(1251); SetConsoleOutputCP(1251); srand(time(0));
	
	start_menu(0);

	do {

		int i;

		Game_map gm;

		gm.gen_map_lvl(game_lvl);
		gm.show_new_map_arr();

		Players_tank *pt = new Players_tank[players_col];
		Enemies_tank *et = new Enemies_tank[4];

		for (i = 0; i < 4; i++) {
			et[i].set_dead(30 * (i + 1));
			et[i].stop_bullet_course();
		}

		Base_tank *b_pt;

		
		game_over = arr_scores[3];

		for (i = players_col-1; i >= 0; i--) {
			pt[i].set_config((i + 1) + '0');
			gm.resp_players(pt[i]);
			pt[i].set_hp(arr_scores[2]);
			pt[i].set_hp_resp(arr_scores[3] + 1);
		}

		arr_scores[3] = game_over;
		game_over = 0; //true

		b_pt = &pt[0];

		gm.change_new_map(pt, et);
		gm.show_change_map();

		set_cur(53, 1); cout << "COINS - " << arr_scores[0];

		set_cur(53, 3); cout << "PLAYER 1";
		set_cur(53, 4); cout << "--------";
		set_cur(53, 5); cout << "RESPAWNS - " << pt[0].ret_hp_resp() - 1;
		set_cur(53, 6); cout << "HP - " << pt[0].ret_hp();
		set_cur(0, 0);

		if (players_col == 2) {
			set_cur(70, 3); cout << "PLAYER 2";
			set_cur(70, 4); cout << "--------";
			set_cur(70, 5); cout << "RESPAWNS - " << pt[1].ret_hp_resp() - 1;
			set_cur(70, 6); cout << "HP - " << pt[1].ret_hp();
			set_cur(0, 0);
		}

		set_cur(53, 8); cout << "ENEMIES";
		set_cur(53, 9); cout << "-------";

		et[0].set_config('f');
		for (i = 0; i < 3; i++)
			for (int j = 0; j < 3; j++) {
				set_cur(53 + i, 10 + j);
				cout << et[0].ret_sprite(i, j, UP);
			}
		set_cur(57, 11); cout << "- " << gm.ret_amount_resp('f');

		et[0].set_config('m');
		for (i = 0; i < 3; i++)
			for (int j = 0; j < 3; j++) {
				set_cur(53 + i, 14 + j);
				cout << et[0].ret_sprite(i, j, UP);
			}
		set_cur(57, 15); cout << "- " << gm.ret_amount_resp('m');

		et[0].set_config('s');
		for (i = 0; i < 3; i++)
			for (int j = 0; j < 3; j++) {
				set_cur(53 + i, 18 + j);
				cout << et[0].ret_sprite(i, j, UP);
			}
		set_cur(57, 19); cout << "- " << gm.ret_amount_resp('s');

		do {

			switch (arr_scores[1])
			{
			case 1:
				if (up_c == 1) {
					for (i = 0; i < players_col; i++)
						control_tank(gm, pt[i]);
				}
				break;
			case 2:
				if (!(up_c % 2)) {
					for (i = 0; i < players_col; i++)
						control_tank(gm, pt[i]);
				}
				break;
			case 3:
				if (up_c < 4) {
					for (i = 0; i < players_col; i++)
						control_tank(gm, pt[i]);
				}
				break;
			}

			for (i = 0; i < 4; i++) {
				switch (et[i].ret_type_en())
				{
				case FAST:
					if (up_c < 4) enemies_tank_logic(gm, et[i], pt, i);
					break;
				case MEDIUM:
					if (!(up_c % 2)) enemies_tank_logic(gm, et[i], pt, i);
					break;
				case SLOW:
					if (up_c == 1) enemies_tank_logic(gm, et[i], pt, i);
					break;
				}
			}

			gm.change_new_map(pt, et);
			gm.show_change_map();
			gm.swap_new_old_arr();

			if (++up_c == 5) up_c = 1; //redraw

			for (i = 0; i < players_col; i++)
				if (pt[i].ret_bullet_course() == STOP && pt[i].ret_bullet_kd() == 0 && pt[i].ret_dead()<0)
					control_bullet(gm, pt[i]);

			if (!pt[0].ret_hp_resp()) {
				if (players_col == 2) {
					if (!pt[1].ret_hp_resp()) {
						game_over = -1;
					}
				}
				else {
					game_over = -1;
				}
			}
			for (int i = 24; i < 27; i++) {
				for (int j = 36; j < 39; j++) {
					if (gm.ret_new_map_arr_ch(i, j) != '$') {
						if (game_lvl>1) arr_scores[0] = arr_scores[0] - (10 * game_lvl);
						set_cur(53, 1); cout << "COINS - " << "      ";
						set_cur(53, 1); cout << "COINS - " << arr_scores[0];
						set_cur(0, 0);
						game_over = -1;
					}
				}
			}
			if (gm.ret_amount_resp('f') == 0 && gm.ret_amount_resp('s') == 0 && gm.ret_amount_resp('m') == 0 &&
				et[0].ret_dead() >= 0 && et[1].ret_dead() >= 0 && et[2].ret_dead() >= 0 && et[3].ret_dead() >= 0) {
				game_over = 1;
			}

			if (game_over == -1) {
				Sleep(1000);
				break;
			}
			if (game_over == 1) {
				arr_scores[0] = arr_scores[0] + (10*game_lvl);
				set_cur(53, 1); cout << "COINS - " << "      ";
				set_cur(53, 1); cout << "COINS - " << arr_scores[0];
				set_cur(0, 0);
				Sleep(1000);
				arr_saves[game_lvl] = true;
				ofstream out("saves", ios::out | ios::binary);
				out.write((char*)&arr_saves, sizeof arr_saves);
				out.close();
				break;
			}
			Sleep(70);

		} while (true);

		for (int i = 1; i < 22; i++) {
			set_cur(53, i);
			cout << "                                              ";
			Sleep(10);
		}

		set_cur(0, 0);

		ofstream out("scores", ios::out | ios::binary);
		out.write((char*)&arr_scores, sizeof arr_scores);
		out.close();

		start_menu(1);

	} while (true);

	return 0;
}
