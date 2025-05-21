#include "engine.hpp"

int main()
{
    cout.sync_with_stdio(false);
    UdpSocket Connector, Verificator;
    Connector.bind(54960);

    for(short i=0;i<2;i++)
    {
        Players[i].port=54961+(i*3);
        Players[i].Listener.bind(54961+(i*3));
        Players[i].Listener.setBlocking(false);
    }

    /*RenderWindow GameWindow(VideoMode(Mul*20+Mul*5, Mul*20), "ACW", Style::None);
    GameWindow.setPosition(Vector2i(GameWindow.getPosition().x+Mul*5,GameWindow.getPosition().y));
    GameWindow.setVerticalSyncEnabled(true);
    RenderWindow GeneralWindow(VideoMode(120, 180), "ACW-General", Style::None);
    GeneralWindow.setVerticalSyncEnabled(true);
    GeneralWindow.setPosition(Vector2i(GameWindow.getPosition().x+400,GameWindow.getPosition().y));*/

    cout << "IP Adresa: " << IpAddress::getLocalAddress() << " ili " << IpAddress::getPublicAddress()  << endl;
    cout << "Unesi ime mape!" << endl;
    cin >> GameMap.Name;
    
    Initialise();

    cout << "Server je spreman" << endl << endl;

    thread ConRecThread(ConnectionReceiver,&Connector,&Verificator,Players); ConRecThread.detach();

    while (active)
    {
        if(Players[0].ready&&Players[1].ready&&Players[0].active&&Players[1].active) Tick();
    }
}