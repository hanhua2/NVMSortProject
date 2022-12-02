build_all:
	g++ -std=c++17 -o GenerateData.o GenerateData.cpp -fopenmp -lpthread -lpmem
	g++ -std=c++17 -o Main.o sortMain.cpp -fopenmp -lpthread -lpmem


test:
	g++ -std=c++17 -o Test.o Test/RwTest.cpp -fopenmp -lpthread -lpmem