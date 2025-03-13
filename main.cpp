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

int read_key(void);

#ifdef _WIN32
#include <conio.h>
int read_key(void)
{
    int c = _getch();
    if (c == 0 || c == 224) // in some cases on windows(?), pressing arrow keys returns two of these values, where the first is 0 or 224.  
        c = _getch() + (c<<8);
    return c;
}
#else
#include <termios.h>
#include <unistd.h>
int read_key(void) 
{
   char buf = 0;
   struct termios old = {0};
   fflush(stdout);
   if(tcgetattr(0, &old) < 0)
       perror("tcsetattr()");
   old.c_lflag &= (~ICANON) & (~ECHO);
   old.c_cc[VMIN] = 1;
   old.c_cc[VTIME] = 0;
   if(tcsetattr(0, TCSANOW, &old) < 0)
       perror("tcsetattr ICANON");
   if(read(0, &buf, 1) < 0)
       perror("read()");
   old.c_lflag |= ICANON | ECHO;
   if(tcsetattr(0, TCSADRAIN, &old) < 0)
       perror("tcsetattr ~ICANON");
   return buf;
}
#endif

using namespace std;

struct ivec2 // our basic 2d point plus a few operations
{
    int x = 0;
    int y = 0;
    ivec2 operator*(int s) const { return { x * s,y * s }; }
    ivec2 operator+(ivec2 q) const { return { x + q.x,y + q.y }; }
    ivec2 operator%(ivec2 q) const { return { x % q.x,y % q.y }; }
    bool operator ==(ivec2 q) const {return x==q.x && y==q.y;}
};

enum {FLOOR =0, EXIT, TREASURE, ORB, ENEMY, HERO}; // cell flags, in display order
constexpr array<ivec2, 4> nbo{ivec2{1,0},{0,1},{-1,0},{0,-1}}; // direction offsets
constexpr char symbols[] = ".>$YD@"; // symbols for the bit defines
constexpr ivec2 dims{ 33,21 };       // map dimensions

/// Game state
int keys[6];
array<uint8_t, dims.x* dims.y> map;
int treasure = 0; // collected treasure
int level = 0;
int steps = 0;
int dynamite = 0;
ivec2 hero;
vector<pair<ivec2,int>> enemy_position_and_last_direction;

// function prototypes
void bit_set(uint8_t& bits, int pos);
void bit_clear(uint8_t& bits, int pos);
bool bit_test(uint8_t& bits, int pos);
bool oob(ivec2 p);
bool oobb(ivec2 p);
uint8_t &mapelem(ivec2 p);
void setup_keys(void);
void create_iteration(ivec2 p);
void place_feature(int feature_bit, int num);
void clearscreen(void);
void display(void);
void create_level(int seed);
void startgame(void);
void use_dynamite(void);
void move(int feature_bit, ivec2& from, ivec2 to);
void win(void);
void welcome(void);

void bit_set(uint8_t& bits, int pos) { bits |= 1 << pos; }
void bit_clear(uint8_t& bits, int pos) { bits &= ~(1UL << pos); }
bool bit_test(uint8_t& bits, int pos) { return bits & (1 << pos); }

// out of bounds
bool oob(ivec2 p) { return p.x < 0 || p.y < 0 || p.x >= dims.x || p.y >= dims.y;  }
// out of bounds or on border
bool oobb(ivec2 p) { return p.x <= 0 || p.y <= 0 || p.x >= (dims.x-1) || p.y >= (dims.y-1); }
// get the map element at position
uint8_t& mapelem(ivec2 p) { return map[p.x + p.y * dims.x]; }

void setup_keys(void)
{
    int i = 0;
    cout<<"Press key for ";
    for (auto text : { "RIGHT",", UP",", LEFT",", DOWN",", DYNAMITE", " and RESTART" })
    {
        cout<<text;
        keys[i++] = read_key();
    }
}

// a step of the algorithm, given an active, carved cell
void create_iteration(ivec2 p)
{
    // for each of the 4 directions, in random order
    array<int, 4> dir4_index{ 0,1,2,3 };
    random_shuffle(dir4_index.begin(), dir4_index.end());
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

void place_feature(int feature_bit, int num)
{
    int floors_traversed = 0;
    int place_at_multiples_of = 50 + (rand()%100);
    while(num > 0)
        for(auto& m : map)
            if(m == 1 && (++floors_traversed % place_at_multiples_of) == 0)
            {
                bit_set( m, feature_bit);
                --num;
            }
}

void clearscreen(void) { cout << "\033[2J\033[0;0H"; }

void display(void)
{
    static const vector<string> reqColour = {"\033[1;32m@\033[0;0m", "\033[1;31mD\033[0;0m", "\033[1;33m$\033[0;0m", "\033[1;34m>\033[0;0m"};
    static const char reqChar[] = {'@', 'D', '$', '>'};

    clearscreen();
    for (int y = dims.y - 1; y >= 0; --y)
    {
        for (int x = 0; x < dims.x; ++x)
        {
            auto m = map[x+y*dims.x];
            char c = '#';
            for (int j = 0; j <= HERO; ++j)
                if (m & (1 << j))
                    c = symbols[j];
            unsigned short int increment = 0;
            unsigned short int foundReqChar = 0U;
            for (auto &key : reqColour) {
                if (reqChar[increment] == c) {
                    cout << key;
                    foundReqChar = 1U;
                    break;
                }
                increment++;
            }
           if (foundReqChar == 0)
               cout << c;
        }
        if      (y == (dims.y - 1)) cout << "  Level:    " << level;
        else if (y == (dims.y - 2)) cout << "  Treasure: " << treasure;
        else if (y == (dims.y - 3)) cout << "  Dynamite: " << dynamite;
        else if (y == (dims.y - 4)) cout << "  Steps:    " << steps;
        cout << '\n';
    }
}

void create_level(int seed)
{
    ++level;
    srand(seed); // seed the randomiser
    map.fill(0); // clear the map
    ivec2 p = { (rand() % (dims.x-1)) | 1, (rand() % (dims.y-1)) | 1 }; // pick a point (odd coords) and carve it
    mapelem(p) = 1;
    create_iteration(p); // run the algorithm
    bit_set( mapelem(p) , HERO);
    place_feature(TREASURE, 10);
    place_feature(ENEMY, 4+level); 
    place_feature(EXIT, 1); 
    int orbchance = 10*(level-15);
    if( (rand()%100) < orbchance)
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
        auto& m = mapelem(ivec2{rand(),rand()}%dims);
        if (m == 0)
        {
            m = 1;
            --to_remove;
        }
    }
}

void startgame(void)
{
    treasure = 0;
    level = 0;
    steps = 0;
    dynamite = 3;
    create_level(time(NULL));
}

// dynamite clears neighbour walls and enemies
void use_dynamite(void)
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

void win(void) // ansi shadow font from TAAG
{
  clearscreen();
  cout<<"\n\n"<< R"(
██╗   ██╗ ██████╗ ██╗   ██╗    ██╗    ██╗ ██████╗ ███╗   ██╗
╚██╗ ██╔╝██╔═══██╗██║   ██║    ██║    ██║██╔═══██╗████╗  ██║
 ╚████╔╝ ██║   ██║██║   ██║    ██║ █╗ ██║██║   ██║██╔██╗ ██║
  ╚██╔╝  ██║   ██║██║   ██║    ██║███╗██║██║   ██║██║╚██╗██║
   ██║   ╚██████╔╝╚██████╔╝    ╚███╔███╔╝╚██████╔╝██║ ╚████║
   ╚═╝    ╚═════╝  ╚═════╝      ╚══╝╚══╝  ╚═════╝ ╚═╝  ╚═══╝
)";
cout << "\n... as the mighty orb is yours to wield!\n\n";
constexpr char tabs[] = "\t\t\t\t\t";
cout << tabs<<"Treasure    : "<<treasure<<'\n';
cout << tabs<<"Steps       : "<<steps<<"\n\n";
cout << tabs<<"Dynamite    : "<<dynamite<<"\n\n";
cout << tabs<<"Total score : "<< int((treasure + 10*dynamite)*1000/(float)(steps+1))<<'\n';
getchar();
startgame();
}

void welcome(void) // ansi shadow font from TAAG
{
  cout << "\n\n"<<R"(
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

void move(int feature_bit, ivec2& from, ivec2 to)
{
    bit_clear( mapelem(from), feature_bit); // clear
    bit_set( mapelem(to), feature_bit); // set
    from = to;
    auto v = mapelem(to);
    if (bit_test(v, HERO) && bit_test(v, ENEMY))
        startgame();
    else if (bit_test(v, HERO) && bit_test(v, TREASURE))
    {
        ++treasure;
        bit_clear(mapelem(to), TREASURE);
    }
    else if (bit_test(v, HERO) && bit_test(v, ORB))
        win();
    else if (bit_test(v, HERO) && bit_test(v, EXIT))
        create_level(time(NULL));
}

int main(void) {
    welcome(); // show welcome screen
    setup_keys(); // setup movement, dynamite and restart game keys
    startgame(); 
    while (true) // main loop
    {
        display();
        int c = read_key();
        if (c == keys[5]) // restart key pressed?
          startgame();
        if(c == keys[4] && dynamite > 0) // dynamite key pressed?
          use_dynamite();
        else // might be a movement key then
        {
          ivec2 dir; // defaults to 0
          for(int i=0;i<4;++i) // set direction if correct key pressed
            if (c == keys[i])
                dir = nbo[i];
          auto q = (hero + dir + dims)%dims; // make new point
          if (mapelem(q) > 0) // if it's not a wall
          {
              move(HERO, hero, q);
              ++steps;
          }
        } 
        // Play all enemies
        for (auto& e : enemy_position_and_last_direction)
        {
            int i0 = rand() % 4; // pick random start dir
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
                    move(ENEMY, e.first, p);
                    e.second = iDir;
                    moved = true;
                    break;
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
    return EXIT_SUCCESS;
}