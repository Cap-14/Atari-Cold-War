#include "engine.hpp"

uint16_t UnitNumber=0;
Map GameMap;
vector<TileTemplate> Tiles;
vector<Unit> UnitTemplates, Units;
vector<Ordnance> Ordnances;
Client Players[2];
Clock TickClock;
Time TickTime;
bool UnitTickedAttack=0, OrdnanceTicked=0, CashTicked=0, started=0, active=true;
string NationName="YUG";

Packet USMessagePacket;
UdpSocket USender;

Unit::Unit()
{
}

Unit::Unit(short damage, short atDamage, short allDamage, short speed, short range, short type)
{
    Damage = damage;
    ATDamage = atDamage;
    AllDamage = allDamage;
    Speed = speed;
    Range = range;
    Type = type;
}

Ordnance::Ordnance(Vector2i Target, short id, short allDamage, short speed, short range, short team)
{
    targettile=Target;
    ID = id;
    AllDamage = allDamage;
    Speed = speed;
    Range = range;
    posy=targettile.y;
    Team=team;
    if(Team==0)
    {
        posx=-1;
        for(short i=0;i<GameMap.Width+1;i++) destination.push_back(Vector2i(i,posy));
    }
    else
    {
        posx=GameMap.Width;
        for(short i=GameMap.Width-1;i>-2;i--) destination.push_back(Vector2i(i,posy));
    }
}

TileTemplate::TileTemplate(short TileID, bool TilePassability, bool TileBlocker)
{
    ID = TileID;
    Passable = TilePassability;
    Obstruction=TileBlocker;
}

Map::Map()
{
}

short Distance(short x1, short x2, short y1, short y2)
{
    return sqrt((x2-x1)*(x2-x1)+(y2-y1)*(y2-y1));
}

void Initialise()
{
    fstream TileFile("Data/Tilesets/Urban.txt");
    short numerator;
    TileFile >> numerator;
    short TileID,TilePassability,TileBlocker;
    for(short i=0;i<numerator;i++)
    {
        TileFile >> TileID >> TilePassability >> TileBlocker;
        Tiles.push_back(TileTemplate(TileID,TilePassability,TileBlocker));
    }
    TileFile.close();

    fstream MapFile("Data/Maps/"+GameMap.Name+".txt");
    MapFile>>GameMap.Width>>GameMap.Height;

    for(short i=0;i<GameMap.Height;i++)
    for(short j=0;j<GameMap.Width;j++)
    {
        short TileReference;
        MapFile >> TileReference;
        GameMap.Tiles[i][j]=&Tiles[TileReference-1];
        if(GameMap.Tiles[i][j]->Passable) GameMap.Occupied[i][j]=0;
        else GameMap.Occupied[i][j]=1;

        if(TileReference-1==18) GameMap.Buildings[0]={j,i};
        else if(TileReference-1==19) GameMap.Buildings[1]={j,i};
        else if(TileReference-1==20) GameMap.Buildings[2]={j,i};
        else if(TileReference-1==21) GameMap.Buildings[3]={j,i};
        else if(TileReference-1==22) GameMap.Buildings[4]={j,i};
        else if(TileReference-1==23) GameMap.Buildings[5]={j,i};
        else if(TileReference-1==24) GameMap.Buildings[6]={j,i};
        else if(TileReference-1==25) GameMap.Buildings[7]={j,i};
    }
    MapFile.close();

    fstream UnitFile("Data/Units/Base.txt");
    UnitFile>>numerator;
    for(short i=0;i<numerator;i++)
    {
        Unit TemplateUnit;
        UnitFile >> TemplateUnit.Damage >> TemplateUnit.ATDamage >> TemplateUnit.AllDamage >> TemplateUnit.Speed >> TemplateUnit.Range >> TemplateUnit.Type >> TemplateUnit.Cost;
        TemplateUnit.ID = i;
        UnitTemplates.push_back(TemplateUnit);
    }
    UnitFile.close();
}

void Tick()
{
    TickTime = TickClock.getElapsedTime();
    float Ticks = TickTime.asSeconds();

    if(Ticks>=61)
    {
        TickClock.restart();
        Ticks-=60;
    }

    if(int(Ticks*10)%2==0&&int(Ticks*10)!=0)
    {
        if(!OrdnanceTicked)
        {
            OrdnanceTicked=1;
            for(short i=0;i<Ordnances.size();i++)
            {
                if(Ordnances[i].destroy) Ordnances.erase(Ordnances.begin()+i);
                Ordnances[i].Move();
            }
        }
    }
    else
    {
        OrdnanceTicked=0;
    }

    if(int(Ticks*10)%15==0&&int(Ticks*10)!=0)
    {
        if(!UnitTickedAttack)
        {
            UnitTickedAttack=1;
            for(short i=0;i<Units.size();i++) if(Units[i].HP>0) Units[i].Attack(&Units);
        }
    }
    else
    {
        UnitTickedAttack=0;
    }

    if(int(Ticks*10)%100==0&&int(Ticks*10)!=0)
    {
        if(!CashTicked)
        {
            Players[0].Cash+=Players[0].NoFactories*20;
            Players[1].Cash+=Players[1].NoFactories*20;

            for(short i=0;i<2;i++)
            {
                USMessagePacket << uint8_t('C') << Players[i].Cash;
                if (USender.send(USMessagePacket, Players[i].IPAddress, Players[i].port+1) == Socket::Done){}
                USMessagePacket.clear();
            }
            CashTicked=true;
        }
    }
    else
    {
        CashTicked=0;
    }
}

void ConnectionReceiver(UdpSocket *Connector, UdpSocket *Verificator, Client *Clients)
{
    Packet NBTPacket;
    IpAddress ReceivedIP;
    unsigned short ReceivedPort, side;
    string username;
    bool run = true;

    while(run)
    {
        if(Connector->receive(NBTPacket,ReceivedIP,ReceivedPort)==Socket::Done)
        NBTPacket >> username >> side;

        NBTPacket.clear();

        for(short i=0;i<2;i++)
        if(!Clients[i].active)
        {
            Clients[i].active=true;
            Clients[i].IPAddress=ReceivedIP;
            Clients[i].clientname = username;
            Clients[i].Side = side;
            Clients[i].Team = side/3;

            cout << "Novi igrac iz " + ReceivedIP.toString() << endl;

            if(i==0)
            {
                NBTPacket << GameMap.Name << uint8_t('y') << Clients[0].port;
                if (Verificator->send(NBTPacket, Clients[0].IPAddress, 54999) != Socket::Done){}
            }
            else
            {
                NBTPacket << GameMap.Name << uint8_t('Y') << Clients[1].port;
                if (Verificator->send(NBTPacket, Clients[1].IPAddress, 54999) != Socket::Done){}
                NBTPacket.clear();
                NBTPacket << Clients[0].port;
                if (Verificator->send(NBTPacket, Clients[0].IPAddress, 55000) != Socket::Done){}
                started=true;
                run=false;

                thread Tredi(Listen,&Clients[0].Listener,ReceivedIP.toString(),Clients[0].port,Clients); Tredi.detach();
                thread TrediJ(ConnectionCheck,Clients[0].port,&Clients[0],&Clients[0].Listener); TrediJ.detach();

                thread Tredi2(Listen,&Clients[1].Listener,ReceivedIP.toString(),Clients[1].port,Clients); Tredi2.detach();
                thread TrediJ2(ConnectionCheck,Clients[1].port,&Clients[1],&Clients[1].Listener); TrediJ2.detach();

                cout << endl;
            }
            break;
        }
    }
}

void Listen(UdpSocket* Listener, string IPAddress, unsigned short port, Client *Clients)
{
    Packet MessagePacket, SMessagePacket;
    IpAddress SenderIP;
    string Message, Name;
    UdpSocket Sender;
    unsigned short clientnum;

    for(short i=0;i<2;i++)
    if(port==Clients[i].port)
    {
        clientnum = i;
        Name = Clients[i].clientname;
        break;
    }

    while(Clients[clientnum].active)
    {
        if (Listener->receive(MessagePacket, SenderIP, port) == Socket::Done)
        {
            uint8_t Message;
            MessagePacket >> Message;
            if(Message=='N')
            {
                uint16_t UType, indx=Units.size();
                MessagePacket >> UType;
                if(Players[clientnum].Cash<UnitTemplates[UType].Cost) goto ending;
                Players[clientnum].Cash-=UnitTemplates[UType].Cost;
                SMessagePacket << Message << UType << Players[clientnum].Team << UnitNumber;
                for(short i=0;i<2;i++) if (Sender.send(SMessagePacket, Clients[i].IPAddress, Clients[i].port+1) == Socket::Done){}
                Units.push_back(UnitTemplates[UType]);
                short bx = GameMap.Buildings[Players[clientnum].Team*4+UnitTemplates[UType].Type].x, by = GameMap.Buildings[Players[clientnum].Team*4+UnitTemplates[UType].Type].y;
                Units[indx].posx=bx;
                Units[indx].posy=by;
                Units[indx].Team=Players[clientnum].Team;
                Units[indx].Number=UnitNumber;
                UnitNumber++;
                SMessagePacket.clear();
            }
            else if(Message=='M')
            {
                uint16_t UNumber,UPosx,UPosy;
                MessagePacket >> UNumber >> UPosx >> UPosy;
                cout << UNumber << endl;
                SMessagePacket << Message << UNumber << UPosx << UPosy;
                if (Sender.send(SMessagePacket, Clients[1-clientnum].IPAddress, Clients[1-clientnum].port+1) == Socket::Done){}
                SMessagePacket.clear();
                for(short i=0;i<Units.size();i++)
                if(Units[i].Number==UNumber)
                {
                    Units[i].posx=UPosx;
                    Units[i].posy=UPosy;
                    if(GameMap.Tiles[UPosy][UPosx]->ID==2)
                    {
                        GameMap.Tiles[UPosy][UPosx]=&Tiles[26+Players[clientnum].Team];
                        Players[clientnum].NoFactories++;
                    }
                    else if(GameMap.Tiles[UPosy][UPosx]->ID==26+1-Players[clientnum].Team)
                    {
                        GameMap.Tiles[UPosy][UPosx]=&Tiles[26+Players[clientnum].Team];
                        Players[clientnum].NoFactories++;
                        Players[1-clientnum].NoFactories--;
                    }
                    break;
                }
            }
            /*else if(Message=='m')
            {
                uint16_t UNumber,UPosx,UPosy;
                MessagePacket >> UNumber >> UPosx >> UPosy;
                SMessagePacket << Message << UNumber << UPosx << UPosy;
                if (Sender.send(SMessagePacket, Clients[1-clientnum].IPAddress, Clients[1-clientnum].port+1) == Socket::Done){}
                SMessagePacket.clear();
                for(short i=0;i<Ordnances.size();i++)
                if(Ordnances[i].Number==UNumber)
                {
                    Ordnances[i].posx=UPosx;
                    Ordnances[i].posy=UPosy;
                    goto ending;
                }
            }*/
            else if(Message=='a')
            {
                uint16_t UID,UTX,UTY,UTeam;
                MessagePacket >> UID >> UTX >> UTY >> UTeam;
                if(Players[clientnum].Cash<UnitTemplates[UID].Cost) goto ending;
                Players[clientnum].Cash-=UnitTemplates[UID].Cost;
                SMessagePacket << Message << UID << UTX << UTY << UTeam << UnitNumber;
                for(short i=0;i<2;i++) if (Sender.send(SMessagePacket, Clients[i].IPAddress, Clients[i].port+1) == Socket::Done){}
                Ordnances.push_back(Ordnance(Vector2i(UTX,UTY),UID,UnitTemplates[UID].AllDamage,0,0,Players[clientnum].Team));
                Ordnances[Ordnances.size()-1].Number=UnitNumber;
                UnitNumber++;
                SMessagePacket.clear();
            }
            else if(Message=='R') Clients[clientnum].ready=true;
            ending:
            MessagePacket.clear();
            SMessagePacket.clear();
        }
    }
    cout << "Igrac ("+Name+") sa " + IPAddress + " se diskonektovao" << endl;
}

void ConnectionCheck(unsigned short port, Client *CheckedClient, UdpSocket *ThreadListener)
{
    UdpSocket Checker;
    Packet CheckPacket;
    IpAddress IPA;
    port+=2;
    Checker.setBlocking(false);
    Checker.bind(port);

    Clock ClockChecker;
    Time TimeChecker;

    while(CheckedClient->active)
    {
        if (Checker.receive(CheckPacket,IPA,port) == Socket::Done)
        {
            CheckPacket.clear();
            ClockChecker.restart();
        }
        else if (TimeChecker.asMilliseconds()>2000)
        {
            CheckedClient->active=false;
        }
        TimeChecker=ClockChecker.getElapsedTime();
    }
    active=false;
}