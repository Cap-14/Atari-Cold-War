#pragma once
#ifndef ENGINE_H
#define ENGINE_H

#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <SFML/Network.hpp>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <thread>

#include "AStar.hpp"

using namespace std;
using namespace sf;

short Distance(short x1, short x2, short y1, short y2);

extern Packet MessagePacket;
extern string IPAddress;
extern UdpSocket Connector, Sender, Receiver;
extern unsigned short port;

class player
{
    public:
        uint16_t Team, Side;
        string NationName, Name;
};
extern player Player;

class Effect
{
    public:
        bool done=false;
        uint16_t posx, posy, ID, frame=0;
        string SoundName;
        SoundBuffer EffectBuffer;
        Sound EffectSound;

        Effect(short id, string sname);
        
        void NextFrame()
        {
            frame++;
            if (frame==4)
            done=true;
        }

        void Start(short x, short y)
        {
            posx=x; posy=y;
            EffectBuffer.loadFromFile("Audio/SFX/"+SoundName+".ogg");
            EffectSound.setBuffer(EffectBuffer);
            EffectSound.play();
        }
};

extern vector<Effect> EffectTemplates;
extern vector<Effect> Effects;

class TileTemplate
{
    public:
        uint16_t ID, Animate;
        bool Passable, Animated, Obstruction;
        TileTemplate(short TileID, bool TilePassability, bool TileAnimated, bool TileBlocker);
};

extern vector<TileTemplate> Tiles;

class Map
{
    public:
        uint16_t Width, Height;
        TileTemplate* Tiles[200][200];
        bool Occupied[200][200], MovementMap[200][200];
        string Name;
        Vector2i Buildings[8]={{-1,-1},{-1,-1},{-1,-1},{-1,-1},{-1,-1},{-1,-1},{-1,-1},{-1,-1}};
        Map();
};

extern Map GameMap;

class Unit
{
    public:
        int16_t HP=100;
        uint16_t Damage, ATDamage, AllDamage, Speed, Range, ID, Cost, EffectID;
        uint16_t Number;
        uint16_t Type, Team=0;
        uint16_t posx,posy,direction=0,movemeter=0;
        bool destroy=false, moving=false;
        vector<Vector2i> destination;

        //Unit(short hp, short dmg, short atdmg, short alldmg, short speed, short range, short type, short id, short team);
        Unit();
        Unit(short damage, short atDamage, short allDamage, short speed, short range, short type);
        void Move()
        {
            if(Type==1)
            {
                if(destination[0].x==posx)
                {
                    if(destination[0].y<posy) direction=0;
                    else direction=2;
                }
                else if(destination[0].y==posy)
                {
                    if(destination[0].x<posx) direction=3;
                    else direction=1;
                }
            }
            if(movemeter<100)
            {
                movemeter+=Speed*10;
                if(movemeter>100) movemeter=100;
                return;
            }
            if(GameMap.Occupied[destination[0].y][destination[0].x])
            {
                movemeter=0;
                return;
            }
            movemeter=0;
            GameMap.Occupied[posy][posx]=0;
            posx=destination[0].x;
            posy=destination[0].y;
            GameMap.Occupied[posy][posx]=1;

            //cout << Number << endl;
            MessagePacket.clear();
            MessagePacket << uint8_t('M') << Number << posx << posy;
            if (Sender.send(MessagePacket, IPAddress, port) != Socket::Done){}

            if(GameMap.Tiles[posy][posx]->ID==2||GameMap.Tiles[posy][posx]->ID==26+1-Team) GameMap.Tiles[posy][posx]=&Tiles[26+Team];

            destination.erase(destination.begin());
            bool oc=GameMap.MovementMap[destination[0].y][destination[0].x];
            if(Type==2) oc=1-oc;
            if(destination.size()==0)
            {
                destination.clear();
                moving=false;
            }
            else if(oc==1) NewDestination(destination[destination.size()-1].x,destination[destination.size()-1].y);
        }
        void NewDestination(short dx, short dy)
        {
            vector<Vector2i> NewDestinations;

            AStar::Generator generator;
            generator.setWorldSize({GameMap.Width, GameMap.Height});
            generator.setHeuristic(AStar::Heuristic::manhattan);
            generator.setDiagonalMovement(false);
            for(short i=0;i<GameMap.Height;i++)
            for(short j=0;j<GameMap.Width;j++)
            {
                if(j==posx&&i==posy)continue;
                if(GameMap.MovementMap[i][j]==1&&Type!=2) generator.addCollision({j,i});
                else if(GameMap.MovementMap[i][j]==0&&Type==2) generator.addCollision({j,i});
            }
            
            auto path = generator.findPath({dx, dy},{posx, posy});

            for(short i=1;i<path.size();i++) NewDestinations.push_back({path[i].x,path[i].y});
            
            if(destination.size()==0||NewDestinations[0]!=destination[0])movemeter=0;
            moving=true;
            destination.clear();
            copy(NewDestinations.begin(),NewDestinations.end(),back_inserter(destination));
        }
};

extern vector<Unit> UnitTemplates, Units;

class Ordnance : public Unit
{
    public:
        Vector2i targettile;

        Ordnance(Vector2i Target, short id, short allDamage, short speed, short range, short eif, short team);
        void Move()
        {
            if(destination[0].x<posx) direction=3;
            else direction=1;
            posx=destination[0].x;
            posy=destination[0].y;
            /*MessagePacket.clear();
            MessagePacket << "m" << Number << posx << posy;
            if (Sender.send(MessagePacket, IPAddress, port) != Socket::Done){}*/
            destination.erase(destination.begin());
            if(destination.size()==0) destroy=true;
        }
};

void Initialise();
void LeftClick();
void RightClick();
void Draw(RenderWindow &GameWindow);
void Input();
void Tick();
void RandomMusic();

/*void Send(UdpSocket* Sender, string IPAddress, unsigned short port);*/
void PingPong(string IPAddress, unsigned short port);
void Receive(UdpSocket* Receiver, string IPAddress, unsigned short port);

extern short Mul, mx, my;

#endif