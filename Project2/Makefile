sht:
	@echo " Compile ht_main ...";
	gcc -I ./include/ -L ./lib/ -Wl,-rpath,./lib/ ./examples/sht_main.c ./src/hash_file.c ./src/sht_file.c -lbf -o ./build/runner -O2 -lm
	rm -r data.db
	rm -r data1.db
	rm -r data2.db
	rm -r data3.db
ht:
	@echo " Compile ht_main ...";
	gcc -I ./include/ -L ./lib/ -Wl,-rpath,./lib/ ./examples/ht_main.c ./src/hash_file.c -lbf -o ./build/runner -O2 -lm
	rm -r data.db

bf:
	@echo " Compile bf_main ...";
	gcc -I ./include/ -L ./lib/ -Wl,-rpath,./lib/ ./examples/bf_main.c -lbf -o ./build/runner -O2

