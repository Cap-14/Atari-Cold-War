g++ -c main.cpp -Iinclude/
g++ -c engine.cpp -Iinclude/
g++ main.o engine.o  -o ACW -Llib/ -lsfml-graphics -lsfml-audio -lsfml-window -lsfml-system -lsfml-network
