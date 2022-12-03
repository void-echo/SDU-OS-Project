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
            # touch ../$i/arch/$j/$k/placeholder.txt
            # if not exist, create a file named ../$i/arch/$j/$k/placeholder.txt
            if [ ! -f ../$i/arch/$j/$k/placeholder.txt ]; then
                touch ../$i/arch/$j/$k/placeholder.txt
                echo "placeholder" > ../$i/arch/$j/$k/placeholder.txt
            fi
            # write "placeholder" to ../$i/arch/$j/$k/placeholder.txt, in overwrite mode
        done
    done
done


# how to tar a directory
# tar -cvf ../lab2.tar ../lab2