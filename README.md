For server.cpp compiling ( in ~/server/ folder )

 g++ -std=c++17 -Itools -Ifile_processing server.cpp ../file_processing/file_proccessing.cpp ../tools/SMA/price_buffer.cpp -o server -pthread

For client.cpp compiling ( in ~/client/ folder )

g++ -std=c++17 -Itools client.cpp ../tools/price_generator/price_generator.cpp -o client -pthread


all paths are relative
