declare -a arr=("bin" "demo0" "demo1" "filesys" "lab2" "lab3" "lab4" "lab5" "lab6" "lab7" "monitor" "network" "test" "threads" "userprog");

declare -a brr=("dec-alpha-osf" "dec-mips-ultrix" "sun-sparc-sunos" "unknown-i386-linux");

declare -a crr=("bin" "depends" "objects");

for i in "${arr[@]}"
do
    for j in "${brr[@]}"
    do
        for k in "${crr[@]}"
        do
            mkdir -p ../$i/arch/$j/$k
            # makedir -p means make parent directories as needed
            touch ../$i/arch/$j/$k/placeholder.txt
        done
    done
done