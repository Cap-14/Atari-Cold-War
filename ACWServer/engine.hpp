#pragma once
#ifndef ENGINE_H
#define ENGINE_H

#include <iostream>
#include <fstream>
#include <cstdint>
#include <cmath>
#include <thread>
#include <SFML/Network.hpp>

using namespace std;
using namespace sf;

short Distance(short x1, short x2, short y1, short y2);

extern bool active;

extern Packet USMessagePacket;
extern UdpSocket USender;
extern uint16_t UnitNumber;

class Client
{
    public:
        uint16_t port;
        UdpSocket Listener;
        IpAddress IPAddress;
        bool active=false, ready=false;
        string clientname;
        uint16_t Team, Side, Cash=300, NoFactories=0;
};

extern Client Players[2];

class TileTemplate
{
    public:
        uint16_t ID;
        bool Passable, Obstruction;
        TileTemplate(short TileID, bool TilePassability, bool TileBlocker);
};

extern vector<TileTemplate> Tiles;

class Map
{
    public:
        uint16_t Width, Height;
        string Name;
        TileTemplate* Tiles[200][200];
        bool Occupied[200][200];
        Vector2i Buildings[8]={{-1,-1},{-1,-1},{-1,-1},{-1,-1},{-1,-1},{-1,-1},{-1,-1},{-1,-1}};
        Map();
};

extern Map GameMap;

class Unit
{
    public:
        int16_t HP=100;
        uint16_t Damage, ATDamage, AllDamage, Speed, Range, ID, Cost, Number;
        uint16_t Type, Team=0, Owner=0;
        uint16_t posx,posy;
        bool destroy=false;

        //Unit(short hp, short dmg, short atdmg, short alldmg, short speed, short range, short type, short id, short team);
        Unit();
        Unit(short damage, short atDamage, short allDamage, short speed, short range, short type);

        void Attack(vector<Unit> *UnitsVector)
        {
            if(ID==1) for(short i=0;i<UnitsVector->size();i++)
            {
                if(UnitsVector->at(i).Team!=Team||UnitsVector->at(i).Number==Number) continue;
                if(Distance(posx,UnitsVector->at(i).posx,posy,UnitsVector->at(i).posy)<=Range&&UnitsVector->at(i).Type==0)
                if(UnitsVector->at(i).HP<100) 
                {
                    UnitsVector->at(i).HP-=Damage;
                    if(UnitsVector->at(i).HP>100) UnitsVector->at(i).HP=100;
                    USMessagePacket<<uint8_t('A')<<UnitsVector->at(i).Number<<UnitsVector->at(i).HP<<ID;
                    for(short i=0;i<2;i++) if (USender.send(USMessagePacket, Players[i].IPAddress, Players[i].port+1) == Socket::Done){}
                    USMessagePacket.clear();

                    break;
                }
            }
            else for(short i=0;i<UnitsVector->size();i++)
            {
                if(UnitsVector->at(i).Team==Team) continue;
                if(Distance(posx,UnitsVector->at(i).posx,posy,UnitsVector->at(i).posy)>Range) continue;
                if(Range>1)
                {
                    bool stop=false;
                    short x0 = posx, y0 = posy, x1 = UnitsVector->at(i).posx, y1 = UnitsVector->at(i).posy;
                    int dx = abs(x1 - x0), dy = abs(y1 - y0);
                    int x = x0, y = y0;
                    int n = 1 + dx + dy;
                    int x_inc = (x1 > x0) ? 1 : -1, y_inc = (y1 > y0) ? 1 : -1;
                    int error = dx - dy;
                    dx *= 2; dy *= 2;

                    for (; n > 0; --n)
                    {
                        if(GameMap.Tiles[y][x]->Obstruction) stop=true;
                        if (error > 0)
                        {
                            x += x_inc;
                            error -= dy;
                        }
                        else
                        {
                            y += y_inc;
                            error += dx;
                        }
                    }
                    if(stop) continue;
                }
                if(UnitsVector->at(i).Type==0) UnitsVector->at(i).HP-=Damage;
                else UnitsVector->at(i).HP-=ATDamage;
                UnitsVector->at(i).HP-=AllDamage;

                USMessagePacket<<uint8_t('A')<<UnitsVector->at(i).Number<<UnitsVector->at(i).HP<<ID;
                for(short i=0;i<2;i++) if (USender.send(USMessagePacket, Players[i].IPAddress, Players[i].port+1) == Socket::Done){}
                USMessagePacket.clear();

                if(UnitsVector->at(i).HP<=0) UnitsVector->erase(UnitsVector->begin()+i);

                break;
            }
        }
};

extern vector<Unit> UnitTemplates, Units;

class Ordnance : public Unit
{
    public:
        Vector2i targettile;
        vector<Vector2i> destination;

        Ordnance(Vector2i Target, short id, short allDamage, short speed, short range, short team);

        void Attack()
        {
            if(ID!=11)
            {
                for(short i=0;i<Units.size();i++)
                {
                    if(Units.at(i).Team==Team) continue;
                    if(Units.at(i).posx==targettile.x&&Units.at(i).posy==targettile.y&&targettile==Vector2i(posx,posy))
                    {
                        Units.at(i).HP-=AllDamage;

                        USMessagePacket<<uint8_t('A')<<Units.at(i).Number<<Units.at(i).HP<<ID;
                        for(short i=0;i<2;i++) if (USender.send(USMessagePacket, Players[i].IPAddress, Players[i].port+1) == Socket::Done){}
                        USMessagePacket.clear();

                        break;
                    }
                }
            }
            else if(!GameMap.Occupied[posy][posx])
            {
                uint16_t indx=Units.size();
                USMessagePacket << uint8_t('P') << Team << UnitNumber << posx << posy << ID;
                for(short i=0;i<2;i++) if (USender.send(USMessagePacket, Players[i].IPAddress, Players[i].port+1) == Socket::Done){}
                USMessagePacket.clear();

                GameMap.Occupied[posy][posx]=1;
                Units.push_back(UnitTemplates[0]);
                Units[indx].Team=Team;
                Units[indx].Number=UnitNumber;
                Units[indx].posx=posx;
                Units[indx].posy=posy;
                UnitNumber++;
            }
        }
        void Move()
        {
            posx=destination[0].x;
            posy=destination[0].y;
            if(posx==targettile.x&&posy==targettile.y) Attack();
            destination.erase(destination.begin());
            if(destination.size()==0) destroy=true;
        }
};

void Listen(UdpSocket* Listener, string IPAddress, unsigned short port, Client *Clients);
void ConnectionCheck(unsigned short port, Client *CheckedClient, UdpSocket *ThreadListener);
void ConnectionReceiver(UdpSocket *Connector, UdpSocket *Verificator, Client *Clients);
void Initialise();
void Tick();

#endif