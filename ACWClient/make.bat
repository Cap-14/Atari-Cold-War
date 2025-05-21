g++ -c main.cpp -Iinclude/
g++ -c AStar.cpp -Iinclude/
g++ -c engine.cpp -Iinclude/
g++ main.o AStar.o engine.o  -o ACW -Llib/ -lsfml-graphics -lsfml-audio -lsfml-window -lsfml-system -lsfml-network
