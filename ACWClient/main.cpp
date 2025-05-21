#include "engine.hpp"

int main()
{
    //cout.sync_with_stdio(false);

    cout << "Unesi ime: ";
    cin >> Player.Name;
    cout << "Izaberi drzavu: " << endl;
    cout << "NATO--------------------------" << endl;
    cout << "(1) SAD" << endl;
    cout << "(2) EU" << endl;
    cout << "(3) Japan" << endl;
    cout << "VARSAVSKI PAKT----------------" << endl;
    cout << "(4) SSSR" << endl;
    cout << "(5) Kina" << endl;
    cout << "(6) Jugoslavija" << endl;
    cin >> Player.Side;
    Player.Side--;
    Player.Team=Player.Side/3;

    if(Player.Side==0) Player.NationName="USA";
    else if(Player.Side==1) Player.NationName="EUR";
    else if(Player.Side==2) Player.NationName="JAP";
    else if(Player.Side==3) Player.NationName="SOV";
    else if(Player.Side==4) Player.NationName="CHI";
    else Player.NationName="YUG";

    IpAddress UnusedIP;
    Packet MessagePacket;

    cout << "Unesi IP Adresu servera: ";
    cin >> IPAddress;
    Connector.bind(54999);

    MessagePacket<<Player.Name<<Player.Side;

    if (Sender.send(MessagePacket, IPAddress, 54960) != Socket::Done){}
    if (Connector.receive(MessagePacket,UnusedIP,port)!= Socket::Done){}

    uint8_t msg;
    MessagePacket>>GameMap.Name>>msg>>port;

    if(msg=='y')
    {
        Connector.bind(55000);
        if (Connector.receive(MessagePacket,UnusedIP,port)!= Socket::Done){}
        MessagePacket>>port;
    }
    MessagePacket.clear();

    thread ReceiveThread(Receive,&Receiver,IPAddress,port); ReceiveThread.detach();
    thread PingPongThread(PingPong,IPAddress,port); PingPongThread.detach();

    Connector.unbind();
    cout << "Konektovan!" << endl;
    
    fstream SettingsFile("Settings.txt");
    SettingsFile >> Mul;
    SettingsFile.close();
    
    RenderWindow GameWindow(VideoMode(Mul*20+Mul*5, Mul*20), "Igra", Style::None);
    GameWindow.setPosition(Vector2i(GameWindow.getPosition().x+Mul*5,GameWindow.getPosition().y));
    GameWindow.setVerticalSyncEnabled(true);

    Initialise();

    msg='R';
    MessagePacket << msg;
    if (Sender.send(MessagePacket, IPAddress, port) != Socket::Done){}

    while (GameWindow.isOpen())
    {
        mx = Mouse::getPosition(GameWindow).x;
        my = Mouse::getPosition(GameWindow).y;
        
        Event GameEvent;
        while (GameWindow.pollEvent(GameEvent))
        {
            if (GameEvent.type==Event::Closed)
            {
                GameWindow.close();
            }
            else if (GameEvent.type==Event::MouseButtonPressed)
            {
                if(Mouse::isButtonPressed(Mouse::Left))
                {
                    LeftClick();
                }
                else if(Mouse::isButtonPressed(Mouse::Right))
                {
                    RightClick();
                }
            }
        }

        Input();
        Tick();

        GameWindow.clear();
        Draw(GameWindow);
        GameWindow.display();

        /*GeneralWindow.clear();
        GeneralWindow.display();*/
    }
}