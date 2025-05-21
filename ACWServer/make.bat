g++.exe -c main.cpp -Iinclude/
g++.exe -c engine.cpp -Iinclude/
g++.exe main.o engine.o  -o ACW -Llib/ -lsfml-graphics -lsfml-audio -lsfml-window -lsfml-system -lsfml-network
