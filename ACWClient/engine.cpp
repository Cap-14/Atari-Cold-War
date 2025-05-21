#include "engine.hpp"

short Frame=0, Mul, mx, my, CameraX=0, CameraY=0;
Map GameMap;
Packet MessagePacket;
vector<TileTemplate> Tiles;
vector<Unit> UnitTemplates, Units;
vector<Ordnance> Ordnances;
vector<Effect> EffectTemplates;
vector<Effect> Effects;
Unit *SelectedUnit;
Texture Tileset, UnitTexture, UITexture, EffectsTexture;
Clock TickClock;
Time TickTime;
player Player;
uint16_t Cash=300, SelectedOrdnance=0;
bool isAnimated=0, UnitTickedMove=0, OrdnanceTicked=0, targeting=0, nomusic=false;
short enddx=20, enddy=20;
UdpSocket Connector, Sender, Receiver;
Music GameMusic;

string IPAddress;
unsigned short port;

/*Unit::Unit(short hp, short dmg, short atdmg, short alldmg, short speed, short range, short type, short id, short team)
{
    HP=hp;
    Damage=dmg;
    ATDamage=atdmg;
    AllDamage=alldmg;
    Speed=speed;
    Range=range;
    Type=type;
    ID=id;
    Team=team;
}*/

Effect::Effect(short id, string sname)
{
    ID=id;
    SoundName=sname;
};

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

Ordnance::Ordnance(Vector2i Target, short id, short allDamage, short speed, short range, short eif, short team)
{
    targettile=Target;
    ID = id;
    AllDamage = allDamage;
    Speed = speed;
    Range = range;
    posy=targettile.y;
    EffectID=eif;
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

TileTemplate::TileTemplate(short TileID, bool TilePassability, bool TileAnimated, bool TileBlocker)
{
    ID = TileID;
    Passable = TilePassability;
    Animated = TileAnimated;
    Animate=0;
    Obstruction=TileBlocker;
}

Map::Map()
{
}

short Distance(short x1, short x2, short y1, short y2)
{
    return sqrt((x2-x1)*(x2-x1)+(y2-y1)*(y2-y1));
}

void RandomMusic()
{
    fstream MusicFile("Data/Music/SongList.txt");
    string holder = "";
    while(holder!=Player.NationName)
    {
        if(holder=="end") {nomusic=1;return;}
        MusicFile >> holder; 
    }
    short numerator, randnum;
    MusicFile >> numerator;
    if(numerator==0){nomusic=1;return;}
    randnum = rand()%numerator;
    for(short i=0;i<randnum;i++) MusicFile >> holder;
    MusicFile >> holder;
    GameMusic.openFromFile("Audio/Music/"+holder+".ogg");
    GameMusic.play();
}

void Initialise()
{
    Units.reserve(40);
    srand(time(NULL));
    Tileset.loadFromFile("Graphics/Urban.png");
    UnitTexture.loadFromFile("Graphics/Spritesheet.png");
    UITexture.loadFromFile("Graphics/UI.png");
    EffectsTexture.loadFromFile("Graphics/Effects.png");
    fstream TileFile("Data/Tilesets/Urban.txt");
    short numerator;
    TileFile >> numerator;
    short TileID,TilePassability,TileAnimated,TileBlocker;
    for(short i=0;i<numerator;i++)
    {
        TileFile >> TileID >> TilePassability >> TileAnimated >> TileBlocker;
        Tiles.push_back(TileTemplate(TileID,TilePassability,TileAnimated,TileBlocker));
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
        if(GameMap.Tiles[i][j]->Passable) {GameMap.Occupied[i][j]=0; GameMap.MovementMap[i][j]=0;}
        else {GameMap.Occupied[i][j]=1; GameMap.MovementMap[i][j]=1;}

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
        UnitFile >> TemplateUnit.Damage >> TemplateUnit.ATDamage >> TemplateUnit.AllDamage >> TemplateUnit.Speed >> TemplateUnit.Range >> TemplateUnit.Type >> TemplateUnit.Cost >> TemplateUnit.EffectID;
        TemplateUnit.ID = i;
        UnitTemplates.push_back(TemplateUnit);
    }
    UnitFile.close();

    fstream EffectFile("Data/Effects/Base.txt");
    EffectFile>>numerator;
    for(short i=0;i<numerator;i++)
    {
        string estr;
        EffectFile >> estr;
        EffectTemplates.push_back(Effect(i,estr));
    }
    EffectFile.close();

    short utx=20, uty=6, prevtype=0;
    for(short i=0;i<UnitTemplates.size();i++)
    {
        if(prevtype!=UnitTemplates[i].Type)
        {
            utx=20;
            uty++;
            prevtype++;
        }
        UnitTemplates[i].posx=utx;
        UnitTemplates[i].posy=uty;
        utx++;
        if(utx==25)
        {
            utx=20;
            uty++;
        }
    }
    
    GameMusic.setVolume(50);
    RandomMusic();
    TickClock.restart();
}

void LeftClick()
{
    uint16_t mxd=mx/Mul, myd=my/Mul;
    if(mxd>=20)
    {
        if (targeting) return;
        for(short i=0;i<UnitTemplates.size();i++)
        if(UnitTemplates[i].posx==mxd&&UnitTemplates[i].posy==myd)
        {
            if(UnitTemplates[i].Type!=3)
            {
                short bx = GameMap.Buildings[Player.Team*4+UnitTemplates[i].Type].x, by = GameMap.Buildings[Player.Team*4+UnitTemplates[i].Type].y;
                if(UnitTemplates[i].Cost>Cash||bx==-1||GameMap.Occupied[by][bx]) return;
                MessagePacket.clear();
                MessagePacket << uint8_t('N') << uint16_t(i);
                if (Sender.send(MessagePacket, IPAddress, port) != Socket::Done){}
                return;
            }
            else
            {
                if(UnitTemplates[i].Cost>Cash) return;
                SelectedUnit = nullptr;
                targeting=true;
                SelectedOrdnance=i;
            }
            return;
        }
    }
    else if(targeting==true)
    {
        if(mxd>=GameMap.Width) return;
        mxd=(mx+CameraX)/Mul; myd=(my+CameraY)/Mul;
        short i = SelectedOrdnance;
        Cash-=UnitTemplates[i].Cost;

        MessagePacket.clear();
        MessagePacket << uint8_t('a') << UnitTemplates[i].ID << mxd << myd << Player.Team;
        if (Sender.send(MessagePacket, IPAddress, port) != Socket::Done){}
        
        targeting=0;
    }
    else
    {
        mxd=(mx+CameraX)/Mul; myd=(my+CameraY)/Mul;
        SelectedUnit = nullptr;
        for(short i=0;i<Units.size();i++)
        if(Units[i].posx==mxd&&Units[i].posy==myd&&Units[i].Team==Player.Team&&Units[i].Type!=3)
        {
            SelectedUnit = &Units[i];
            break;
        }
    }
}

void RightClick()
{
    targeting=0;
    if(SelectedUnit==nullptr) return;
    short mxd=(mx+CameraX)/Mul, myd=(my+CameraY)/Mul, oc=GameMap.MovementMap[myd][mxd];
    if(SelectedUnit->Type==2) oc=1-oc;
    if(oc==1||mxd>GameMap.Width-1||myd>GameMap.Height-1) return;
    SelectedUnit->NewDestination(mxd,myd);
}

void Draw(RenderWindow &GameWindow)
{
    VertexArray Verteces(Triangles), DestinationVerteces(Lines);
    for(short i=CameraY/Mul;i<enddy;i++)
    for(short j=CameraX/Mul;j<enddx;j++)
    {
        short tilenum = GameMap.Tiles[i][j]->ID, animate = GameMap.Tiles[i][j]->Animate;
        Verteces.append(Vertex(Vector2f(j*Mul    -CameraX,i*Mul    -CameraY),Vector2f(tilenum*20   ,animate*20   )));
        Verteces.append(Vertex(Vector2f(j*Mul+Mul-CameraX,i*Mul    -CameraY),Vector2f(tilenum*20+20,animate*20   )));
        Verteces.append(Vertex(Vector2f(j*Mul+Mul-CameraX,i*Mul+Mul-CameraY),Vector2f(tilenum*20+20,animate*20+20)));
        Verteces.append(Vertex(Vector2f(j*Mul    -CameraX,i*Mul    -CameraY),Vector2f(tilenum*20   ,animate*20    )));
        Verteces.append(Vertex(Vector2f(j*Mul+Mul-CameraX,i*Mul+Mul-CameraY),Vector2f(tilenum*20+20,animate*20+20)));
        Verteces.append(Vertex(Vector2f(j*Mul    -CameraX,i*Mul+Mul-CameraY),Vector2f(tilenum*20   ,animate*20+20)));
    }

    GameWindow.draw(Verteces,&Tileset);

    Verteces.clear();
    RectangleShape HealthBars[Units.size()], MoveBars[Units.size()+Ordnances.size()];
    for(short i=0;i<Units.size();i++)
    {
        short x = Units[i].posx, y = Units[i].posy, sx = Units[i].ID, sy = 0;
        if(Units[i].Type!=1) sy=Frame;
        else sy = Units[i].direction;
        sy+=Units[i].Team*4;
        Verteces.append(Vertex(Vector2f(x*Mul    -CameraX,y*Mul    -CameraY),Vector2f(sx*20   ,sy*20   )));
        Verteces.append(Vertex(Vector2f(x*Mul+Mul-CameraX,y*Mul    -CameraY),Vector2f(sx*20+20,sy*20   )));
        Verteces.append(Vertex(Vector2f(x*Mul+Mul-CameraX,y*Mul+Mul-CameraY),Vector2f(sx*20+20,sy*20+20)));
        Verteces.append(Vertex(Vector2f(x*Mul    -CameraX,y*Mul    -CameraY),Vector2f(sx*20   ,sy*20   )));
        Verteces.append(Vertex(Vector2f(x*Mul+Mul-CameraX,y*Mul+Mul-CameraY),Vector2f(sx*20+20,sy*20+20)));
        Verteces.append(Vertex(Vector2f(x*Mul    -CameraX,y*Mul+Mul-CameraY),Vector2f(sx*20   ,sy*20+20)));

        short hp = (Units[i].HP/5)*Mul/20; if(hp<0) hp=0;
        HealthBars[i].setFillColor(Color::Green);
        HealthBars[i].setPosition(x*Mul-CameraX,y*Mul-CameraY+(19*Mul/20));
        HealthBars[i].setSize(Vector2f(hp,1*(Mul/20)));

        short mm = (Units[i].movemeter/5)*Mul/20;
        MoveBars[i].setFillColor(Color::Red);
        MoveBars[i].setPosition(x*Mul-CameraX,y*Mul-CameraY+(18*Mul/20));
        MoveBars[i].setSize(Vector2f(mm,1*(Mul/20)));
    }
    for(short i=0;i<Ordnances.size();i++)
    {
        short x = Ordnances[i].posx, y = Ordnances[i].posy, sx = Ordnances[i].ID, sy = sy = Ordnances[i].direction;
        sy+=Ordnances[i].Team*4;
        Verteces.append(Vertex(Vector2f(x*Mul    -CameraX,y*Mul    -CameraY),Vector2f(sx*20   ,sy*20   )));
        Verteces.append(Vertex(Vector2f(x*Mul+Mul-CameraX,y*Mul    -CameraY),Vector2f(sx*20+20,sy*20   )));
        Verteces.append(Vertex(Vector2f(x*Mul+Mul-CameraX,y*Mul+Mul-CameraY),Vector2f(sx*20+20,sy*20+20)));
        Verteces.append(Vertex(Vector2f(x*Mul    -CameraX,y*Mul    -CameraY),Vector2f(sx*20   ,sy*20   )));
        Verteces.append(Vertex(Vector2f(x*Mul+Mul-CameraX,y*Mul+Mul-CameraY),Vector2f(sx*20+20,sy*20+20)));
        Verteces.append(Vertex(Vector2f(x*Mul    -CameraX,y*Mul+Mul-CameraY),Vector2f(sx*20   ,sy*20+20)));

        short j=i+Units.size();
        short mm = (Ordnances[i].movemeter/5)*Mul/20;
        MoveBars[j].setFillColor(Color::Red);
        MoveBars[j].setPosition(x*Mul-CameraX,y*Mul-CameraY+(18*Mul/20));
        MoveBars[j].setSize(Vector2f(mm,1*(Mul/20)));
    }

    GameWindow.draw(Verteces,&UnitTexture);
    for(short i=0;i<Units.size();i++)
    {
        GameWindow.draw(HealthBars[i]);
        GameWindow.draw(MoveBars[i]);
    }

    if(SelectedUnit!=nullptr&&SelectedUnit->moving==true)
    {
        short x = SelectedUnit->posx, y = SelectedUnit->posy;
        short dx = SelectedUnit->destination[SelectedUnit->destination.size()-1].x, dy = SelectedUnit->destination[SelectedUnit->destination.size()-1].y;
        DestinationVerteces.clear();
        DestinationVerteces.append(Vertex(Vector2f(x*Mul -CameraX+10*Mul/20,y*Mul -CameraY+10*Mul/20),Color::Yellow));
        DestinationVerteces.append(Vertex(Vector2f(dx*Mul-CameraX+10*Mul/20,dy*Mul-CameraY+10*Mul/20),Color::Yellow));
        GameWindow.draw(DestinationVerteces);
    }

    Verteces.clear();

    for(short i=0;i<Effects.size();i++)
    {
        short x = Effects[i].posx, y = Effects[i].posy, sx = Effects[i].ID, sy=Effects[i].frame;
        Verteces.append(Vertex(Vector2f(x*Mul    -CameraX,y*Mul    -CameraY),Vector2f(sx*20   ,sy*20   )));
        Verteces.append(Vertex(Vector2f(x*Mul+Mul-CameraX,y*Mul    -CameraY),Vector2f(sx*20+20,sy*20   )));
        Verteces.append(Vertex(Vector2f(x*Mul+Mul-CameraX,y*Mul+Mul-CameraY),Vector2f(sx*20+20,sy*20+20)));
        Verteces.append(Vertex(Vector2f(x*Mul    -CameraX,y*Mul    -CameraY),Vector2f(sx*20   ,sy*20   )));
        Verteces.append(Vertex(Vector2f(x*Mul+Mul-CameraX,y*Mul+Mul-CameraY),Vector2f(sx*20+20,sy*20+20)));
        Verteces.append(Vertex(Vector2f(x*Mul    -CameraX,y*Mul+Mul-CameraY),Vector2f(sx*20   ,sy*20+20)));
    }
    GameWindow.draw(Verteces,&EffectsTexture);
    Verteces.clear();

    if(SelectedUnit!=nullptr)
    {
        short x = SelectedUnit->posx, y = SelectedUnit->posy;
        Verteces.append(Vertex(Vector2f(x*Mul    -CameraX,y*Mul    -CameraY),Vector2f(60,40)));
        Verteces.append(Vertex(Vector2f(x*Mul+Mul-CameraX,y*Mul    -CameraY),Vector2f(80,40)));
        Verteces.append(Vertex(Vector2f(x*Mul+Mul-CameraX,y*Mul+Mul-CameraY),Vector2f(80,60)));
        Verteces.append(Vertex(Vector2f(x*Mul    -CameraX,y*Mul    -CameraY),Vector2f(60,40)));
        Verteces.append(Vertex(Vector2f(x*Mul+Mul-CameraX,y*Mul+Mul-CameraY),Vector2f(80,60)));
        Verteces.append(Vertex(Vector2f(x*Mul    -CameraX,y*Mul+Mul-CameraY),Vector2f(60,60)));
    }

    if(targeting)
    {
        short mxd=(mx+CameraX)/Mul, myd=(my+CameraY)/Mul;
        Verteces.append(Vertex(Vector2f(mxd*Mul-CameraX    ,myd*Mul-CameraY    ),Vector2f(40,40)));
        Verteces.append(Vertex(Vector2f(mxd*Mul-CameraX+Mul,myd*Mul-CameraY    ),Vector2f(60,40)));
        Verteces.append(Vertex(Vector2f(mxd*Mul-CameraX+Mul,myd*Mul-CameraY+Mul),Vector2f(60,60)));
        Verteces.append(Vertex(Vector2f(mxd*Mul-CameraX    ,myd*Mul-CameraY    ),Vector2f(40,40)));
        Verteces.append(Vertex(Vector2f(mxd*Mul-CameraX+Mul,myd*Mul-CameraY+Mul),Vector2f(60,60)));
        Verteces.append(Vertex(Vector2f(mxd*Mul-CameraX    ,myd*Mul-CameraY+Mul),Vector2f(40,60)));
    }

    Verteces.append(Vertex(Vector2f(20*Mul      ,0       ),Vector2f(41,17)));
    Verteces.append(Vertex(Vector2f(20*Mul+5*Mul,0       ),Vector2f(41,17)));
    Verteces.append(Vertex(Vector2f(20*Mul+5*Mul,0+20*Mul),Vector2f(41,17)));
    Verteces.append(Vertex(Vector2f(20*Mul      ,0       ),Vector2f(41,17)));
    Verteces.append(Vertex(Vector2f(20*Mul+5*Mul,0+20*Mul),Vector2f(41,17)));
    Verteces.append(Vertex(Vector2f(20*Mul      ,0+20*Mul),Vector2f(41,17)));

    short fx=Player.Team, fy=Player.Side%3;
    Verteces.append(Vertex(Vector2f(20*Mul      ,0    ),Vector2f(fx*20   ,fy*20   )));
    Verteces.append(Vertex(Vector2f(20*Mul+Mul*5,0    ),Vector2f(fx*20+20,fy*20   )));
    Verteces.append(Vertex(Vector2f(20*Mul+Mul*5,Mul*5),Vector2f(fx*20+20,fy*20+20)));
    Verteces.append(Vertex(Vector2f(20*Mul      ,0    ),Vector2f(fx*20   ,fy*20   )));
    Verteces.append(Vertex(Vector2f(20*Mul+Mul*5,Mul*5),Vector2f(fx*20+20,fy*20+20)));
    Verteces.append(Vertex(Vector2f(20*Mul      ,Mul*5),Vector2f(fx*20   ,fy*20+20)));

    short CashDigits=Cash, CashNumbers[4];
    CashNumbers[0]=CashDigits/1000; CashDigits%=1000;
    CashNumbers[1]=CashDigits/100; CashDigits%=100;
    CashNumbers[2]=CashDigits/10; CashDigits%=10;
    CashNumbers[3]=CashDigits;

    Verteces.append(Vertex(Vector2f(20*Mul         ,Mul*5         ),Vector2f(40,0)));
    Verteces.append(Vertex(Vector2f(20*Mul+5*Mul/20,Mul*5         ),Vector2f(45,0)));
    Verteces.append(Vertex(Vector2f(20*Mul+5*Mul/20,Mul*5+7*Mul/20),Vector2f(45,7)));
    Verteces.append(Vertex(Vector2f(20*Mul         ,Mul*5         ),Vector2f(40,0)));
    Verteces.append(Vertex(Vector2f(20*Mul+5*Mul/20,Mul*5+7*Mul/20),Vector2f(45,7)));
    Verteces.append(Vertex(Vector2f(20*Mul         ,Mul*5+7*Mul/20),Vector2f(40,7)));

    for(short i=0;i<4;i++)
    {
        short pdx=i*(5*Mul/20),dx = 45+CashNumbers[i]*5;
        Verteces.append(Vertex(Vector2f(20*Mul+5*Mul/20 +pdx,Mul*5         ),Vector2f(dx  ,0)));
        Verteces.append(Vertex(Vector2f(20*Mul+10*Mul/20+pdx,Mul*5         ),Vector2f(dx+5,0)));
        Verteces.append(Vertex(Vector2f(20*Mul+10*Mul/20+pdx,Mul*5+7*Mul/20),Vector2f(dx+5,7)));
        Verteces.append(Vertex(Vector2f(20*Mul+5*Mul/20 +pdx,Mul*5         ),Vector2f(dx  ,0)));
        Verteces.append(Vertex(Vector2f(20*Mul+10*Mul/20+pdx,Mul*5+7*Mul/20),Vector2f(dx+5,7)));
        Verteces.append(Vertex(Vector2f(20*Mul+5*Mul/20 +pdx,Mul*5+7*Mul/20),Vector2f(dx  ,7)));
    }
    
    VertexArray UTs(Triangles);
    for(short i=0;i<UnitTemplates.size();i++)
    {
        short x = UnitTemplates[i].posx, y = UnitTemplates[i].posy, sx = UnitTemplates[i].ID, sy = Player.Team*4;
        short c = 127+127*(UnitTemplates[i].Cost<=Cash&&GameMap.Buildings[Player.Team*4+UnitTemplates[i].Type].x!=-1);
        UTs.append(Vertex(Vector2f(x*Mul    ,y*Mul    ),Color(c,c,c,255),Vector2f(sx*20   ,sy*20   )));
        UTs.append(Vertex(Vector2f(x*Mul+Mul,y*Mul    ),Color(c,c,c,255),Vector2f(sx*20+20,sy*20   )));
        UTs.append(Vertex(Vector2f(x*Mul+Mul,y*Mul+Mul),Color(c,c,c,255),Vector2f(sx*20+20,sy*20+20)));
        UTs.append(Vertex(Vector2f(x*Mul    ,y*Mul    ),Color(c,c,c,255),Vector2f(sx*20   ,sy*20   )));
        UTs.append(Vertex(Vector2f(x*Mul+Mul,y*Mul+Mul),Color(c,c,c,255),Vector2f(sx*20+20,sy*20+20)));
        UTs.append(Vertex(Vector2f(x*Mul    ,y*Mul+Mul),Color(c,c,c,255),Vector2f(sx*20   ,sy*20+20)));
    }

    GameWindow.draw(Verteces,&UITexture);
    GameWindow.draw(UTs,&UnitTexture);
}

void Input()
{
    short oldcamx=CameraX, oldcamy=CameraY;
    if(Keyboard::isKeyPressed(Keyboard::A)) CameraX-=10;
    else if(Keyboard::isKeyPressed(Keyboard::D)) CameraX+=10;
    if(Keyboard::isKeyPressed(Keyboard::W)) CameraY-=10;
    else if(Keyboard::isKeyPressed(Keyboard::S)) CameraY+=10;

    if(oldcamx!=CameraX)
    {
        if (CameraX<0) CameraX=0;
        else if (CameraX>Mul*GameMap.Width-Mul*20) CameraX=Mul*GameMap.Width-Mul*20;
        enddx = (CameraX+Mul)/Mul+20; if(enddx>GameMap.Width) enddx=GameMap.Width;
    }
    if(oldcamy!=CameraY)
    {
        if (CameraY<0) CameraY=0;
        else if (CameraY>Mul*GameMap.Height-Mul*20) CameraY=Mul*GameMap.Height-Mul*20;
        enddy = (CameraY+Mul)/Mul+20; if(enddy>GameMap.Height) enddy=GameMap.Height;
    }
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

    if(GameMusic.getStatus()!=2&&!nomusic) RandomMusic();

    for(short i=0;i<Effects.size();i++)
    {
        if(Effects[i].done) Effects.erase(Effects.begin()+i);
    }

    if(int(Ticks*10)%3==0&&int(Ticks*10)!=0)
    {
        if(!isAnimated)
        {
            isAnimated=1;
            for(short i=0;i<Tiles.size();i++)
            if(Tiles[i].Animated)
            {
                Tiles[i].Animate++;
                Frame++;
                if(Tiles[i].Animate>3) Tiles[i].Animate=0;
                if(Frame>3) Frame=0;
            }
            for(short i=0;i<Effects.size();i++)
            Effects[i].NextFrame();
        }
    }
    else
    {
        isAnimated=0;
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

    if(int(Ticks*10)%5==0&&int(Ticks*10)!=0)
    {
        if(!UnitTickedMove)
        {
            UnitTickedMove=1;
            for(short i=0;i<Units.size();i++) if(Units[i].moving) Units[i].Move();
        }
    }
    else
    {
        UnitTickedMove=0;
    }
    //cout << int(Ticks*10) << " " << int(Ticks*10)%2 << " " << isAnimated << endl;
}



/*void Send(UdpSocket* Sender, string IPAddress, unsigned short port)
{ 
    Packet MessagePacket;
    string Message;
    while(true)
    {
        getline(cin, Message);
        if(Message!="")
        {
            MessagePacket << Message;
            if (Sender->send(MessagePacket, IPAddress, port) != Socket::Done){}
            MessagePacket.clear();
        }
    }
}*/

void PingPong(string IPAddress, unsigned short cport)
{
    UdpSocket Sender;
    Packet MessagePacket;
    cport+=2;

    Clock PingPongClock;
    Time PingPongTime;

    while(true)
    {
        PingPongTime = PingPongClock.getElapsedTime();
        if(PingPongTime.asMilliseconds()>=80)
        {
            PingPongClock.restart();
            MessagePacket << 1;
            if (Sender.send(MessagePacket, IPAddress, cport) != Socket::Done){}
            MessagePacket.clear();
        }
    }
}

void Receive(UdpSocket* Receiver, string IPAddress, unsigned short cport)
{
    Receiver->bind(cport+1);
    Packet MessagePacket;
    IpAddress SenderIP;
    uint8_t Message;
    unsigned short sport=cport+1;
    while(true)
    {
        if (Receiver->receive(MessagePacket, SenderIP, sport) == Socket::Done)
        {
            MessagePacket >> Message;
            cout << Message << endl;
            if(Message=='N')
            {
                uint16_t UType, UTeam, UNumber;
                MessagePacket >> UType >> UTeam >> UNumber;
                cout << UType << "|" << UTeam << "|" << UNumber << endl;
                short bx = GameMap.Buildings[UTeam*4+UnitTemplates[UType].Type].x, by = GameMap.Buildings[UTeam*4+UnitTemplates[UType].Type].y;
                if(UTeam==Player.Team) Cash-=UnitTemplates[UType].Cost;
                GameMap.Occupied[by][bx]=1;
                Unit PUnit = UnitTemplates[UType];
                PUnit.Number=UNumber;
                PUnit.Team=UTeam;
                PUnit.posx=bx;
                PUnit.posy=by;
                Units.push_back(PUnit);
            }
            else if(Message=='M')
            {
                uint16_t UNumber, UPosx, UPosy;
                MessagePacket >> UNumber >> UPosx >> UPosy;
                //cout << UNumber << " " << +UPosx << " " << +UPosy << endl;
                for(short i=0;i<Units.size();i++)
                {
                    if(Units[i].Number==UNumber)
                    {
                        //cout << Units.size() << "|";
                        GameMap.Occupied[Units[i].posy][Units[i].posx]=0;
                        //cout << Units.size() << "|";
                        Units[i].posx=UPosx;
                        //cout << Units.size() << "|";
                        Units[i].posy=UPosy;
                        //cout << Units.size() << "|";
                        GameMap.Occupied[UPosy][UPosx]=1;
                        //cout << Units.size() << "|";
                        if(GameMap.Tiles[UPosy][UPosx]->ID==2||GameMap.Tiles[UPosy][UPosx]->ID==26+1-Units[i].Team) GameMap.Tiles[UPosy][UPosx]=&Tiles[26+Units[i].Team];
                        //cout << Units.size() << endl;
                        break;
                    }
                }
            }
            else if(Message=='m')
            {
                uint16_t UNumber, UPosx, UPosy;
                MessagePacket >> UNumber >> UPosx >> UPosy;
                for(short i=0;i<Ordnances.size();i++)
                {
                    if(Ordnances[i].Number==UNumber)
                    {
                        Ordnances[i].posx=UPosx;
                        Ordnances[i].posy=UPosy;
                        break;
                    }
                }
            }
            else if(Message=='A')
            {
                uint16_t UNumber, UHP, UType;
                MessagePacket>>UNumber>>UHP>>UType;
                for(short i=0;i<Units.size();i++)
                {
                    if(Units[i].Number==UNumber)
                    {
                        Units[i].HP=UHP;
                        Effects.push_back(EffectTemplates[UnitTemplates[UType].EffectID]);
                        Effects[Effects.size()-1].Start(Units[i].posx,Units[i].posy);
                        if(Units[i].HP<=0) 
                        {
                            if(SelectedUnit!=nullptr&&SelectedUnit->Number==Units[i].Number) SelectedUnit=nullptr;
                            GameMap.Occupied[Units[i].posy][Units[i].posx]=0;
                            Units.erase(Units.begin()+i);
                        }
                        break;
                    }
                }
            }
            else if(Message=='a')
            {
                uint16_t UID,UTX,UTY,UTeam,UNumber;
                MessagePacket >> UID >> UTX >> UTY >> UTeam >> UNumber;
                Ordnances.push_back(Ordnance(Vector2i(UTX,UTY),UID,UnitTemplates[UID].AllDamage,UnitTemplates[UID].Speed,UnitTemplates[UID].Range,UnitTemplates[UID].EffectID,UTeam));
                Ordnances[Ordnances.size()-1].Number=UNumber;
            }
            else if(Message=='P')
            {
                uint16_t UTeam,UNumber,UTX,UTY,UID;
                MessagePacket >> UTeam >> UNumber >> UTX >> UTY >> UID;

                Unit PUnit = UnitTemplates[0];
                PUnit.Number=UNumber;
                PUnit.Team=UTeam;
                PUnit.posx=UTX;
                PUnit.posy=UTY;
                Units.push_back(PUnit);

                Effects.push_back(EffectTemplates[UnitTemplates[UID].EffectID]);
                Effects[Effects.size()-1].Start(UTX,UTY);
            }

            else if(Message=='C') MessagePacket >> Cash;
        }
    }
}