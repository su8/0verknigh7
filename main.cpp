/*
Copyright 03/13/2025 https://github.com/su8/0verknigh7
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
MA 02110-1301, USA.
*/
#include <iostream>
#include <cstdlib>
#include <random>
#include <algorithm>
#include <array>
#include <vector>
#include <string>
#include <utility>

#ifdef _WIN32
#include <conio.h>
#include <windows.h>
#endif /* _WIN32 */

struct ivec2 // our basic 2d point plus a few operations
{
    int x = 0;
    int y = 0;
    ivec2 operator*(int s) const { return { x * s,y * s }; }
    ivec2 operator+(ivec2 q) const { return { x + q.x,y + q.y }; }
    ivec2 operator%(ivec2 q) const { return { x % q.x,y % q.y }; }
    bool operator ==(ivec2 q) const {return x==q.x && y==q.y;}
};

enum {FLOOR =0, EXIT, TREASURE, ORB, ENEMY, LIFE, HERO}; // cell flags, in display order
constexpr std::array<ivec2, 4> nbo{ivec2{1,0},{0,1},{-1,0},{0,-1}}; // direction offsets
constexpr char symbols[] = ".>$YDL@"; // symbols for the bit defines
constexpr ivec2 dims{ 33,21 };       // map dimensions

/// Game state
int keys[6];
std::array<uint8_t, dims.x* dims.y> map;
int treasure = 0; // collected treasure
int level = 0;
int steps = 0;
int dynamite = 0;
int life = 1;
ivec2 hero;
std::vector<std::pair<ivec2,int>> enemy_position_and_last_direction;
int ch;

// function prototypes
static int read_key(void);
static void bit_set(uint8_t& bits, int pos);
static void bit_clear(uint8_t& bits, int pos);
static bool bit_test(uint8_t& bits, int pos);
//static bool oob(ivec2 p);
static bool oobb(ivec2 p);
static uint8_t &mapelem(ivec2 p);
static void setup_keys(void);
static void create_iteration(ivec2 p);
static void place_feature(int feature_bit, int num);
static void clearscreen(unsigned short int clearOnWin);
static void display(void);
static void create_level(void);
static void startgame(void);
static void use_dynamite(void);
static void move(int feature_bit, ivec2& from, ivec2 to);
static inline void moveEnemy(void);
static inline void moveHero(void);
static void win(void);
static void welcome(void);

#ifdef _WIN32
static int read_key(void)
{
    int c = _getch();
    if (c == 0 || c == 224) // in some cases on windows(?), pressing arrow keys returns two of these values, where the first is 0 or 224.
        c = _getch() + (c<<8);
    return c;
}
#else
#include <termios.h>
#include <unistd.h>
static int read_key(void)
{
   char buf = 0;
   struct termios old = {0};
   fflush(stdout);
   if(tcgetattr(0, &old) < 0)
       perror("tcsetattr()");
   old.c_lflag &= (~ICANON) & (~ECHO); // local modes = Non Canonical mode and Disable echo
   old.c_cc[VMIN] = 1;  // control chars (MIN value) = 1
   old.c_cc[VTIME] = 0; // control chars (TIME value) = 0 (No time)
   if(tcsetattr(0, TCSANOW, &old) < 0)
       perror("tcsetattr ICANON");
   if(read(0, &buf, 1) < 0)
       perror("read()");
   old.c_lflag |= ICANON | ECHO; // local modes = Canonical mode and Enable echo
   if(tcsetattr(0, TCSADRAIN, &old) < 0)
       perror("tcsetattr ~ICANON");
   return buf;
}
#endif /* _WIN32 */

static void bit_set(uint8_t& bits, int pos) { bits |= 1 << pos; }
static void bit_clear(uint8_t& bits, int pos) { bits &= ~(1UL << pos); }
static bool bit_test(uint8_t& bits, int pos) { return bits & (1 << pos); }

// out of bounds
//static bool oob(ivec2 p) { return p.x < 0 || p.y < 0 || p.x >= dims.x || p.y >= dims.y;  }
// out of bounds or on border
static bool oobb(ivec2 p) { return p.x <= 0 || p.y <= 0 || p.x >= (dims.x-1) || p.y >= (dims.y-1); }
// get the map element at position
static uint8_t& mapelem(ivec2 p) { return map[p.x + p.y * dims.x]; }

static void setup_keys(void)
{
    int i = 0;
    std::cout<<"Press key for ";
    for (auto text : { "RIGHT",", UP",", LEFT",", DOWN",", DYNAMITE", " and RESTART" })
    {
        std::cout<<text;
        keys[i++] = read_key();
    }
}

// a step of the algorithm, given an active, carved cell
static void create_iteration(ivec2 p)
{
    std::random_device rd;
    std::mt19937 g(rd());
    // for each of the 4 directions, in random order
    std::array<int, 4> dir4_index{ 0,1,2,3 };
    std::shuffle(dir4_index.begin(), dir4_index.end(), g);
    for (auto i : dir4_index)
    {
        auto nb = p + nbo[i] * 2; // get the nb coord
        if (oobb(nb) || mapelem(nb) != 0) // if oob or already visited, skip
            continue;
        mapelem(nb) = 1; // otherwise, mark it as visited (carve)
        mapelem(p + nbo[i]) = 1; // ... and also carve the intermediate tile, to make a path there
        create_iteration(nb); // Now repeat the process from this current tile
    }
}

static void place_feature(int feature_bit, int num)
{
    int floors_traversed = 0;
    int place_at_multiples_of = 50 + (std::rand()%100);
    while(num > 0)
        for(auto& m : map)
            if(m == 1 && (++floors_traversed % place_at_multiples_of) == 0)
            {
                bit_set( m, feature_bit);
                --num;
            }
}

static void clearscreen(unsigned short int clearOnWin) {
#ifdef _WIN32
    if (clearOnWin == 1U) std::cout << "\033[2J\033[0;0H";
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    COORD Position;
    Position.X = 0;
    Position.Y = 0;
    SetConsoleCursorPosition(hOut, Position);
#else
    static_cast<void>(clearOnWin);
    std::cout << "\033[2J\033[0;0H";
#endif /*_WIN32 */
}

static void display(void)
{
    static const std::vector<std::pair<char, std::string>> reqColour = { {'@', "\033[1;32m@\033[0;0m"}, {'D', "\033[1;31mD\033[0;0m"}, {'$', "\033[1;36m$\033[0;0m"}, {'>', "\033[1;34m>\033[0;0m"}, {'Y', "\033[1;35mY\033[0;0m"}, {'#', "\033[1;33m#\033[0;0m"}, {'L', "\033[1;36mL\033[0;0m"} };
    clearscreen(0U);
    for (int y = dims.y - 1; y >= 0; --y)
    {
        for (int x = 0; x < dims.x; ++x)
        {
            auto m = map[x+y*dims.x];
            char c = '#';
            for (int j = 0; j <= HERO; ++j)
                if (m & (1 << j))
                    c = symbols[j];
            unsigned short int foundReqChar = 0U;
            for (const auto &[key, val] : reqColour) {
                if (key == c) {
                    std::cout << val;
                    foundReqChar = 1U;
                    break;
                }
            }
            if (foundReqChar == 0U)
                std::cout << c;
        }
        if      (y == (dims.y - 1)) std::cout << "  Level:    " << level;
        else if (y == (dims.y - 2)) std::cout << "  Treasure: " << treasure;
        else if (y == (dims.y - 3)) std::cout << "  Dynamite: " << dynamite;
        else if (y == (dims.y - 4)) std::cout << "  Steps:    " << steps;
        else if (y == (dims.y - 5)) std::cout << "  Life(s):  " << life;
        std::cout << '\n';
    }
}

static void create_level(void)
{
    ++level;
    map.fill(0); // clear the map
    ivec2 p = { (std::rand() % (dims.x-1)) | 1, (std::rand() % (dims.y-1)) | 1 }; // pick a point (odd coords) and carve it
    mapelem(p) = 1;
    create_iteration(p); // run the algorithm
    bit_set( mapelem(p) , HERO);
    place_feature(TREASURE, 10);
    place_feature(ENEMY, 4+level); 
    place_feature(EXIT, 1); 
    place_feature(LIFE, 1);
    int orbchance = 10*(level-15);
    if( (std::rand()%100) < orbchance)
        place_feature(ORB, 1);
    enemy_position_and_last_direction.clear();
    hero = p;
    for (p.y = 0; p.y < dims.y; ++p.y)
        for (p.x = 0; p.x < dims.x; ++p.x)
            if (bit_test(mapelem(p), ENEMY))
                enemy_position_and_last_direction.emplace_back(p, -1 /*invalid direction*/);

    // remove some walls
    int to_remove = 20;
    while(to_remove > 0)
    {
        auto& m = mapelem(ivec2{std::rand(),std::rand()}%dims);
        if (m == 0)
        {
            m = 1;
            --to_remove;
        }
    }
}

static void startgame(void)
{
#ifdef _WIN32
    std::cout << "\033[2J\033[0;0H";
#endif /* _WIN32 */
    treasure = 0;
    level = 0;
    steps = 0;
    dynamite = 3;
    life = 1;
    create_level();
}

// dynamite clears neighbour walls and enemies
static void use_dynamite(void)
{
  --dynamite;
  auto& vec = enemy_position_and_last_direction;
  for(int i=0;i<4;++i)
  {
    auto p = (hero + nbo[i] + dims)%dims;
    auto& m = mapelem(p);
    if(bit_test(m, ENEMY))
        vec.erase(std::remove_if(vec.begin(), vec.end(), [p](auto x){return x.first == p;}), vec.end());
    bit_clear(m, ENEMY);
    bit_set(m, FLOOR);
  }
}

static inline void win(void) // ansi shadow font from TAAG
{
  clearscreen(1U);
  std::cout<<"\n\n"<< R"(
██╗   ██╗ ██████╗ ██╗   ██╗    ██╗    ██╗ ██████╗ ███╗   ██╗
╚██╗ ██╔╝██╔═══██╗██║   ██║    ██║    ██║██╔═══██╗████╗  ██║
 ╚████╔╝ ██║   ██║██║   ██║    ██║ █╗ ██║██║   ██║██╔██╗ ██║
  ╚██╔╝  ██║   ██║██║   ██║    ██║███╗██║██║   ██║██║╚██╗██║
   ██║   ╚██████╔╝╚██████╔╝    ╚███╔███╔╝╚██████╔╝██║ ╚████║
   ╚═╝    ╚═════╝  ╚═════╝      ╚══╝╚══╝  ╚═════╝ ╚═╝  ╚═══╝
)";
    std::cout << "\n... as the mighty orb is yours to wield!\n\n";
    constexpr char tabs[] = "\t\t\t\t\t";
    std::cout << tabs<<"Treasure    : "<<treasure<<'\n';
    std::cout << tabs<<"Steps       : "<<steps<<"\n\n";
    std::cout << tabs<<"Dynamite    : "<<dynamite<<"\n\n";
    std::cout << tabs<<"Total score : "<< int((treasure + 10*dynamite)*1000/(float)(steps+1))<<'\n';
    getchar();
    startgame();
}

static void welcome(void) // ansi shadow font from TAAG
{
  std::cout << "\n\n"<<R"(
 ██████╗ ██╗   ██╗███████╗███████╗████████╗    ███████╗ ██████╗ ██████╗     
██╔═══██╗██║   ██║██╔════╝██╔════╝╚══██╔══╝    ██╔════╝██╔═══██╗██╔══██╗    
██║   ██║██║   ██║█████╗  ███████╗   ██║       █████╗  ██║   ██║██████╔╝    
██║▄▄ ██║██║   ██║██╔══╝  ╚════██║   ██║       ██╔══╝  ██║   ██║██╔══██╗    
╚██████╔╝╚██████╔╝███████╗███████║   ██║       ██║     ╚██████╔╝██║  ██║    
 ╚══▀▀═╝  ╚═════╝ ╚══════╝╚══════╝   ╚═╝       ╚═╝      ╚═════╝ ╚═╝  ╚═╝    
                                                                            
        ████████╗██╗  ██╗███████╗     ██████╗ ██████╗ ██████╗               
        ╚══██╔══╝██║  ██║██╔════╝    ██╔═══██╗██╔══██╗██╔══██╗              
           ██║   ███████║█████╗      ██║   ██║██████╔╝██████╔╝              
           ██║   ██╔══██║██╔══╝      ██║   ██║██╔══██╗██╔══██╗              
           ██║   ██║  ██║███████╗    ╚██████╔╝██║  ██║██████╔╝              
           ╚═╝   ╚═╝  ╚═╝╚══════╝     ╚═════╝ ╚═╝  ╚═╝╚═════╝               
)" <<"\n\n\n\n\t\t\tWelcome brave adventurer, and may fate shine upon you!\n\n";
}

static void move(int feature_bit, ivec2& from, ivec2 to)
{
    bit_clear( mapelem(from), feature_bit); // clear
    bit_set( mapelem(to), feature_bit); // set
    from = to;
    auto v = mapelem(to);
    if (bit_test(v, HERO) && bit_test(v, ENEMY))
    {
        --life;
        if (life == 0)
            startgame();
    }
    else if (bit_test(v, HERO) && bit_test(v, TREASURE))
    {
        ++treasure;
        bit_clear(mapelem(to), TREASURE);
    }
    else if (bit_test(v, HERO) && bit_test(v, LIFE))
    {
        ++life;
        bit_clear(mapelem(to), LIFE);
    }
    else if (bit_test(v, HERO) && bit_test(v, ORB))
        win();
    else if (bit_test(v, HERO) && bit_test(v, EXIT))
        create_level();
}

static inline void moveEnemy(void)
{
    for (auto& e : enemy_position_and_last_direction)
    {
        int i0 = std::rand() % 4; // pick random start dir
        bool moved = false;
        for (int i = 0; i<4;++i) // try all dirs, starting at the random one
        {
            int iDirOpposite = (i0 + i+2) % 4;
            if (iDirOpposite == e.second) // don't go opposite of last direction
                continue;
            int iDir = (i0 + i) % 4;
            auto p = (e.first + nbo[iDir] + dims)%dims; // wrap around map
            if (bit_test(mapelem(p), FLOOR)) // if movable target pos, go there
            {
                if (!bit_test(mapelem(p), ENEMY)) // don't swallow ENEMY when colliding 2 or more enemies
                {
                    move(ENEMY, e.first, p);
                    e.second = iDir;
                    moved = true;
                    break;
                }
            }
        }
        if (!moved) // failed to move? Can surely go opposite of last position
        {
            int iDirOpposite = (e.second + 2) % 4;
            move(ENEMY, e.first, e.first + nbo[iDirOpposite]);
            e.second = iDirOpposite;
        }
    }
}

static inline void moveHero(void)
{
    ivec2 dir; // defaults to 0
    for(int i=0;i<4;++i) // set direction if correct key pressed
        if (ch == keys[i])
            dir = nbo[i];
    auto q = (hero + dir + dims)%dims; // make new point
    if (mapelem(q) > 0) // if it's not a wall
    {
        move(HERO, hero, q);
        ++steps;
    }
}

int main(void) {
#ifdef _WIN32
    SetConsoleTitle("0verknigh7");
#endif /*_WIN32 */
    welcome(); // show welcome screen
    setup_keys(); // setup movement, dynamite and restart game keys
    startgame(); 
    while (true) // main loop
    {
        display();
        ch = read_key();
        if (ch == keys[5]) // restart key pressed?
            startgame();
        if(ch == keys[4] && dynamite > 0) // dynamite key pressed?
            use_dynamite();
        else // might be a movement key then
            moveHero(); // Move Hero

        moveEnemy(); // Move all enemies
    }
    return EXIT_SUCCESS;
}