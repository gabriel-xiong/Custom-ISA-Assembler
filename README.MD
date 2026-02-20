Gabriel Xiong
gx566

How to run simulator/assembler:

gcc -o hw5-asm hw5-asm.c
gcc -o hw5-sim hw5-sim.c
./hw5-asm <input.tk> <output.tko>
./hw5-sim <input.tko>

How to run tests: 

cd testing/test_tinker_files
chmod +x *.sh
./test_binary_search.sh
./test_fibonacci.sh
./test_matrix.sh