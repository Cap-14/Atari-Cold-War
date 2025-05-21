g++.exe -c main.cpp -Iinclude/
g++.exe -c AStar.cpp -Iinclude/
g++.exe -c engine.cpp -Iinclude/
g++.exe main.o AStar.o engine.o  -o ACW -Llib/ -lsfml-graphics -lsfml-audio -lsfml-window -lsfml-system -lsfml-network
