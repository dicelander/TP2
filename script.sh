#!/bin/bash

# Create the output directory if it doesn't exist
if [ ! -d "output" ]; then
   mkdir output
fi

# Run the client 3 times
./client $@
mv output.csv output/output1.csv
./client $@
mv output.csv output/output2.csv
./client $@
mv output.csv output/output3.csv

# Add the files to git, commit, and push
git add output/output1.csv
git add output/output2.csv
git add output/output3.csv
git commit -m "Resultados estao em output um para cada vez que o código inteiro rodou"
git push

